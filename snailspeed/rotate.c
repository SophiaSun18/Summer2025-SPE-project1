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
// #include "my_utils.h"
#include <string.h>

uint64_t* restrict int64_img;
const int prefetch_amount = 10;
typedef uint64_t vec_64tx4 __attribute__((__vector_size__(32)));
const vec_64tx4 stay_masks[] = {
    {0xFFFFFFFF00000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFFFFFF00000000ULL},
    {0xFFFF0000FFFF0000ULL, 0xFFFF0000FFFF0000ULL, 0xFFFF0000FFFF0000ULL, 0xFFFF0000FFFF0000ULL},
    {0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL, 0xFF00FF00FF00FF00ULL},
    {0xF0F0F0F0F0F0F0F0ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xF0F0F0F0F0F0F0F0ULL}
};

// get, set, and rotate for block size = 64
inline __attribute__((always_inline)) void get_block_64(const bytes_t row_size, uint32_t i, uint32_t j, uint64_t block_dst[]) {

  for (int y = 0; y < 64; y++) {
    block_dst[y] = __builtin_bswap64(int64_img[(64 * j + y) * row_size + i]);
    __builtin_prefetch(int64_img+(64 * j + y) * row_size + i + prefetch_amount, 0, 0);
  }
}

inline __attribute__((always_inline)) void set_block_64(const bytes_t row_size, uint32_t di, uint32_t dj, uint64_t block_src[]) {

  for (int y = 0; y < 64; y++) {
    int64_img[(64 * dj + y) * row_size + di] = __builtin_bswap64(block_src[y]);
    __builtin_prefetch(int64_img+(64 * dj + y) * row_size + di + prefetch_amount, 0, 0);
  }
}

inline __attribute__((always_inline)) void get_and_set_block_64(const bytes_t row_size, uint32_t i, uint32_t j, uint64_t block[]) {

  for (int y = 0; y < 64; y++) {
    uint64_t tmp = __builtin_bswap64(int64_img[(64 * j + y) * row_size + i]);
    int64_img[(64 * j + y) * row_size + i] = __builtin_bswap64(block[y]);
    block[y] = tmp;
    __builtin_prefetch(int64_img+(64 * j + y) * row_size + i + prefetch_amount, 0, 1);
  }
}

inline __attribute__((always_inline)) int min(int a, int b) {
  return (a < b) ? a : b;
}

void rotate_block_64(uint64_t block[]) {

  // rotate row r left by r + 1
  for (int r = 0; r < 64; r++) {
      block[r] = __builtin_rotateleft64(block[r], r + 1);
  }

  // rotate column c down by c + 1
  uint64_t scratch[64];
  int r;

  vec_64tx4 block_vec[16], scratch_vec[16];
  int vec_size = sizeof(vec_64tx4) / sizeof(uint64_t); // 4

  // vectorization, load the block into vector
  for (r = 0; r < 64; r+=vec_size) {
      for (int e = 0; e < vec_size; e++) {
          block_vec[r / vec_size][e] = block[r + e];
      }
  }

  for (r = 0; r < 8; r++) {
      scratch_vec[r] = (block_vec[r] & stay_masks[0]) | (block_vec[r + 8] & ~stay_masks[0]);
      scratch_vec[r + 8] = (block_vec[r + 8] & stay_masks[0]) | (block_vec[r] & ~stay_masks[0]);
  }

  for (r = 0; r < 4; r++) {
      block_vec[r] = (scratch_vec[r] & stay_masks[1]) | (scratch_vec[r + 12] & ~stay_masks[1]);
  }
  for (r = 0; r < 12; r++) {
      block_vec[r + 4] = (scratch_vec[r + 4] & stay_masks[1]) | (scratch_vec[r] & ~stay_masks[1]);
  }

  for (r = 0; r < 2; r++) {
      scratch_vec[r] = (block_vec[r] & stay_masks[2]) | (block_vec[r + 14] & ~stay_masks[2]);
  }
  for (r = 0; r < 14; r++) {
      scratch_vec[r + 2] = (block_vec[r + 2] & stay_masks[2]) | (block_vec[r] & ~stay_masks[2]);
  }

  block_vec[0] = (scratch_vec[0] & stay_masks[3]) | (scratch_vec[15] & ~stay_masks[3]);
  for (r = 0; r < 15; r++) {
      block_vec[r + 1] = (scratch_vec[r + 1] & stay_masks[3]) | (scratch_vec[r] & ~stay_masks[3]);
  }

  // store the vec back to block as the following case is easier to handle in block
  for (r = 0; r < 64; r+=vec_size) {
      for (int e = 0; e < vec_size; e++) {
          block[r + e] = block_vec[r / vec_size][e];
      }
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

  // rotate row r left by r
  for (int r = 0; r < 64; r++) {
      block[r] = __builtin_rotateleft64(scratch[r], r);
  }

}

// Rotates a bit array clockwise 90 degrees.
//
// The bit array is of `N` by `N` bits where N is a multiple of 64
void rotate_bit_matrix(uint64_t *img, const bits_t N) {

  int64_img = img;
  const uint32_t row_size = N / 64;
  const int cache_block = 100;
  uint64_t block[cache_block*cache_block][64];

  // if odd case, set up different w_bound and handle the middle block
  if (row_size % 2 == 1) {
    int w_bound = row_size / 2;
    get_block_64(row_size, w_bound, w_bound, block[0]);
    rotate_block_64(block[0]);
    set_block_64(row_size, w_bound, w_bound, block[0]);
  }

  for (int i = 0; i < row_size/2; i+=cache_block) {
    for (int j = 0; j < (row_size+1)/2; j+=cache_block) {
      
      const uint32_t actual_block_i = min(cache_block, row_size/2-i);
      const uint32_t actual_block_j = min(cache_block, (row_size+1)/2-j);

      // fetch and rotate the 1st quadrant
      int w = j, h = i, tmp;
      for (int ki = 0; ki < actual_block_i; ki++) {
        for (int kj = 0; kj < actual_block_j; kj++) {
          get_block_64(row_size, w+kj, h+ki, block[ki*cache_block+kj]);
          rotate_block_64(block[ki*cache_block+kj]);
        }
      }

      // fetch and rotate the 2nd quadrant, save in the 1st quadrant
      tmp = w, w = row_size - h - 1, h = tmp;
      for (int kj = 0; kj < actual_block_j; kj++) {
        for (int ki = actual_block_i - 1; ki >= 0; ki--) {
          get_and_set_block_64(row_size, w-ki, h+kj, block[ki*cache_block+kj]);
          rotate_block_64(block[ki*cache_block+kj]);
        }
      }

      // fetch and rotate the 3rd quadrant, save in 2nd quadrant
      tmp = w, w = row_size - h - 1, h = tmp;
      for (int ki = actual_block_i - 1; ki >= 0; ki--) {
        for (int kj = actual_block_j - 1; kj >= 0; kj--) {
          get_and_set_block_64(row_size, w-kj, h-ki, block[ki*cache_block+kj]);
          rotate_block_64(block[ki*cache_block+kj]);
        }
      }

      // fetch and rotate the 4th quadrant, save in 3rd quadrant
      tmp = w, w = row_size - h - 1, h = tmp;
      for(int kj = actual_block_j - 1; kj >= 0; kj--) {
        for(int ki = 0; ki < actual_block_i; ki++) {
          get_and_set_block_64(row_size, w+ki, h-kj, block[ki*cache_block+kj]);
          rotate_block_64(block[ki*cache_block+kj]);
        }
      }

      // save the 4th quadrant in the 1st quadrant
      tmp = w, w = row_size - h - 1, h = tmp;
      for(int ki = 0; ki < actual_block_i; ki++) {
        for(int kj = 0; kj < actual_block_j; kj++) {
          set_block_64(row_size, w+kj, h+ki, block[ki*cache_block+kj]);
        }
      }

    }
  }
  return;
}