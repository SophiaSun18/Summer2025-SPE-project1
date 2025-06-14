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


// get, set, and rotate for block size = 32
void get_block_32(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint32_t block_dst[]) {

    for (uint32_t y = 0; y < block_size; y++) {
        uint8_t *src = img + (j + y) * row_size + i / 8;
        block_dst[y] = ((uint32_t)src[0] << 24) |
                        ((uint32_t)src[1] << 16) |
                        ((uint32_t)src[2] << 8)  |
                        ((uint32_t)src[3]);
    }
}

void set_block_32(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint32_t block_src[]) {

    for (uint32_t y = 0; y < block_size; y++) {
        uint8_t *dst = img + (j + y) * row_size + i / 8;
        uint32_t val = block_src[y];
        dst[3] = val & 0xFF;
        dst[2] = (val >> 8) & 0xFF;
        dst[1] = (val >> 16) & 0xFF;
        dst[0] = (val >> 24) & 0xFF;
    }
}

void rotate_row_32(uint32_t *row, uint32_t block_size, uint32_t shift_left) {
    
    if (shift_left == 0 || shift_left >= 32) {  // shift >= block size is UB
        return;
    }

    // shift left to get the left half, shift right to get the wrap around
    uint32_t valid_row = *row;
    valid_row = ((valid_row << shift_left) | (valid_row >> (block_size - shift_left)));
    *row = valid_row;

}

void rotate_block_32(uint32_t block_size, uint32_t block[]) {

    uint32_t scratch[block_size];

    // rotate row r left by r + 1
    for (int r = 0; r < block_size; r++) {
        rotate_row_32(&block[r], block_size, r + 1);
    }

    // rotate column c down by c + 1
    // 1. column 16-31 down by 16 / up by 16
    for (int r = 0; r < 32; r++)
        scratch[r] = (block[r] & 0xFFFF0000) | (block[(r + 16) % 32] & 0x0000FFFF);

    // 2. column 8-15, 24-31 down by 8 / up by 24
    for (int r = 0; r < 32; r++)
        block[r] = (scratch[r] & 0xFF00FF00) | (scratch[(r + 24) % 32] & 0x00FF00FF);

    // 3. column 4-7, 12-15, 20-23... down by 4 / up by 28
    for (int r = 0; r < 32; r++)
        scratch[r] = (block[r] & 0xF0F0F0F0) | (block[(r + 28) % 32] & 0x0F0F0F0F);

    // 4. column 2-3, 6-7, 10-11... down by 2 / up by 30
    for (int r = 0; r < 32; r++)
        block[r] = (scratch[r] & 0xCCCCCCCC) | (scratch[(r + 30) % 32] & 0x33333333);

    // 5. odd columns down by 1 / up by 31
    for (int r = 0; r < 32; r++)
        scratch[r] = (block[r] & 0xAAAAAAAA) | (block[(r + 31) % 32] & 0x55555555);

    // 6. all columns down by 1
    for (int r = 0; r < 32; r++)
        block[r] = (scratch[r] & 0x00000000) | (scratch[(r + 31) % 32] & 0xFFFFFFFF);

    // rotate row r left by r
    for (int r = 0; r < block_size; r++) {
        rotate_row_32(&block[r], block_size, r);
    }
    
}

// get, set, and rotate for block size = 64
void get_block_64(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_dst[]) {

    for (uint32_t y = 0; y < block_size; y++) {
        uint8_t *src = img + (j + y) * row_size + i / 8;
        block_dst[y] = ((uint64_t)src[0] << 56) |
                        ((uint64_t)src[1] << 48) |
                        ((uint64_t)src[2] << 40) |
                        ((uint64_t)src[3] << 32) |
                        ((uint64_t)src[4] << 24) |
                        ((uint64_t)src[5] << 16) |
                        ((uint64_t)src[6] << 8)  |
                        ((uint64_t)src[7]);
    }
}

void set_block_64(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_src[]) {

    for (uint32_t y = 0; y < block_size; y++) {
        uint8_t *dst = img + (j + y) * row_size + i / 8;
        uint64_t val = block_src[y];
        dst[0] = (val >> 56) & 0xFF;
        dst[1] = (val >> 48) & 0xFF;
        dst[2] = (val >> 40) & 0xFF;
        dst[3] = (val >> 32) & 0xFF;
        dst[4] = (val >> 24) & 0xFF;
        dst[5] = (val >> 16) & 0xFF;
        dst[6] = (val >> 8)  & 0xFF;
        dst[7] = val & 0xFF;
    }
}

void rotate_row_64(uint64_t *row, uint32_t block_size, uint32_t shift_left) {
    
    if (shift_left == 0 || shift_left >= block_size) {  // shift >= block size is UB
        return;
    }

    uint64_t valid_row = *row;
    valid_row = ((valid_row << shift_left) | (valid_row >> (block_size - shift_left)));
    *row = valid_row;
}

void rotate_block_64(uint32_t block_size, uint64_t block[]) {

    // rotate row r left by r + 1
    for (int r = 0; r < block_size; r++) {
        if (r + 1 < block_size) {
            block[r] = ((block[r] << (r + 1)) | (block[r] >> (block_size - (r + 1))));
        }
    }

    uint64_t scratch[block_size];
    // rotate column c down by c + 1
    for (int r = 0; r < 64; r++) {
        scratch[r] = (block[r] & 0xFFFFFFFF00000000ULL) | (block[(r + 32) % 64] & 0x00000000FFFFFFFFULL);
    }
    for (int r = 0; r < 64; r++) {
        block[r] = (scratch[r] & 0xFFFF0000FFFF0000ULL) | (scratch[(r + 48) % 64] & 0x0000FFFF0000FFFFULL);
    }
    for (int r = 0; r < 64; r++) {
        scratch[r] = (block[r] & 0xFF00FF00FF00FF00ULL) | (block[(r + 56) % 64] & 0x00FF00FF00FF00FFULL);
    }
    for (int r = 0; r < 64; r++) {
        block[r] = (scratch[r] & 0xF0F0F0F0F0F0F0F0ULL) | (scratch[(r + 60) % 64] & 0x0F0F0F0F0F0F0F0FULL);
    }
    for (int r = 0; r < 64; r++) {
        scratch[r] = (block[r] & 0xCCCCCCCCCCCCCCCCULL) | (block[(r + 62) % 64] & 0x3333333333333333ULL);
    }
    for (int r = 0; r < 64; r++) {
        block[r] = (scratch[r] & 0xAAAAAAAAAAAAAAAAULL) | (scratch[(r + 63) % 64] & 0x5555555555555555ULL);
    }
    for (int r = 0; r < 64; r++) {
        scratch[r] = block[(r + 63) % 64];
    }
    memcpy(block, scratch, block_size * sizeof(uint64_t));
        
    // rotate row r left by r
    for (int r = 0; r < block_size; r++) {
        if (r != 0 && r < block_size) {
            block[r] = ((block[r] << r) | (block[r] >> (block_size - r)));
        }
    } 

}