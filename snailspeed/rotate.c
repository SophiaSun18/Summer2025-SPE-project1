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


// Rotates a bit array clockwise 90 degrees.
//
// The bit array is of `N` by `N` bits where N is a multiple of 64
void rotate_bit_matrix(uint8_t *img, const bits_t N) {

  uint64_t *int64_img = (uint64_t *) img;
  const uint32_t block_size = 64;
  const uint32_t row_size = N / 64;

  uint64_t tmp_block[64], save_block[64];
  uint32_t h_bound = N/2, w_bound = N/2;

  // if odd case, set up different w_bound and handle the middle block
  if (row_size % 2 != 0) {
    w_bound = (row_size - 1) * 32;
    get_block_64(int64_img, row_size, w_bound, w_bound, tmp_block);
    rotate_and_set_block_64(int64_img, row_size, w_bound, w_bound, tmp_block);
  }

  uint32_t w, h;
  for (h = 0; h < h_bound; h += block_size) {
    for (w = 0; w < w_bound; w += block_size) {
      
      uint32_t i = w, j = h, ni = N - i - block_size, nj = N - j - block_size;

      get_block_64(int64_img, row_size, i, j, tmp_block);

      get_block_64(int64_img, row_size, nj, i, save_block);
      rotate_and_set_block_64(int64_img, row_size, nj, i, tmp_block);

      get_block_64(int64_img, row_size, ni, nj, tmp_block);
      rotate_and_set_block_64(int64_img, row_size, ni, nj, save_block);

      get_block_64(int64_img, row_size, j, ni, save_block);
      rotate_and_set_block_64(int64_img, row_size, j, ni, tmp_block);

      rotate_and_set_block_64(int64_img, row_size, i, j, save_block);
    }
  }

  return;
}