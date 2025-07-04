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
void get_block_64(uint64_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint64_t block_dst[]) {

    int word_offset = i / 64;
    for (int y = 0; y < 64; y++) {
        __builtin_prefetch(&img[(j + y + 16) * row_size + word_offset]);
        block_dst[y] = __builtin_bswap64(img[(j + y) * row_size + word_offset]);
    }
}

void rotate_and_set_block_64(uint64_t *img, const bytes_t row_size, uint32_t di, uint32_t dj, uint64_t block[]) {

    // rotate row r left by r + 1
    for (int r = 0; r < 64; r++) {
        block[r] = __builtin_rotateleft64(block[r], r + 1);
    }

    // rotate column c down by c + 1
    uint64_t scratch[64];
    int r;
    for (r = 0; r < 32; r++) {
        scratch[r] = (block[r] & 0xFFFFFFFF00000000) | (block[r + 32] & 0x00000000FFFFFFFF);
        scratch[r + 32] = (block[r + 32] & 0xFFFFFFFF00000000) | (block[r] & 0x00000000FFFFFFFF);
    }

    for (r = 0; r < 16; r++) {
        block[r] = (scratch[r] & 0xFFFF0000FFFF0000) | (scratch[r + 48] & 0x0000FFFF0000FFFF);
    }
    for (r = 0; r < 48; r++) {
        block[r + 16] = (scratch[r + 16] & 0xFFFF0000FFFF0000) | (scratch[r] & 0x0000FFFF0000FFFF);
    }

    for (r = 0; r < 8; r++) {
        scratch[r] = (block[r] & 0xFF00FF00FF00FF00) | (block[r + 56] & 0x00FF00FF00FF00FF);
    }
    for (r = 0; r < 56; r++) {
        scratch[r + 8] = (block[r + 8] & 0xFF00FF00FF00FF00) | (block[r] & 0x00FF00FF00FF00FF);
    }

    for (r = 0; r < 4; r++) {
        block[r] = (scratch[r] & 0xF0F0F0F0F0F0F0F0) | (scratch[r + 60] & 0x0F0F0F0F0F0F0F0F);
    }
    for (r = 0; r < 60; r++) {
        block[r + 4] = (scratch[r + 4] & 0xF0F0F0F0F0F0F0F0) | (scratch[r] & 0x0F0F0F0F0F0F0F0F);
    }

    for (r = 0; r < 2; r++) {
        scratch[r] = (block[r] & 0xCCCCCCCCCCCCCCCC) | (block[r + 62] & 0x3333333333333333);
    }
    for (r = 0; r < 62; r++) {
        scratch[r + 2] = (block[r + 2] & 0xCCCCCCCCCCCCCCCC) | (block[r] & 0x3333333333333333);
    }

    block[0] = (scratch[0] & 0xAAAAAAAAAAAAAAAA) | (scratch[63] & 0x5555555555555555);
    for (r = 0; r < 63; r++) {
        block[r + 1] = (scratch[r + 1] & 0xAAAAAAAAAAAAAAAA) | (scratch[r] & 0x5555555555555555);
    }

    scratch[0] = block[63];
    for (r = 0; r < 63; r++) {
        scratch[r + 1] = block[r];
    }
    
    // rotate row r left by r and set back to the destination in the matrix
    int word_offset = di / 64;
    for (int y = 0; y < 64; y++) {
        scratch[y] = __builtin_rotateleft64(scratch[y], y);
        img[(dj + y) * row_size + word_offset] = __builtin_bswap64(scratch[y]);
    }
}