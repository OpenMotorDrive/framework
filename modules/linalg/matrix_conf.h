#pragma once

#define MATRIX_ASSERT(x) chDbgCheck((x))
#define MATRIX_MALLOC(size) chHeapAlloc(NULL, (size))
#define MATRIX_FREE(v) chHeapFree((v))
