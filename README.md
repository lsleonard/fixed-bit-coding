# fixed-bit-coding
High-speed, lossless data compression for 2 to 64 values

To run the fixed bit coding test bed program after compiling main.c with a target of fbc:

    fbc input-file block-size loop-count

      input-file is the filename of the file to compress

      block-size is a value from 2 to 64, number of bytes to compress at a time

      loop-count is optional number of loops in memory to repeat the compression for average run time

The program generates these files:

    input-file.fbc contains the compressed data

    input-file.fbc.cq contains the block size and a bit that indicates whether compression succeeded or not for each input block

    input-file.fbcd contains the decompressed data
    
Compressing small amounts of data is an application that most data compression algorithms cannot address. Most of these algorithms, including LZW methods, become viable with more than 32 data values. The fixed bit coding method described in this paper is designed to compress and decompress 2 to 64 bytes as quickly as possible. By comparing  compression ratios, fixed bit coding is estimated overall to produce compression results within 10% of Huffman coding, the optimal frequency-based algorithm. 

The basic method employed by fixed bit encoding is well known as using only the number of bits required to encode the number of unique characters in a data set. This algorithm performs this task as efficiently as possible, in part by ending the algorithm early if the predetermined number of unique values is exceeded, but primarily because the algorithm requires very little analysis. 

Many programmatic data sets are very small and would be suitable targets for data compression with an algorithm such as fixed bit coding. Possible applications of compression of small data sets include reducing memory storage requirements, such as an adjunct to a memory manager; and data compression on the fly, such as in embedded software during network data transfer. 

Fixed bit coding is implemented in the files at https://github.com/lsleonard/fixed-bit-coding. The fbc264 function compresses 2 to 64 values, and calls fbc25 for two to five values. Call fbc25 directly to avoid the call overhead. All functions are defined static and are included in the fbc.h header file. The execution of the fbc test bed requires an input file name, and optionally the block size (number of character values to compress with default of 64) and loop count (with default of 1). The input file of up to 20 Mbytes is read into memory. An array of unsigned long values is allocated to store whether data was compressed or not. This array and the block size are written to a file appended with fbc.cq. The compressed or original data is written to a file appended with .fbc. The compressed percentage is printed, then compressed blocks, based on the bits returned by blocks that did compress, and compression time and rate. The decompress routine fbc264d or fbc25d is called after reading in the .fbc.cq and .fbc data files. Decompression rate and time is printed.

When the COMPRESS_1_PERCENT macro is defined in fbc.h, the unique value limit is set based on producing a minimum of 1% compression rather than the default of 25% compression. Because the number of unique values is limited to 16, the results from this macro can differ from 25% for 34 or fewer input values.

When the macro GEN_STATS is defined in main.c, additional information about the data is printed, including number of uncompressed blocks and the percentage of encoded blocks by number of unique values.

As this algorithm is intended as a low-level tool for compression of small data sets, the implementation of how to manage compressed and uncompressed data is left for the application developer. For example, compressed data could be concatenated to save unused bits in the last byte of output. Also, the number of input values is not stored in the compressed data. The test bed does not attempt to compress the bits that represent whether compression occurred or not, although this data could be highly compressed in some cases. The results from running the test bed are similar to what you can expect in a memory-based usage of the function, although the overhead of maintaining the file structure increases execution time as the number of input values decreases.

Special handling of 2 to 5 values is done in the functions fbc25 and fbc25d to allow direct calls for these very small data sets. In addition to supporting output of one or two bytes for one unique value (all values the same), 2 and 3 values support compression when two nibbles define the bytes, and 4 or 5 values support 2 unique values.
