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
  // Get the number of bytes per row in `img`
  const uint32_t row_size = bits_to_bytes(N);
  const uint32_t block_size = 64;
  uint32_t num_block = N / block_size;

  // If the number of block per row is an odd value, set w_bound to be less than N/2 to avoid duplicate rotation
  uint32_t w_bound = (num_block % 2 == 0) ? (num_block * 32) : (num_block - 1) * 32;

  uint64_t block0[block_size];
  uint64_t block1[block_size];
  uint64_t block2[block_size];
  uint64_t block3[block_size];

  uint32_t w, h;
  for (h = 0; h < N/2; h += block_size) {
    for (w = 0; w < w_bound; w += block_size) {

      uint32_t i = w, j = h, ni = N - i - block_size, nj = N - j - block_size;

      get_block_64(img, row_size, i, j, block_size, block0);
      get_block_64(img, row_size, nj, i, block_size, block1);
      get_block_64(img, row_size, ni, nj, block_size, block2);
      get_block_64(img, row_size, j, ni, block_size, block3);

      rotate_block_64(block_size, block0);
      rotate_block_64(block_size, block1);
      rotate_block_64(block_size, block2);
      rotate_block_64(block_size, block3);

      set_block_64(img, row_size, nj, i, block_size, block0);
      set_block_64(img, row_size, ni, nj, block_size, block1);
      set_block_64(img, row_size, j, ni, block_size, block2);
      set_block_64(img, row_size, i, j, block_size, block3);

    }
  }

  // if odd case, handle the middle block
  if (num_block % 2 != 0) {
    get_block_64(img, row_size, w_bound, w_bound, block_size, block0);
    rotate_block_64(block_size, block0);
    set_block_64(img, row_size, w_bound, w_bound, block_size, block0);
  }

  return;
}