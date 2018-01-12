#include "matrixXf.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef MATRIX_ASSERT
#define MATRIX_ASSERT(x) {}
#endif

#ifndef MATRIX_MALLOC
#define MATRIX_MALLOC(size) malloc((size))
#endif

#ifndef MATRIX_FREE
#define MATRIX_FREE(v) free((v))
#endif

void mat_zero(struct matrixXf_s* M) {
    memset(M->val, 0, sizeof(float) * M->rows * M->cols);
}

void mat_mul(struct matrixXf_s* A, struct matrixXf_s* B, struct matrixXf_s* AB_ret) {
    MATRIX_ASSERT(A != AB_ret && B != AB_ret);
    MATRIX_ASSERT(AB_ret->rows == A->rows && AB_ret->cols == B->cols && A->cols == B->rows);

    mat_zero(AB_ret);

    for(uint8_t i = 0; i < A->rows; i++) {
        for(uint8_t j = 0; j < B->cols; j++) {
            for(uint8_t k = 0;k < A->cols; k++) {
                MAT_FIELD(*AB_ret, i, j) += MAT_FIELD(*A, i, k) * MAT_FIELD(*B, k, j);
            }
        }
    }
}

static inline void swap(float* a, float* b)
{
    float c;
    c = *a;
    *a = *b;
    *b = c;
}

static void mat_pivot(struct matrixXf_s* A, struct matrixXf_s* P_ret) {
    uint8_t n = A->rows;

    MATRIX_ASSERT(A != P_ret);
    MATRIX_ASSERT(n == A->cols && n == P_ret->rows && n == P_ret->cols);

    for(uint8_t i = 0; i<n; i++){
        for(uint8_t j=0; j<n; j++) {
            MAT_FIELD(*P_ret, i, j) = i==j?1:0;
        }
    }

    for(uint8_t i = 0; i < n; i++) {
        uint8_t max_j = i;
        for(uint8_t j=i;j<n;j++){
            if(fabsf(MAT_FIELD(*A, j, i)) > fabsf(MAT_FIELD(*A, max_j, i))) {
                max_j = j;
            }
        }

        if(max_j != i) {
            for(uint8_t k = 0; k < n; k++) {

                swap(&MAT_FIELD(*P_ret, i, k), &MAT_FIELD(*P_ret, max_j, k));
            }
        }
    }
}

static void mat_forward_sub(struct matrixXf_s* L, struct matrixXf_s* M_ret) {
    uint8_t n = L->rows;
    MATRIX_ASSERT(L != M_ret);
    MATRIX_ASSERT(L->cols == n && M_ret->rows == n && M_ret->cols == n);

    mat_zero(M_ret);

    // Forward substitution solve LY = I
    for(int i = 0; i < n; i++) {
        MAT_FIELD(*M_ret, i, i) = 1/MAT_FIELD(*L, i, i);
        for (int j = i+1; j < n; j++) {
            for (int k = i; k < j; k++) {
                MAT_FIELD(*M_ret, j, i) -= MAT_FIELD(*L, j, k) * MAT_FIELD(*M_ret, k, i);
            }
            MAT_FIELD(*M_ret, j, i) /= MAT_FIELD(*L, j, j);
        }
    }
}

static void mat_backward_sub(struct matrixXf_s* U, struct matrixXf_s* M_ret) {
    uint8_t n = U->rows;
    MATRIX_ASSERT(U != M_ret);
    MATRIX_ASSERT(U->cols == n && M_ret->rows == n && M_ret->cols == n);

    mat_zero(M_ret);

    // Backward Substitution solve UY = I
    for(int i = n-1; i >= 0; i--) {
        MAT_FIELD(*M_ret, i, i) = 1/MAT_FIELD(*U, i, i);
        for (int j = i - 1; j >= 0; j--) {
            for (int k = i; k > j; k--) {
                MAT_FIELD(*M_ret, j, i) -= MAT_FIELD(*U, j, k) * MAT_FIELD(*M_ret, k, i);
            }
            MAT_FIELD(*M_ret, j, i) /= MAT_FIELD(*U, j, j);
        }
    }
}

static bool mat_LU_decompose(struct matrixXf_s* A, struct matrixXf_s* L_ret, struct matrixXf_s* U_ret, struct matrixXf_s* P_ret) {
    uint8_t n = A->rows;
    MATRIX_ASSERT(A != L_ret && A != U_ret && A != P_ret);
    MATRIX_ASSERT(L_ret != U_ret && L_ret != P_ret);
    MATRIX_ASSERT(U_ret != P_ret);
    MATRIX_ASSERT(A->rows == n && A->cols == n && L_ret->rows == n && L_ret->cols == n && U_ret->rows == n && U_ret->cols == n && P_ret->rows == n && P_ret->cols == n);

    struct matrixXf_s* A_prime = MATRIX_MALLOC(MAT_SIZE(n,n));
    if (!A_prime) {
        return false;
    }
    A_prime->rows = n;
    A_prime->cols = n;

    mat_pivot(A,P_ret);

    mat_mul(P_ret,A,A_prime);

    mat_zero(L_ret);
    mat_zero(U_ret);

    for(uint8_t i = 0; i < n; i++) {
        MAT_FIELD(*L_ret, i, i) = 1;
    }
    for(uint8_t i = 0; i < n; i++) {
        for(uint8_t j = 0; j < n; j++) {
            if(j <= i) {
                MAT_FIELD(*U_ret, j, i) = MAT_FIELD(*A_prime, j, i);
                for(uint8_t k = 0; k < j; k++) {
                    MAT_FIELD(*U_ret, j, i) -= MAT_FIELD(*L_ret, j, k) * MAT_FIELD(*U_ret, k, i);
                }
            }
            if(j >= i) {
                MAT_FIELD(*L_ret, j, i) = MAT_FIELD(*A_prime, j, i);
                for(uint8_t k = 0; k < i; k++) {
                    MAT_FIELD(*L_ret, j, i) -= MAT_FIELD(*L_ret, j, k) * MAT_FIELD(*U_ret, k, i);
                }
                MAT_FIELD(*L_ret, j, i) /= MAT_FIELD(*U_ret, i, i);
            }
        }
    }
    MATRIX_FREE(A_prime);
    return true;
}

bool mat_inverse(struct matrixXf_s* A, struct matrixXf_s* M_ret) {
    uint8_t n = A->rows;
    MATRIX_ASSERT(A->rows == A->cols);

    void* workspace = MATRIX_MALLOC(MAT_SIZE(n,n)*4);
    if (!workspace) {
        return false;
    }
    void* workspace_tail = workspace;

    struct matrixXf_s* L = workspace_tail;
    L->rows = n;
    L->cols = n;
    workspace_tail = (uint8_t*)workspace_tail + MAT_SIZE(n,n);
    // 1/4 allocated

    struct matrixXf_s* U = workspace_tail;
    U->rows = n;
    U->cols = n;
    workspace_tail = (uint8_t*)workspace_tail + MAT_SIZE(n,n);
    // 2/4 allocated

    struct matrixXf_s* P = workspace_tail;
    P->rows = n;
    P->cols = n;
    workspace_tail = (uint8_t*)workspace_tail + MAT_SIZE(n,n);
    // 3/4 allocated

    if (!mat_LU_decompose(A,L,U,P)) {
        MATRIX_FREE(workspace);
        return false;
    }

    struct matrixXf_s* L_inv = workspace_tail;
    L_inv->rows = n;
    L_inv->cols = n;
    workspace_tail = (uint8_t*)workspace_tail + MAT_SIZE(n,n);
    // 4/4 allocated
    mat_forward_sub(L, L_inv);
    // L is now free

    struct matrixXf_s* U_inv = L;
    mat_backward_sub(U, U_inv);
    // U is now free

    struct matrixXf_s* inv_unpivoted = U;
    mat_mul(U_inv,L_inv,inv_unpivoted);
    // U_inv and L_inv are now free

    struct matrixXf_s* inv_pivoted = U_inv;
    mat_mul(inv_unpivoted,P,inv_pivoted);

    for(uint8_t i = 0; i < n; i++) {
        for(uint8_t j = 0; j < n; j++) {
            if(isnan(MAT_FIELD(*inv_pivoted, i, j)) || isinf(MAT_FIELD(*inv_pivoted, i, j))){
                MATRIX_FREE(workspace);
                return false;
            }
        }
    }

    memcpy(M_ret, inv_pivoted, MAT_SIZE(n,n));
    MATRIX_FREE(workspace);
    return true;
}
