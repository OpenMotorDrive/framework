#pragma once

#include <stdint.h>
#include <stdbool.h>

struct matrixXf_s {
    uint8_t rows;
    uint8_t cols;
    float val[];
};

struct matrix3f_s {
    uint8_t rows;
    uint8_t cols;
    float val[9];
};

#define MAT_FIELD(M,r,c) ((M).val[(r)*(M).cols+(c)])
#define MAT_SIZE(r,c) (sizeof(struct matrixXf_s)+sizeof(float)*(size_t)(r)*(size_t)(c))

void mat_zero(struct matrixXf_s* M);
void mat_mul(struct matrixXf_s* A, struct matrixXf_s* B, struct matrixXf_s* AB);
bool mat_inverse(struct matrixXf_s* A, struct matrixXf_s* M_ret);
void mat_print(struct matrixXf_s* M);
