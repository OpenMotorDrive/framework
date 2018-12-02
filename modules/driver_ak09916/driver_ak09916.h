#include <modules/driver_icm20x48/driver_icm20x48.h>

struct ak09916_instance_s {
    struct icm20x48_instance_s *icm_dev;
    struct {
        uint64_t timestamp;
        float x;
        float y;
        float z;
        bool hofl;
    } meas;
};

bool ak09916_init(struct ak09916_instance_s *instance, struct icm20x48_instance_s* icm_instance);
bool ak09916_update(struct ak09916_instance_s* instance);
void ak09916_recv_byte(uint8_t recv_byte_idx, uint8_t recv_byte);
uint8_t ak09916_send_byte(void);
