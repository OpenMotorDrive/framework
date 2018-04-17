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

#pragma once

#include <math.h>
#include <stdint.h>

#define M_SQRT2_F ((float)M_SQRT2)
#define M_PI_F ((float)M_PI)

#define SQ(__X) ((__X)*(__X))

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define bkpt() __asm volatile("BKPT #0\n")

#define SIGN(x) ( (x)>=0 ? 1 : -1 )

#define LEN(x) (sizeof(x)/sizeof(x[0]))

#define FNV_1_OFFSET_BASIS_64 14695981039346656037UL

#define UNUSED(x) ((void)(x))

#define LINKED_LIST_APPEND(TYPE, HEAD_PTR, NEW_ITEM_PTR) { \
    (NEW_ITEM_PTR)->next = NULL; \
    TYPE** insert_ptr = &(HEAD_PTR); \
    while(*insert_ptr) { \
        insert_ptr = &(*insert_ptr)->next; \
    } \
    *insert_ptr = (NEW_ITEM_PTR); \
}

#define LINKED_LIST_REMOVE(TYPE, HEAD_PTR, REMOVE_ITEM_PTR) { \
    TYPE** remove_ptr = &(HEAD_PTR); \
    while (*remove_ptr && *remove_ptr != (REMOVE_ITEM_PTR)) { \
        remove_ptr = &(*remove_ptr)->next; \
    } \
    \
    if (*remove_ptr) { \
        *remove_ptr = (*remove_ptr)->next; \
    } \
}


float sinf_fast(float x);
float cosf_fast(float x);
float constrain_float(float val, float min_val, float max_val);
float wrap_1(float x);
float wrap_2pi(float val);
float wrap_pi(float val);

void transform_a_b_c_to_alpha_beta(float a, float b, float c, float* alpha, float* beta);
void transform_alpha_beta_to_a_b_c(float alpha, float beta, float* a, float* b, float* c);
void transform_d_q_to_alpha_beta(float theta, float d, float q, float* alpha, float* beta);
void transform_alpha_beta_to_d_q(float theta, float alpha, float beta, float* d, float* q);

void hash_fnv_1a(uint32_t len, const uint8_t* buf, uint64_t* hash);
uint16_t crc16_ccitt(const void *buf, size_t len, uint16_t crc);
uint32_t crc32(const uint8_t *buf, uint32_t len, uint32_t crc);
