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
    
Compressing small amounts of data is an application that most data compression algorithms cannot address efficiently. Some of these algorithms, including LZW methods, become viable with more than 32 data values. The fixed bit coding method described in this paper is designed to compress and decompress 2 to 64 bytes as quickly as possible. By determining  compression ratios of selected data sets with Huffman coding, fixed bit coding is estimated overall to produce results that are close to Huffman coding, the optimal frequency-based algorithm. Benchmarking against QuickLZ shows the speed and compression tradeoff of using fixed bit coding implemented in a test bed that compresses an entire file. 

The basic method employed by fixed bit coding is well known as using only the number of bits required to encode the number of unique characters in a data set. This algorithm performs this task as efficiently as possible, in part by ending the algorithm early if the predetermined number of unique values is exceeded, but primarily because the algorithm requires very little analysis. Fast execution speed, especially for decoding, results from encoding the same number of bits for every data value. 

The fixed bit coding algorithm includes a text mode because English text is a common data format and a data type where small data sets cannot be compressed by fixed bit coding. The Alice in Wonderland text file alice29.txt from the Squash Compression Benchmark achieves 24.3% compression.

The algorithm also includes a single value mode where a single value repeats in at least 1/3 of the number of input values. In this case, any number of other unique values can occur while getting over 18% compression for 64 input values.

Although this paper does not address any specific application of fixed bit coding, its use for small data sets includes subsets of a data set where highly compressible bytes are known to exist. The high speed of scanning means that even when some sections of data cannot be compressed, the overhead for compressing smaller sets of data is very minimal and decode speed is extremely fast.

Fixed bit coding is implemented in the files at https://github.com/lsleonard/fixed-bit-coding. The fbc264 function compresses 2 to 64 values, and calls fbc25 for two to five values. Call fbc25 directly to avoid the call overhead. All functions are defined static and are included in the fbc.h header file. 

The execution of the fixed bit coding test bed requires an input file name, and optionally the block size (number of character values to compress with default of 64) and loop count (with default of 1). The input file of up to 20 Mbytes is read into memory. An array of unsigned long values is allocated to store whether data was compressed or not. This array and the block size are written to a file appended with fbc.cq. The compressed or original data is written to a file appended with .fbc. The compressed percentage is printed, then compressed blocks, based on the bits returned by blocks that did compress, and compression time and rate. The decompress routine fbc264d or fbc25d is called after reading in the .fbc.cq and .fbc data files. Decompression rate and time is printed.

When the macro GEN_STATS is defined in main.c, additional information about the data is printed, including number of uncompressed blocks and the percentage of encoded blocks by number of unique values.

As this algorithm is intended as a low-level tool for compression of small data sets, the implementation of how to manage compressed and uncompressed data is left for the application developer. For example, compressed data could be concatenated to save unused bits in the last byte of output. Also, the number of input values is not stored in the compressed data. The test bed does not attempt to compress the bits that represent whether compression occurred or not, although this data could be highly compressed in some cases. The results from running the test bed are similar to what you can expect in a memory-based usage of the function, although the overhead of maintaining the file structure increases execution time as the number of input values decreases.

Special handling of 2 to 5 values is done in the functions fbc25 and fbc25d to allow direct calls for these very small data sets. In addition to supporting output of one or two bytes for one unique value (all values the same), 2 and 3 values support compression when two nibbles define the bytes, and 4 or 5 values support 2 unique values.
