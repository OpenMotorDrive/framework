#include <common/ctor.h>
#include <hal.h>
#include <modules/pubsub/pubsub.h>
#include <modules/worker_thread/worker_thread.h>
#include <modules/lpwork_thread/lpwork_thread.h> // TODO: use high priority worker thread for CAN
#include <modules/can/can_driver.h>

#if !defined(CAN1) && defined(CAN)
#define CAN1 CAN
#endif

#undef CAN_BTR_BRP
#define CAN_BTR_BRP(n) (n)
#undef CAN_BTR_TS1
#define CAN_BTR_TS1(n) ((n) << 16)
#undef CAN_BTR_TS2
#define CAN_BTR_TS2(n) ((n) << 20)
#undef CAN_BTR_SJW
#define CAN_BTR_SJW(n) ((n) << 24)

#define NUM_TX_MAILBOXES 3
#define NUM_RX_MAILBOXES 2
#define RX_FIFO_DEPTH 3

static void can_driver_stm32_start(void* ctx, bool silent, bool auto_retransmit, uint32_t baudrate);
static void can_driver_stm32_stop(void* ctx);
bool can_driver_stm32_abort_tx_mailbox_I(void* ctx, uint8_t mb_idx);
bool can_driver_stm32_load_tx_mailbox_I(void* ctx, uint8_t mb_idx, struct can_frame_s* frame);

static const struct can_driver_iface_s can_driver_stm32_iface = {
    can_driver_stm32_start,
    can_driver_stm32_stop,
    can_driver_stm32_abort_tx_mailbox_I,
    can_driver_stm32_load_tx_mailbox_I,
};

struct can_driver_stm32_instance_s {
    struct can_instance_s* frontend;
    CAN_TypeDef* can;
};

static struct can_driver_stm32_instance_s can1_instance;

RUN_ON(CAN_INIT) {
    // TODO make this index configurable and enable multiple instances
    can1_instance.can = CAN1;
    can1_instance.frontend = can_driver_register(0, &can1_instance, &can_driver_stm32_iface, NUM_TX_MAILBOXES, NUM_RX_MAILBOXES, RX_FIFO_DEPTH);
}

static void can_driver_stm32_start(void* ctx, bool silent, bool auto_retransmit, uint32_t baudrate) {
    struct can_driver_stm32_instance_s* instance = ctx;

    rccEnableCAN1(FALSE);

    instance->can->FMR = (instance->can->FMR & 0xFFFF0000) | CAN_FMR_FINIT;
    instance->can->sFilterRegister[0].FR1 = 0;
    instance->can->sFilterRegister[0].FR2 = 0;
    instance->can->FM1R = 0;
    instance->can->FFA1R = 0;
    instance->can->FS1R = 1;
    instance->can->FA1R = 1;

    instance->can->FMR &= ~CAN_FMR_FINIT;

    nvicEnableVector(STM32_CAN1_TX_NUMBER, STM32_CAN_CAN1_IRQ_PRIORITY);
    nvicEnableVector(STM32_CAN1_RX0_NUMBER, STM32_CAN_CAN1_IRQ_PRIORITY);
    nvicEnableVector(STM32_CAN1_SCE_NUMBER, STM32_CAN_CAN1_IRQ_PRIORITY);

    instance->can->MCR = CAN_MCR_INRQ;
    while((instance->can->MSR & CAN_MSR_INAK) == 0) {
        chThdSleep(MS2ST(1));
    }

    instance->can->BTR = (silent?CAN_BTR_SILM:0) | CAN_BTR_SJW(0) | CAN_BTR_TS2(2-1) | CAN_BTR_TS1(15-1) | CAN_BTR_BRP((STM32_PCLK1/18)/baudrate - 1);
    instance->can->MCR = CAN_MCR_ABOM | CAN_MCR_AWUM | (auto_retransmit?0:CAN_MCR_NART);

    instance->can->IER = CAN_IER_TMEIE | CAN_IER_FMPIE0; // TODO: review reference manual for other interrupt flags needed
}

static void can_driver_stm32_stop(void* ctx) {
    struct can_driver_stm32_instance_s* instance = ctx;

    instance->can->MCR = 0x00010002;
    instance->can->IER = 0x00000000;

    nvicDisableVector(STM32_CAN1_TX_NUMBER);
    nvicDisableVector(STM32_CAN1_RX0_NUMBER);
    nvicDisableVector(STM32_CAN1_SCE_NUMBER);

    rccDisableCAN1(FALSE);
}

bool can_driver_stm32_abort_tx_mailbox_I(void* ctx, uint8_t mb_idx) {
    struct can_driver_stm32_instance_s* instance = ctx;

    chDbgCheckClassI();

    switch(mb_idx) {
        case 0:
            instance->can->TSR = CAN_TSR_ABRQ0;
            return true;
        case 1:
            instance->can->TSR = CAN_TSR_ABRQ1;
            return true;
        case 2:
            instance->can->TSR = CAN_TSR_ABRQ2;
            return true;
    }
    return false;
}

bool can_driver_stm32_load_tx_mailbox_I(void* ctx, uint8_t mb_idx, struct can_frame_s* frame) {
    struct can_driver_stm32_instance_s* instance = ctx;

    chDbgCheckClassI();

    CAN_TxMailBox_TypeDef* mailbox = &instance->can->sTxMailBox[mb_idx];

    mailbox->TDTR = frame->DLC;
    mailbox->TDLR = frame->data32[0];
    mailbox->TDHR = frame->data32[1];

    if (frame->IDE) {
        mailbox->TIR = ((uint32_t)frame->EID << 3) | (frame->RTR ? CAN_TI0R_RTR : 0) | CAN_TI0R_IDE | CAN_TI0R_TXRQ;
    } else {
        mailbox->TIR = ((uint32_t)frame->SID << 21) | (frame->RTR ? CAN_TI0R_RTR : 0) | CAN_TI0R_TXRQ;
    }

    return true;
}

static void can_driver_stm32_retreive_rx_frame_I(struct can_frame_s* frame, CAN_FIFOMailBox_TypeDef* mailbox) {
    frame->data32[0] = mailbox->RDLR;
    frame->data32[1] = mailbox->RDHR;
    frame->RTR = (mailbox->RIR & CAN_RI0R_RTR) != 0;
    frame->IDE = (mailbox->RIR & CAN_RI0R_IDE) != 0;
    if (frame->IDE) {
        frame->EID = (mailbox->RIR & (CAN_RI0R_STID|CAN_RI0R_EXID)) >> 3;
    } else {
        frame->SID = (mailbox->RIR & CAN_RI0R_STID) >> 21;
    }
    frame->DLC = mailbox->RDTR & CAN_RDT0R_DLC;
}

static void stm32_can_rx_handler(struct can_driver_stm32_instance_s* instance) {
    systime_t rx_systime = chVTGetSystemTimeX();
    while (true) {
        chSysLockFromISR();
        if ((instance->can->RF0R & CAN_RF0R_FMP0) == 0) {
            chSysUnlockFromISR();
            break;
        }
        struct can_frame_s frame;
        can_driver_stm32_retreive_rx_frame_I(&frame, &instance->can->sFIFOMailBox[0]);
        can_driver_rx_frame_received_I(instance->frontend, 0, rx_systime, &frame);
        instance->can->RF0R = CAN_RF0R_RFOM0;
        chSysUnlockFromISR();
    }

    while (true) {
        chSysLockFromISR();
        if ((instance->can->RF1R & CAN_RF1R_FMP1) == 0) {
            chSysUnlockFromISR();
            break;
        }
        struct can_frame_s frame;
        can_driver_stm32_retreive_rx_frame_I(&frame, &instance->can->sFIFOMailBox[1]);
        can_driver_rx_frame_received_I(instance->frontend, 1, rx_systime, &frame);
        instance->can->RF1R = CAN_RF1R_RFOM1;
        chSysUnlockFromISR();
    }
}

static void stm32_can_tx_handler(struct can_driver_stm32_instance_s* instance) {
    systime_t t_now = chVTGetSystemTimeX();

    chSysLockFromISR();
    if ((instance->can->TSR & CAN_TSR_RQCP0) != 0) {
        can_driver_tx_request_complete_I(instance->frontend, 0, (instance->can->TSR & CAN_TSR_TXOK0) != 0, t_now);
        instance->can->TSR = CAN_TSR_RQCP0;
    }

    if ((instance->can->TSR & CAN_TSR_RQCP1) != 0) {
        can_driver_tx_request_complete_I(instance->frontend, 1, (instance->can->TSR & CAN_TSR_TXOK1) != 0, t_now);
        instance->can->TSR = CAN_TSR_RQCP1;
    }

    if ((instance->can->TSR & CAN_TSR_RQCP2) != 0) {
        can_driver_tx_request_complete_I(instance->frontend, 2, (instance->can->TSR & CAN_TSR_TXOK2) != 0, t_now);
        instance->can->TSR = CAN_TSR_RQCP2;
    }
    chSysUnlockFromISR();
}

OSAL_IRQ_HANDLER(STM32_CAN1_TX_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    stm32_can_tx_handler(&can1_instance);

    OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(STM32_CAN1_RX0_HANDLER) {
    OSAL_IRQ_PROLOGUE();

    stm32_can_rx_handler(&can1_instance);

    OSAL_IRQ_EPILOGUE();
}
