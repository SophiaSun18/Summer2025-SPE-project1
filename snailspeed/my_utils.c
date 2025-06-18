/**
 * Copyright (c) 2024 MIT License by 6.106 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include "../utils/utils.h"
#include "my_utils.h"
#include <string.h>


// get, set, and rotate for block size = 64
void get_block_64(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_dst[]) {

    for (uint32_t y = 0; y < block_size; y++) {
        uint8_t *src = img + (j + y) * row_size + i / 8;
        memcpy(&block_dst[y], src, sizeof(uint64_t));
        block_dst[y] = __builtin_bswap64(block_dst[y]);
    }
}

void rotate_and_set_block_64(uint8_t *img, const bytes_t row_size, uint32_t di, uint32_t dj, uint32_t block_size, uint64_t block[]) {

    // rotate row r left by r + 1
    for (int r = 0; r < block_size; r++) {
        block[r] = __builtin_rotateleft64(block[r], r + 1);
    }

    // rotate column c down by c + 1
    uint64_t scratch[block_size];

    for (int r = 0; r < 32; r++) {
        scratch[r] = (block[r] & 0xFFFFFFFF00000000ULL) | (block[r + 32] & 0x00000000FFFFFFFFULL);
        scratch[r + 32] = (block[r + 32] & 0xFFFFFFFF00000000ULL) | (block[r] & 0x00000000FFFFFFFFULL);
    }

    for (int r = 0; r < 16; r++) {
        block[r] = (scratch[r] & 0xFFFF0000FFFF0000ULL) | (scratch[r + 48] & 0x0000FFFF0000FFFFULL);
    }
    for (int r = 0; r < 48; r++) {
        block[r + 16] = (scratch[r + 16] & 0xFFFF0000FFFF0000ULL) | (scratch[r] & 0x0000FFFF0000FFFFULL);
    }

    for (int r = 0; r < 8; r++) {
        scratch[r] = (block[r] & 0xFF00FF00FF00FF00ULL) | (block[r + 56] & 0x00FF00FF00FF00FFULL);
    }
    for (int r = 0; r < 56; r++) {
        scratch[r + 8] = (block[r + 8] & 0xFF00FF00FF00FF00ULL) | (block[r] & 0x00FF00FF00FF00FFULL);
    }

    for (int r = 0; r < 4; r++) {
        block[r] = (scratch[r] & 0xF0F0F0F0F0F0F0F0ULL) | (scratch[r + 60] & 0x0F0F0F0F0F0F0F0FULL);
    }
    for (int r = 0; r < 60; r++) {
        block[r + 4] = (scratch[r + 4] & 0xF0F0F0F0F0F0F0F0ULL) | (scratch[r] & 0x0F0F0F0F0F0F0F0FULL);
    }

    for (int r = 0; r < 2; r++) {
        scratch[r] = (block[r] & 0xCCCCCCCCCCCCCCCCULL) | (block[r + 62] & 0x3333333333333333ULL);
    }
    for (int r = 0; r < 62; r++) {
        scratch[r + 2] = (block[r + 2] & 0xCCCCCCCCCCCCCCCCULL) | (block[r] & 0x3333333333333333ULL);
    }

    block[0] = (scratch[0] & 0xAAAAAAAAAAAAAAAAULL) | (scratch[63] & 0x5555555555555555ULL);
    for (int r = 0; r < 63; r++) {
        block[r + 1] = (scratch[r + 1] & 0xAAAAAAAAAAAAAAAAULL) | (scratch[r] & 0x5555555555555555ULL);
    }

    scratch[0] = block[63];
    for (int r = 0; r < 63; r++) {
        scratch[r + 1] = block[r];
    }
        
    // rotate row r left by r and set back to the destination in the matrix
    for (uint32_t y = 0; y < block_size; y++) {
        scratch[y] = __builtin_rotateleft64(scratch[y], y);
        uint8_t *dst = img + (dj + y) * row_size + di / 8;
        uint64_t val = __builtin_bswap64(scratch[y]);
        memcpy(dst, &val, sizeof(uint64_t));
    }
}