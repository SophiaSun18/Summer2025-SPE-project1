Project 1 Beta Write-Up

**Design Overview**

The beta design introduces the following improvements based on the starter code:

1. The shift from bit rotation to block rotation. In the starter code, the matrix rotation is done through processing 4 bits at a time, each rotating to the location of another in a cycle, which is highly inefficient when applying to individual bits in the matrix due to high loop overhead and frequent memory access. By replacing get\_bit() and set\_bit() with get\_block() and set\_block(), the beta design now processes 4 blocks of bits at a time, with each block stored as a vector of words(uint32\_t or uint64\_t), which greatly reduces the loop overhead and allows the application of bitwise operations to better utilize the cache and further improve the performance.

2. The row-column-row algorithm in in-block rotation. After a block is collected from the matrix, the r-c-r algorithm is used to achieve the correct and efficient in-block rotation. Instead of taking each bit in the block and looking for its destination in the block after rotation, each row and column of bits are processed together using bit shifting and wrapping-around techniques, so there’s no need to go back to the same individual bit processing methods as the starter code.

3. The divide-and-conquer solution for column rotation. The column rotation in the r-c-r algorithm can be done in two ways. The first one is the naive approach, which loops through each row to collect the i-th bit for i-th column into a word, rotates the word, then loops through each row again to put the new i-th bit back, which takes O(N^2). The second one is the divide-and-conquer approach, which applies bitwise operations (such as masking and bit hacking) in multiple stages to progressively manipulate bits and rotate columns until the ideal result is reached. Each stage takes O(N) time to iterate through the matrix, and the number of stages is O(log N), which takes O(N logN) as a result and greatly improves the performance. A diagram has been included at the end of the write up to visualize this approach.

**State of Completeness**

Currently, the beta design is able to pass the correctness check, and reaches around tier 25-26 in the telerun performance check.

A tricky aspect in the design is when the matrix contains an odd number of blocks per row, which may cause duplicate rotation on the middle rows and columns. To handle this issue, a variable w\_bound has been set to define the column boundary of the for loop differently for odd and even number of blocks per row, and an extra if statement has been added at the end of the program to ensure than the middle block of the entire matrix won’t be missed when the number of blocks per row is an odd value.

**Additional Information**

At the early stage of this project, the progress has been slow due to insufficient understanding of the matrix structure and the available techniques. Some early state immature ideas and implementations have been kept in unused\_util.c with comments to keep a record of the early works. 

**Acknowledgement**

Some key algorithms in this beta design (such as r-c-r and divide-and-conquer) are derived from the recitation slides. I also used ChatGPT as a tool to help me understand some key concepts and to discuss and revise ideas during the development process.

