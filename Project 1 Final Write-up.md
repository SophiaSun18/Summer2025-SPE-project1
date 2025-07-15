Project 1 Final Write-Up

**Design Overview**

The beta design follows the instructions in the handout. The fundamental idea is to divide the matrix into 4 quadrants, collect 64*64-bits blocks from each quadrant, rotate them 90-degree clockwise, then write them back to the matrix at the location after rotation, until the entire matrix is rotated. Each block consists of 64 uint64_t words, which is the basic element of the matrix rotation and all utility functions, replacing the bit-by-bit rotation in the starter code.

The in-block rotation is done through implementing the row-column-row algorithim as described in the handout, processing columns and rows together. Specifically, a divide-and-conquer solution combined with bitwise operations is applied in the column rotation, which outperforms the naive solution that requires using nested loops to access each column in the block.

**Final Update**

The main optimization in the final release is the 2-layer tiling. Each quadrant of the matrix is divided into tiles, and each tile contains many 64*64-bits blocks. The main for loop goes through each tile in row-major order, then each block inside the current tile is rotated in row-major order. This access pattern of blocks is more cache-friendly and has brought the major improvement of tiers among all final optimizations.

Other techniques, such as builtin intrinsics and prefetching are applied in the final release and have demonstrated obvious improvement on the performance. The column rotation in rcr algorithim is also further optimized by removing complicated % operations and replacing with multiple for loops.

**State of Completeness**

The beta release reaches about tier 25-26 in the telerun performance check, and the final release reaches tier 39. Both releases are able to pass the correctness check without any perceivable system failure.

**Additional Information**

At the early stage of beta, the progress has been slow due to insufficient understanding of the matrix structure and the available techniques. Some early state immature ideas and implementations have been kept in unused\_util.c with comments to keep a record of the early works.

A main failed optimization attempt in the final stage is applying vectorization on the column rotation. As the column rotation includes many logically simple for loops, I tried to manually vectorize them to handle 4 uint64_t words in 1 operation instead of 1 by 1, but it didn't actually improve the performance. The reason might be that the for loops are already straight-forward in logic and the compiler can eaily vectorize them without my hard-coded vectorization.

Another unsuccessful attempt in column rotation is loop unrolling using #pragma unroll. No perceivable performance optimization in the telerun tier check. The reason is very likely the same as above that the loop logics are simple and the loops are relatively short (all < 64 iterations), so the compilers can automatically unroll the loops and doesn't need extra instruction about it.

**Acknowledgement**

Some key algorithms in this beta design (such as r-c-r and divide-and-conquer) are derived from the recitation slides, and I consulted other student's codes (dougyo and saraiev, emichen and mqwang) during the final optimization to get new ideas and hints. Special thanks to dougyo and saraiev's group as I reproduced and analyzed their code structure to get a more in-depth understanding on how does my code fall behind and how can I further optimize it.