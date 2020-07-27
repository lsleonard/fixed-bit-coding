//
//  main.c
//  Fixed Bit Coding file-based test bed
//
//  Created by Stevan Leonard on 7/16/20.
//  Copyright © 2020 Oxford House Software. All rights reserved.
//

#include "fbc.h" // functions are defined static

// definitions for implementation of file-based test bed in main()
#include "string.h"
#include "time.h"
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

//#define GEN_STATS
#ifdef GEN_STATS
double fTotalOutBytes;
long int gCountUnableToCompress=0;
long int gCountAverageUniques=0;
long int gCountNibbles=0;
static long int gCountUniques[MAX_UNIQUES];
long int gnUniques=0;
#endif

#define MAX_FILE_SIZE 20000000
unsigned char inVal[MAX_FILE_SIZE]; // read entire file into memory
unsigned char outVal[MAX_FILE_SIZE]; // encode into memory
long int gCountBlocks;
long total_out_bytes;
long int nBytes_remaining;
unsigned long nBytes_to_compress;
long bytes_decompressed;
unsigned long start_inVal;
long compressedInBytes=0;
double compressedOutBytes=0;
unsigned long int gCompressedORnot;
unsigned long gCORN[MAX_FILE_SIZE/128+1]; // store compressed bit info in memory for speed, also output to file .cq
unsigned long gCORNindex;
unsigned long gCORNblocks;
unsigned long compressedBlockCount=0;

// -----------------------------------------------------------------------------------
int main(int argc, const char * argv[])
// -----------------------------------------------------------------------------------
{
    FILE *f_out;
    int nbout;
    FILE *f_input;
    FILE *f_compressedORnot; // bits that indicate whether compressed or not
    int intBlockSize=64;
    unsigned long nBytes=1;
    clock_t begin, end;
    double timeSpent=0;
    double minTimeSpent=100; // 100 seconds
    int loopCntForTime=1;
    int loopCnt=0;
    char fName[256];
    char cfName[256]; // for compressed or not
    
    if (argc < 2)
    {
        printf("fbc error: input file must be specified\n");
        return 14;
    }
    strcpy(fName, argv[1]);
    f_input = fopen(fName, "r");
    if (!f_input)
    {
        printf("fbc error: file not found: %s\n", fName);
        return 9;
    }
    fseek(f_input, 0, SEEK_END); // set to end of file
    if (ftell(f_input) > MAX_FILE_SIZE)
    {
        printf("fbc: file exceeds max size of %d\n", MAX_FILE_SIZE);
        return 10;
    }
    fseek(f_input, 0, SEEK_SET); // reset to beginning of file
    f_out = fopen(strcat(fName, ".fbc"), "w");
    nBytes = fread(&inVal, 1, MAX_FILE_SIZE , f_input);
    if (nBytes < 1)
        return 3;
    strcpy(cfName, fName);
    f_compressedORnot = fopen(strcat(cfName, ".cq"), "w");
    if (!f_compressedORnot)
        return 4;
    if (argc >= 3)
        sscanf(argv[2], "%d", &intBlockSize);
    if ((intBlockSize < MIN_FBC_BYTES) || (intBlockSize > MAX_FBC_BYTES))
    {
        printf("fbc error: block size must be from %d to %d\n", MIN_FBC_BYTES, MAX_FBC_BYTES);
        return 3;
    }
    unsigned char blockSize=intBlockSize;
    if (fwrite(&blockSize, 1, sizeof(blockSize), f_compressedORnot) < 0)
        return -13;
    if (argc >= 4)
        sscanf(argv[2], "%d", &loopCntForTime);
    if (loopCntForTime < 1 || loopCntForTime > 1000000)
        loopCntForTime = 1;

COMPRESS_TIMED_LOOP:
    begin = clock();
    nBytes_remaining = nBytes;
    total_out_bytes = 0;
    start_inVal = 0;
    gCORNindex = 0;
    gCORNblocks = 0;
#ifdef GEN_STATS
    fTotalOutBytes = 1.0; // block size
#endif
    while (nBytes_remaining > 0)
    {
        nBytes_to_compress = nBytes_remaining<intBlockSize ? nBytes_remaining : intBlockSize;
        if (nBytes_to_compress < intBlockSize)
        {
            // last bytes less than intBlockSize are output uncompressed
            memcpy(outVal+total_out_bytes, inVal+start_inVal, nBytes_to_compress);
            total_out_bytes += nBytes_to_compress;
#ifdef GEN_STATS
            fTotalOutBytes += nBytes_to_compress;
#endif
            gCompressedORnot <<= 1; // 0 indicates not compressed
            if ((++gCORNblocks & 0x3f) == 0)
            {
                // output this set of 64 bits and prep for next
                gCORN[gCORNindex++] = gCompressedORnot;
            }
            break; // done processing
        }
        gCountBlocks++; // global count of blocks processed
        if (intBlockSize < 6)
            nbout = fbc25(inVal+start_inVal, outVal+total_out_bytes, intBlockSize);
        else
            nbout = fbc264(inVal+start_inVal, outVal+total_out_bytes, intBlockSize);
        if (nbout < 0)
        {
            printf("Error from fbc %d: Number of values must be from 2 to 64\n", nbout);
            return -2;
        }
        else if (nbout == 0)
        {
            // unable to compress: output original vals and use gCompressedORnot to record
            memcpy(outVal+total_out_bytes, inVal+start_inVal, intBlockSize);
            total_out_bytes += intBlockSize;
            gCompressedORnot <<= 1; // 0 indicates not compressed
#ifdef GEN_STATS
            gCountUnableToCompress++;
            fTotalOutBytes += intBlockSize;
#endif
        }
        else
        {
            // compressed output
#ifdef GEN_STATS
            fTotalOutBytes += (float)nbout / 8.0;
            if (intBlockSize < 6)
            {
                // accumulate stats for 2-5 values
                if (nbout <= 10)
                {
                    gCountUniques[0]++; // single unique
                    gCountAverageUniques++; // single unique
                }
                else if (nbout == 12)
                    gCountNibbles++;
                else
                {
                    gCountAverageUniques += 2; // two uniques, not nibbles
                    gCountUniques[1]++;
                }
            }
            else
            {
                // examine first byte of output for unique count
                unsigned int nUniques=(outVal[total_out_bytes] >> 1) & 0xf;
                gCountUniques[nUniques]++; // uniques encoded as 0 to 15
                gCountAverageUniques += nUniques + 1;
                gnUniques++;
            }
#endif
            gCompressedORnot <<= 1;
            gCompressedORnot |= 1; // 1 indicates compressed
            compressedInBytes += intBlockSize;
            compressedOutBytes += (float)nbout / 8.0;
            compressedBlockCount++;
            total_out_bytes += nbout / 8;
            if (nbout & 7)
                total_out_bytes++; // add partial byte for byte boundary
        }
        nBytes_remaining -= nBytes_to_compress;
        start_inVal += nBytes_to_compress;
        if ((++gCORNblocks & 0x3f) == 0)
        {
            // output this set of 64 bits and prep for next
            gCORN[gCORNindex++] = gCompressedORnot;
        }
    }
    end = clock();
    timeSpent = (double)(end-begin) / (double)CLOCKS_PER_SEC;
    if (timeSpent < minTimeSpent)
        minTimeSpent = timeSpent;
    if (++loopCnt < loopCntForTime)
    {
        usleep(10); // sleep 10 us
        goto COMPRESS_TIMED_LOOP;
    }
    timeSpent = minTimeSpent;
    unsigned long nBytesWritten = fwrite(outVal, 1, total_out_bytes, f_out);
    if (nBytesWritten < 1)
        return 7;
    fclose(f_out);
    if (gCORNblocks & 0x3f)
    {
        // ouput final partial block of compressed or not
        gCompressedORnot <<= 64 - (gCORNblocks & 0x3f);
        gCORN[gCORNindex++] = gCompressedORnot;
    }
    for (int i=0; i<gCORNindex; i++)
        fwrite(&gCORN[i], sizeof(gCompressedORnot), 1, f_compressedORnot);
    fclose(f_compressedORnot);
    unsigned long gCORNbytes = gCORNindex * sizeof(gCompressedORnot);
    
    printf("Fixed Bit Coding   compressed byte output=%.2f%%   compressed blocks=%.2lf%%\n   time=%f sec.   %.0f bytes per second   inbytes=%lu   outbytes=%ld\n   outbytes/block=%.2f   block size=%d\n   loop count=%d   file=%s\n", (float)100*(1.0-(float)(total_out_bytes+gCORNbytes)/nBytes), (float)100*(1.0-(float)compressedOutBytes/(float)compressedInBytes),  minTimeSpent, (float)nBytes/minTimeSpent, nBytes, total_out_bytes+gCORNbytes, (float)(total_out_bytes+gCORNbytes)/nBytes*(float)intBlockSize, intBlockSize, loopCnt, fName);
#ifdef GEN_STATS
    long int compressedBlocks=gCountBlocks-gCountUnableToCompress;
    printf("   compressed bit output=%.2f%%   uncompressed blocks=%.2f%%\n   average # uniques=%.2f  1 unique=%.2f%%  2 nibbles=%.2f%%  2 u=%.2f%%  3 u=%.2f%%  4 u=%.2f%%  5 u=%.2f%%  6 u=%.2f%%  7 u=%.2f%%  8 u=%.2f%%  9 u=%.2f%%  10 u=%.2f%%  11 u=%.2f%%  12 u=%.2f%%  13 u=%.2f%%  14 u=%.2f%%  15 u=%.2f%%  16 u=%.2f%%\n", (1.0-(fTotalOutBytes+gCORNbytes)/(float)nBytes)*100,   (float)gCountUnableToCompress/(float)gCountBlocks*100,
        (float)gCountAverageUniques/compressedBlocks, (float)gCountUniques[0]/compressedBlocks*100, (float)gCountNibbles/gCountBlocks*100, (float)gCountUniques[1]/compressedBlocks *100, (float)gCountUniques[2]/compressedBlocks *100, (float)gCountUniques[3]/compressedBlocks *100, (float)gCountUniques[4]/compressedBlocks *100, (float)gCountUniques[5]/compressedBlocks *100, (float)gCountUniques[6]/compressedBlocks *100, (float)gCountUniques[7]/compressedBlocks *100, (float)gCountUniques[8]/compressedBlocks *100, (float)gCountUniques[9]/compressedBlocks *100, (float)gCountUniques[10]/compressedBlocks *100, (float)gCountUniques[11]/compressedBlocks *100, (float)gCountUniques[12]/compressedBlocks *100, (float)gCountUniques[13]/compressedBlocks *100, (float)gCountUniques[14]/compressedBlocks *100, (float)gCountUniques[15]/compressedBlocks *100);
#endif
    

    // decompress output ------------------------------------
    f_input = fopen(fName, "r");
    f_out = fopen(strcat(fName, "d"), "w");
    nBytes = fread(&inVal, 1, total_out_bytes , f_input);
    if (nBytes < 1)
        return 5;
    loopCnt = 0;
    minTimeSpent = 60; // 60 seconds
    gCountBlocks = 0;
    int bytes_processed;
DECOMPRESS_TIMED_LOOP:
    gCORNblocks = 0;
    gCORNindex = 0; // point at first block of compressed bits
    gCompressedORnot = gCORN[gCORNindex++]; // get first block
    nBytes_remaining = nBytes;
    total_out_bytes = 0;
    start_inVal = 0;
    begin = clock();
    while (nBytes_remaining > 0)
    {
        // check whether to decompress or copy uncompressed data to output
        gCountBlocks++;
        if ((gCompressedORnot & 0x8000000000000000) == 0)
        {
            // uncompressed input block
            long outBytes;
            if (nBytes_remaining < intBlockSize)
                outBytes = nBytes_remaining; // last block in file
            else
                outBytes = intBlockSize;
            memcpy(outVal+total_out_bytes, inVal+start_inVal, outBytes);
            total_out_bytes += outBytes;
            start_inVal += outBytes;
            nBytes_remaining -= outBytes;
        }
        else
        {
            // compressed input block
            if (intBlockSize < 6)
                bytes_decompressed = fbc25d(inVal+start_inVal, outVal+total_out_bytes, intBlockSize, &bytes_processed);
            else
                bytes_decompressed = fbc264d(inVal+start_inVal, outVal+total_out_bytes, intBlockSize, &bytes_processed);
            if (bytes_decompressed < 1)
            {
                fwrite(outVal, 1, total_out_bytes, f_out);
                fclose(f_out);
                fclose(f_compressedORnot);
                printf("error from ibe264d\n");
                goto COMPRESS_DATA;
            }
            total_out_bytes += bytes_decompressed;
            start_inVal += bytes_processed;
            nBytes_remaining -= bytes_processed;
        }
        if (++gCORNblocks & 0x3f)
        {
            gCompressedORnot <<= 1;
        }
        else
        {
            //unsigned long cBytes;
            // get the next block of compressed or not bits
            gCompressedORnot = gCORN[gCORNindex++];
            //cBytes=fread(&gCompressedORnot, sizeof(gCompressedORnot), 1, f_compressedORnot);
            //if (ferror(f_compressedORnot))
            //    return 6;
        }
    }
    end = clock();
    timeSpent = (double)(end-begin) / (double)CLOCKS_PER_SEC;
    if (timeSpent < minTimeSpent)
        minTimeSpent = timeSpent;
    if (++loopCnt < loopCntForTime)
    {
        usleep(10); // sleep 10 us
        goto DECOMPRESS_TIMED_LOOP;
    }
    timeSpent = minTimeSpent;

    fwrite(outVal, 1, total_out_bytes, f_out);
    fclose(f_out);
    //fclose(f_compressedORnot);
    printf("fbc264d decompression bytes per second=%.0lf   time=%f sec.\ninbytes=%lu   outbytes=%ld\n", (float)total_out_bytes/(float)minTimeSpent, minTimeSpent, nBytes, total_out_bytes);
    // compare two files someday
COMPRESS_DATA:
    return 0;
}