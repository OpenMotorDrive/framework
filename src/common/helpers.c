/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/helpers.h>
#include <math.h>
#include <stdint.h>

float wrap_1(float x) {
    volatile float z = (x + 25165824.0f);
    x = x - (z - 25165824.0f);
    return x;
}

float constrain_float(float val, float min_val, float max_val)
{
    if (val < min_val) {
        return min_val;
    }
    if (val > max_val) {
        return max_val;
    }
    return val;
}

float wrap_pi(float x)
{
    return wrap_1(x/M_PI_F)*M_PI_F;
}

float wrap_2pi(float x)
{
    x = wrap_pi(x);

    if (x < 0) {
        x += 2*M_PI_F;
    }

    return x;
}

float sinf_fast(float x)
{
    const float Q = 3.1f;
    const float P = 3.6f;
    float y;

    x = wrap_1(x/M_PI_F);
    y = x - x * fabsf(x);
    return y * (Q + P * fabsf(y));
}

float cosf_fast(float x)
{
    return sinf_fast(x+M_PI_F/2.0f);
}

void transform_a_b_c_to_alpha_beta(float a, float b, float c, float* alpha, float* beta)
{
    *alpha = 0.666666666666667*a - 0.333333333333333*b - 0.333333333333333*c;
    *beta = 0.577350269189626*b - 0.577350269189626*c;
}

void transform_alpha_beta_to_a_b_c(float alpha, float beta, float* a, float* b, float* c)
{
    *a = alpha;
    *b = -0.5*alpha + 0.866025403784439*beta;
    *c = -0.5*alpha - 0.866025403784439*beta;
}

void transform_d_q_to_alpha_beta(float theta, float d, float q, float* alpha, float* beta)
{
    float sin_theta = sinf_fast(theta);
    float cos_theta = cosf_fast(theta);

    *alpha = d*cos_theta - q*sin_theta;
    *beta = d*sin_theta + q*cos_theta;
}

void transform_alpha_beta_to_d_q(float theta, float alpha, float beta, float* d, float* q)
{
    float sin_theta = sinf_fast(theta);
    float cos_theta = cosf_fast(theta);

    *d = alpha*cos_theta + beta*sin_theta;
    *q = -alpha*sin_theta + beta*cos_theta;
}

// FNV-1a implementation
#define FNV_1_PRIME_64 1099511628211UL

void hash_fnv_1a(uint32_t len, const uint8_t* buf, uint64_t* hash)
{
    uint32_t i;
    for (i=0; i<len; i++) {
        *hash ^= (uint64_t)buf[i];
        *hash *= FNV_1_PRIME_64;
    }
}

uint16_t crc16_ccitt(const void *buf, size_t len, uint16_t crc) {
    for (size_t i = 0; i < len; i++) {
        crc = crc ^ (((uint8_t*)buf)[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

uint32_t crc32(const uint8_t *buf, uint32_t len, uint32_t crc)
{
    uint32_t i;
    uint8_t j;

    crc = ~crc;
    for (i = 0; i < len; i++) {
        crc = crc ^ buf[i];
        for (j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}
