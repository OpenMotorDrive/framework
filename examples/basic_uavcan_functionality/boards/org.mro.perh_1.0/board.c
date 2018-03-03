#include <hal.h>

void boardInit(void) {
    // default AFIO mapping puts CAN RX/TX on PA11/PA12
    // make sure USB isn't mapped to those pins
//    palSetLineMode(BOARD_PAL_LINE_CAN_RX, PAL_MODE_INPUT);
//    palSetLineMode(BOARD_PAL_LINE_CAN_TX, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
//    palSetPadMode(GPIOA, 11, PAL_MODE_INPUT);
//    palSetPadMode(GPIOA, 12, PAL_MODE_STM32_ALTERNATE_PUSHPULL);

//    // maple-mini LED
//    palSetPadMode(GPIOB, 1, PAL_MODE_OUTPUT_PUSHPULL);
    // bluepill LED
//    palSetPadMode(GPIOC, 13, PAL_MODE_OUTPUT_PUSHPULL);

//    /* Configure CAN pin: RX */
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN_RX;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
//    GPIO_Init(GPIO_CAN, &GPIO_InitStructure);

//    /* Configure CAN pin: TX */
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN_TX;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIO_CAN, &GPIO_InitStructure);

//    GPIO_PinRemapConfig(GPIO_Remapping_CAN , ENABLE);
}
