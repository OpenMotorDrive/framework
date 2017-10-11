#include <common/slip.h>

uint8_t slip_encode_and_append(uint8_t byte, uint8_t* out_buf_len, uint8_t *out_buf, uint8_t out_buf_size) {
    if (byte == SLIP_END) {
        if ((*out_buf_len) > out_buf_size-2) {
            return 0;
        }

        out_buf[(*out_buf_len)++] = SLIP_ESC;
        out_buf[(*out_buf_len)++] = SLIP_ESC_END;
    } else if (byte == SLIP_ESC) {
        if ((*out_buf_len) > out_buf_size-2) {
            return 0;
        }

        out_buf[(*out_buf_len)++] = SLIP_ESC;
        out_buf[(*out_buf_len)++] = SLIP_ESC_ESC;
    } else {
        if ((*out_buf_len) > out_buf_size-1) {
            return 0;
        }

        out_buf[(*out_buf_len)++] = byte;
    }
    return 1;
}

uint8_t slip_decode(uint8_t in_len, uint8_t *in_buf, uint8_t *out_buf) {
    uint8_t i;
    uint8_t out_len = 0;
    uint8_t esc_flag = 0;

    for (i=0; i<in_len; i++) {
        if (esc_flag) {
            if (in_buf[i] == SLIP_ESC_ESC) {
                out_buf[out_len++] = SLIP_ESC;
            } else if (in_buf[i] == SLIP_ESC_END) {
                out_buf[out_len++] = SLIP_END;
            } else {
                // invalid escape character
                return 0;
            }
            esc_flag = 0;
        } else if (in_buf[i] == SLIP_ESC) {
            esc_flag = 1;
        } else if (in_buf[i] == SLIP_END) {
            return out_len;
        } else {
            out_buf[out_len++] = in_buf[i];
        }
    }

    return 0;
}
