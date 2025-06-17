#include "../utils/utils.h"
#include <string.h>

// same approach as get_bit(), get a full byte instead of 1 bit in the byte
uint8_t get_byte(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j) {
    uint64_t byte_offset = j * row_size + i;
    uint8_t *img_byte = img + byte_offset;
    return *img_byte;
}

// same approach as set_bit(), set a full byte instead of 1 bit in the byte
void set_byte(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint8_t value) {
    uint64_t byte_offset = j * row_size + i;
    uint8_t *img_byte = img + byte_offset;
    *img_byte = value;
}

// collect all bits into a block using get_bit()
void get_block(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint8_t *block_dst) {
    for (uint32_t x = 0; x < block_size; x++) {
        for (uint32_t y = 0; y < block_size; y++) {
          block_dst[y * block_size + x] = get_bit(img, row_size, i + x, j + y);
        }
    }
}

// set a block using set_bit()
void set_block(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint8_t *block_src) {
    for (uint32_t x = 0; x < block_size; x++) {
        for (uint32_t y = 0; y < block_size; y++) {
          set_bit(img, row_size, i + x, j + y, block_src[y * block_size + x]);
        }
    }
}

// rotate a block by going through each bit in the old block and put the bit to the new location in the new block
void rotate_block(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint8_t *tmp_block, uint8_t *save_block) {

    get_block(img, row_size, i, j, block_size, tmp_block);
    for (uint32_t x = 0; x < block_size; x++) {
        for (uint32_t y = 0; y < block_size; y++) {
            // new x = block size - y - 1, new y = x
            save_block[x * block_size + (block_size - y - 1)] = tmp_block[y * block_size + x];
        }
    }
    set_block(img, row_size, i, j, block_size, save_block);
}

// temporary idea: rotate blocks recursively
// divide the matrix into 4 top-level blocks, rotate, then for each block, divide into 4 sub-blocks and rotate again until block size reaches 1
// stopped developing as it takes too much redundant work and recursion is hard to debug and optimize
void rotate_block_new(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t N, uint8_t *tmp_block, uint8_t *save_block) {

    // i, j: starting bit of the entire rotation, N: total # of bits in the matrix
    // block size: split the block into 4, so size = N/2

    if (N <= 1) return;

    uint32_t block_size = N / 2;

    get_block(img, row_size, i, j, block_size, tmp_block);

    for (uint32_t quadrant = 0; quadrant < 4; quadrant++) {
        uint32_t next_i = N - j - block_size, next_j = i;
        memcpy(save_block, tmp_block, block_size * block_size);

        get_block(img, row_size, next_i, next_j, block_size, tmp_block);
        set_block(img, row_size, next_i, next_j, block_size, save_block);

        i = next_i;
        j = next_j;
    }

    rotate_block_new(img, row_size, i, j, block_size, tmp_block, save_block);  // top left
    rotate_block_new(img, row_size, i + block_size, j, block_size, tmp_block, save_block);  // top right
    rotate_block_new(img, row_size, i, j + block_size, block_size, tmp_block, save_block);  // bottom left
    rotate_block_new(img, row_size, i + block_size, j + block_size, block_size, tmp_block, save_block);  // bottom right
}

// in order to solve the case where some matrix size N can't be divided into even # of blocks by block size = 64
// using N = 192 as the first-step, do block size = N / 4 = 48
// get a block with the assumption that the block size may not match exact word length of 32 or 64 but still a multiple of 8 to use get_byte()
void get_block_48(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_dst[][(block_size + 63) / 64]) {

    uint32_t words_per_row = (block_size + 63) / 64;
    for (uint32_t y = 0; y < block_size; y++) {
        for (uint32_t x = 0; x < words_per_row; x++) {
            uint64_t word = 0;
            // each row contains 1 64-bit word
            // each 64-bit word needs 8 bytes to fill up
            // to keep track of the # of bits collected as full words already, 64 * x
            for (uint32_t b = 0; b < 8; b++) {
                uint32_t bit_index = x * 64 + b * 8;
                if (bit_index >= block_size) {
                    break;
                }
                uint32_t pixel_x = i + bit_index;
                uint32_t pixel_y = j + y;
                uint8_t byte_val = get_byte(img, row_size, pixel_x, pixel_y);
                word |= ((uint64_t)byte_val) << (b * 8);
            }
            block_dst[y][x] = word;
        }
    }
}

// set a block with the same assumption to use set_byte()
void set_block_48(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_src[][(block_size + 63) / 64]) {

    uint32_t words_per_row = (block_size + 63) / 64;
    for (uint32_t y = 0; y < block_size; y++) {
        for (uint32_t x = 0; x < words_per_row; x++) {
            uint64_t word = block_src[y][x];
            for (uint32_t b = 0; b < 8; b++) {
                uint32_t bit_index = x * 64 + b * 8;
                if (bit_index >= block_size) {
                    break;
                }
                uint8_t byte = (word >> (8 * b)) & 0xFF;
                set_byte(img, row_size, i + bit_index, j + y, byte);
            }
        }
    }
}

// get a 64-bit block using get_byte()
void get_block_64_old(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_dst[][block_size / 64]) {

    uint32_t words_per_row = block_size / 64;
    for (uint32_t y = 0; y < block_size; y++) {
        for (uint32_t x = 0; x < words_per_row; x++) {
            uint64_t word = 0;
            for (uint32_t b = 0; b < 8; b++) {
                uint8_t byte_val = get_byte(img, row_size, i + x * 8 + b, j + y);
                word |= ((uint64_t)byte_val) << (8 * b);
            }
            block_dst[y][x] = word;
        }
    }
}

// set a 64-bit block using set_byte()
void set_block_64_old(uint8_t *img, const bytes_t row_size, uint32_t i, uint32_t j, uint32_t block_size, uint64_t block_src[][block_size / 64]) {

    uint32_t words_per_row = block_size / 64;
    for (uint32_t y = 0; y < block_size; y++) {
        for (uint32_t x = 0; x < words_per_row; x++) {
            uint64_t word = block_src[y][x];
            for (uint32_t b = 0; b < 8; b++) {
                uint8_t byte = (word >> (8 * b)) & 0xFF;
                set_byte(img, row_size, i / 8 + x * 8 + b, j + y, byte);
            }
        }
    }
}

// rotate a 32-bit block using the naive appraoch for rotate column
void rotate_block_32(uint32_t block_size, uint32_t block[]) {

    uint32_t scratch[block_size];

    // rotate row r left by r + 1
    for (int r = 0; r < block_size; r++) {
        rotate_row_32(&block[r], block_size, r + 1);
    }

    // rotate column c down by c + 1
   uint32_t mask = 1U << (block_size - 1);     // get the left most digit
   for (int c = 0; c < block_size; c++) {
       uint32_t col = 0;
       for (int j = 0; j < block_size; j++) {
           col = col | (((block[j][0] << c) & mask) >> j);
       }
       col = (col >> (c + 1)) | (col << (block_size - c - 1));
       for (int j = 0; j < block_size; j++) {
           block[j][0] = (block[j][0] & ~(mask >> c)) | (((col << j) >> c) & (mask >> c));
       }
   }

    // rotate row r left by r
    for (int r = 0; r < block_size; r++) {
        rotate_row_32(&block[r], block_size, r);
    }
    
}

// rotate a 64-bit block using the naive appraoch for rotate column
void rotate_block_64_old(uint32_t block_size, uint64_t block[]) {

    // rotate row r left by r + 1
    for (int r = 0; r < block_size; r++) {
        if (r + 1 < block_size) {
            block[r] = ((block[r] << (r + 1)) | (block[r] >> (block_size - (r + 1))));
        }
    }

    uint64_t mask = 1ULL << (block_size - 1);     // get the left most digit
   // rotate column c down by c + 1
   for (int c = 0; c < block_size; c++) {
       uint64_t col = 0;
       for (int j = 0; j < block_size; j++) {
           col = col | (((block[j] << c) & mask) >> j);
       }
       col = (col >> (c + 1)) | (col << (block_size - c - 1));
       for (int j = 0; j < block_size; j++) {
           block[j] = (block[j] & ~(mask >> c)) | (((col << j) >> c) & (mask >> c));
       }
   }

    // rotate row r left by r
    for (int r = 0; r < block_size; r++) {
        if (r != 0 && r < block_size) {
            block[r] = ((block[r] << r) | (block[r] >> (block_size - r)));
        }
    } 

}

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
