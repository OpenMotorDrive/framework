#include <common/crc64_we.h>

uint64_t crc64_we(const uint8_t *buf, uint32_t len, uint64_t crc)
{
    uint32_t i;
    uint8_t j;

    crc = ~crc;

    for (i = 0; i < len; i++) {
        crc ^= ((uint64_t)buf[i]) << 56;
        for (j = 0; j < 8; j++) {
            crc = (crc & (1ULL<<63)) ? (crc<<1)^0x42F0E1EBA9EA3693ULL : (crc<<1);
        }
    }

    return ~crc;
}
