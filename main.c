//
//  main.c
//  Fixed Bit Coding file-based test bed
//
//  Created by Stevan Leonard on 7/16/20.
//  Copyright © 2020 Oxford House Software. All rights reserved.
//

#include "fbc.h" // functions are defined static

// definitions for implementation of file-based test bed in main()
#include "stdint.h"
#include "string.h"
#include "time.h"
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

//#define GEN_STATS
#ifdef GEN_STATS
static double fTotalOutBytes;
static uint64_t gCountUnableToCompress;
static uint64_t gCountAverageUniques;
static uint64_t gCountNibbles;
static uint64_t gCountUniques[MAX_UNIQUES];
static uint32_t gTextModeCnt;
static uint32_t gSingleValueModeCnt;
#endif

#define MAX_FILE_SIZE 20000000
unsigned char inVal[MAX_FILE_SIZE]; // read entire file into memory
unsigned char outVal[MAX_FILE_SIZE]; // encode into memory
uint64_t gCountBlocks;
uint64_t total_out_bytes;
int64_t nBytes_remaining;
uint64_t nBytes_to_compress;
int64_t bytes_decompressed;
uint64_t start_inVal;
int64_t compressedInBytes=0;
double compressedOutBytes=0;
uint64_t  gCompressedORnot;
uint64_t gCORN[MAX_FILE_SIZE/128+1]; // store compressed bit info in memory for speed, also output to file .cq
uint32_t gCORNindex;
uint64_t gCORNblocks;
uint64_t compressedBlockCount=0;

static uint32_t top16[256];
struct top16_s {
    unsigned char val;
    uint32_t count;
} top16_struct[256];

static int qCompare(const void *v1, const void *v2)
{
    if (((struct top16_s *)v1)->count > ((struct top16_s *)v2)->count)
        return -1;
    if (((struct top16_s *)v1)->count < ((struct top16_s *)v2)->count)
        return 1;
    return 0;
} // end qCompare

void countTop16(int64_t nValues)
{
    while (--nValues > 0)
    {
        top16[inVal[nValues]]++;
    }
    for (int i=0; i<256; i++)
    {
        top16_struct[i].count = top16[i];
        top16_struct[i].val = (unsigned char)i;
    }
    qsort(&top16_struct, 256, sizeof(struct top16_struct*), qCompare);
}

// -----------------------------------------------------------------------------------
int main(int argc, const char * argv[])
// -----------------------------------------------------------------------------------
{
    FILE *f_out;
    int32_t nbout;
    FILE *f_input;
    FILE *f_compressedORnot; // bits that indicate whether compressed or not
    uint32_t uintBlockSize=64;
    int64_t nBytes=1;
    clock_t begin, end;
    double timeSpent=0;
    double minTimeSpent=100; // 100 seconds
    int32_t loopCntForTime=1;
    int32_t loopCnt=0;
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
    printf("Fixed Bit Coding v1.6\n   file=%s\n", fName);
    fseek(f_input, 0, SEEK_END); // set to end of file
    if (ftell(f_input) > MAX_FILE_SIZE)
    {
        printf("fbc: file exceeds max size of %d\n", MAX_FILE_SIZE);
        return 10;
    }
    fseek(f_input, 0, SEEK_SET); // reset to beginning of file
    f_out = fopen(strcat(fName, ".fbc"), "w");
    nBytes = (long)fread(&inVal, 1, MAX_FILE_SIZE , f_input);
    if (nBytes < 1)
        return 3;
    strcpy(cfName, fName);
    f_compressedORnot = fopen(strcat(cfName, ".cq"), "w");
    if (!f_compressedORnot)
        return 4;
    if (argc >= 3)
    {
        int32_t blockSize;
        sscanf(argv[2], "%d", &blockSize);
        uintBlockSize = (uint32_t)blockSize;
    }
    if ((uintBlockSize < MIN_FBC_BYTES) || (uintBlockSize > MAX_FBC_BYTES))
    {
        printf("fbc error: block size must be from %d to %d\n", MIN_FBC_BYTES, MAX_FBC_BYTES);
        return 3;
    }
    countTop16(nBytes); // use to find the ordering of top 16 for text mode characters
        
    unsigned char blockSize;
    blockSize = (unsigned char)uintBlockSize;
    uint64_t nBytesWritten = fwrite(outVal, 1, total_out_bytes, f_out);
    if (nBytesWritten < total_out_bytes)
        return 7;
    if (fwrite(&blockSize, 1, sizeof(blockSize), f_compressedORnot) < sizeof(blockSize))
        return -13;
    if (argc >= 4)
        sscanf(argv[3], "%d", &loopCntForTime);
    if (loopCntForTime < 1 || loopCntForTime > 1000000)
        loopCntForTime = 1;

COMPRESS_TIMED_LOOP:
    begin = clock();
    nBytes_remaining = (long)nBytes;
    total_out_bytes = 0;
    start_inVal = 0;
    gCORNindex = 0;
    gCORNblocks = 0;
#ifdef GEN_STATS
    fTotalOutBytes = 1.0; // block size
#endif
    while (nBytes_remaining > 0)
    {
        nBytes_to_compress = (uint64_t)(nBytes_remaining<(long)uintBlockSize ? nBytes_remaining : (long)uintBlockSize);
        if (nBytes_to_compress < uintBlockSize)
        {
            // last bytes less than uintBlockSize are output uncompressed
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
        if (uintBlockSize < 6)
            nbout = fbc25(inVal+start_inVal, outVal+total_out_bytes, uintBlockSize);
        else
            nbout = fbc264(inVal+start_inVal, outVal+total_out_bytes, uintBlockSize);
        if (nbout < 0)
        {
            if (nbout == -1)
            {
                printf("Error from fbc264 %d: Values out of range 2 to 64\n", nbout);
                return -2;
            }
            if (nbout == -2)
            {
                printf("Error from fbc25 %d: Values out of range 2 to 5\n", nbout);
                return -2;
            }
            printf("Unexpected program error=%d", nbout);
            return -3;
        }
        else if (nbout == 0)
        {
            // unable to compress: output original vals and use gCompressedORnot to record
            memcpy(outVal+total_out_bytes, inVal+start_inVal, uintBlockSize);
            total_out_bytes += uintBlockSize;
            gCompressedORnot <<= 1; // 0 indicates not compressed
#ifdef GEN_STATS
            gCountUnableToCompress++;
            fTotalOutBytes += uintBlockSize;
#endif
        }
        else
        {
            // compressed output
#ifdef GEN_STATS
            fTotalOutBytes += (float)nbout / 8.0;
            if (uintBlockSize < 6)
            {
                // accumulate stats for 2-5 values
                if (nbout <= 10)
                {
                    gCountUniques[0]++; // single unique
                    gCountAverageUniques++; // single unique
                }
                else if ((nbout == 12) || (nbout == 14))
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
                uint32_t nUniques=(outVal[total_out_bytes] >> 1) & 0xf;
                if (outVal[total_out_bytes] & 1)
                {
                    gCountUniques[0]++; // 1 unique
                    gCountAverageUniques++;
                }
                else
                {
                    if (nUniques == 0)
                    {
                        if (outVal[total_out_bytes] == 0)
                            gTextModeCnt++; // text mode encoding
                        else
                            gSingleValueModeCnt++; // single value mode encoding
                    }
                    else
                    {
                        gCountUniques[nUniques]++; // 2 to 16 uniques encoded as 1 to 15
                        gCountAverageUniques += nUniques + 1;
                    }
                }
            }
#endif
            gCompressedORnot <<= 1;
            gCompressedORnot |= 1; // 1 indicates compressed
            compressedInBytes += uintBlockSize;
            compressedOutBytes += (float)nbout / 8.0;
            compressedBlockCount++;
            total_out_bytes += (uint64_t)nbout / 8;
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
    
    nBytesWritten = fwrite(outVal, 1, total_out_bytes, f_out);
    if (nBytesWritten < total_out_bytes)
        return 7;
    fclose(f_out);
    if (gCORNblocks & 0x3f)
    {
        // ouput final partial block of compressed or not
        gCompressedORnot <<= 64 - (gCORNblocks & 0x3f);
        gCORN[gCORNindex++] = gCompressedORnot;
    }
    for (uint32_t i=0; i<gCORNindex; i++)
        fwrite(&gCORN[i], sizeof(gCompressedORnot), 1, f_compressedORnot);
    fclose(f_compressedORnot);
    uint64_t gCORNbytes = (uint64_t)gCORNindex * sizeof(gCompressedORnot);
    
    printf("   compressed byte output=%.2f%%   compressed blocks=%.2lf%%\n   time=%f sec.   %.0f bytes per second   inbytes=%lld   outbytes=%llu\n   outbytes/block=%.2f   block size=%d   loop count=%d\n", (float)100*(1.0-(float)(total_out_bytes+gCORNbytes)/nBytes), (float)100*(1.0-(float)compressedOutBytes/(float)compressedInBytes),  minTimeSpent, (float)nBytes/minTimeSpent, nBytes, total_out_bytes+gCORNbytes, (float)(total_out_bytes+gCORNbytes)/nBytes*(float)uintBlockSize, uintBlockSize, loopCnt);
#ifdef GEN_STATS
    uint64_t compressedBlocks=gCountBlocks-gCountUnableToCompress;
    printf("   compressed bit output=%.2f%%   uncompressed blocks=%.2f%%\n   average # uniques=%.2f  1 unique=%.2f%%  2 nibbles=%.2f%%  2 u=%.2f%%  3 u=%.2f%%  4 u=%.2f%%  5 u=%.2f%%  6 u=%.2f%%  7 u=%.2f%%  8 u=%.2f%%  9 u=%.2f%%  10 u=%.2f%%  11 u=%.2f%%  12 u=%.2f%%  13 u=%.2f%%  14 u=%.2f%%  15 u=%.2f%%  16 u=%.2f%%\n", (1.0-(fTotalOutBytes+gCORNbytes)/(float)nBytes)*100,   (float)gCountUnableToCompress/(float)gCountBlocks*100,
        (float)gCountAverageUniques/(compressedBlocks-gTextModeCnt), (float)gCountUniques[0]/compressedBlocks*100, (float)gCountNibbles/gCountBlocks*100, (float)gCountUniques[1]/compressedBlocks *100, (float)gCountUniques[2]/compressedBlocks *100, (float)gCountUniques[3]/compressedBlocks *100, (float)gCountUniques[4]/compressedBlocks *100, (float)gCountUniques[5]/compressedBlocks *100, (float)gCountUniques[6]/compressedBlocks *100, (float)gCountUniques[7]/compressedBlocks *100, (float)gCountUniques[8]/compressedBlocks *100, (float)gCountUniques[9]/compressedBlocks *100, (float)gCountUniques[10]/compressedBlocks *100, (float)gCountUniques[11]/compressedBlocks *100, (float)gCountUniques[12]/compressedBlocks *100, (float)gCountUniques[13]/compressedBlocks *100, (float)gCountUniques[14]/compressedBlocks *100, (float)gCountUniques[15]/compressedBlocks *100);
    printf("   text mode blocks: %d  %.01f%% total blocks  %.01f%% compressed blocks\n", gTextModeCnt/loopCnt, (float)gTextModeCnt/(float)gCountBlocks*100, (float)gTextModeCnt/(float)compressedBlocks*100);
    printf("   single value mode blocks: %d  %.01f%% total blocks  %.01f%% compressed blocks\n", gSingleValueModeCnt/loopCnt, (float)gSingleValueModeCnt/(float)gCountBlocks*100, (float)gSingleValueModeCnt/(float)compressedBlocks*100);
#endif
    
    // decompress output ------------------------------------
    f_input = fopen(fName, "r");
    f_out = fopen(strcat(fName, "d"), "w");
    nBytes = (long)fread(&inVal, 1, total_out_bytes , f_input);
    if ((uint64_t)nBytes < total_out_bytes)
        return 5;
    loopCnt = 0;
    minTimeSpent = 60; // 60 seconds
    gCountBlocks = 0;
    uint32_t bytes_processed;
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
            uint64_t outBytes;
            if ((uint64_t)nBytes_remaining < uintBlockSize)
                outBytes = (uint64_t)nBytes_remaining; // last block in file
            else
                outBytes = uintBlockSize;
            memcpy(outVal+total_out_bytes, inVal+start_inVal, outBytes);
            total_out_bytes += outBytes;
            start_inVal += outBytes;
            nBytes_remaining -= outBytes;
        }
        else
        {
            // compressed input block
            if (uintBlockSize < 6)
                bytes_decompressed = fbc25d(inVal+start_inVal, outVal+total_out_bytes, uintBlockSize, &bytes_processed);
            else
                bytes_decompressed = fbc264d(inVal+start_inVal, outVal+total_out_bytes, uintBlockSize, &bytes_processed);
            if (bytes_decompressed < 1)
            {
                fwrite(outVal, 1, total_out_bytes, f_out);
                fclose(f_out);
                fclose(f_compressedORnot);
                printf("error from fbc264d\n");
                goto COMPRESS_DATA;
            }
            total_out_bytes += (uint64_t)bytes_decompressed;
            start_inVal += (uint64_t)bytes_processed;
            nBytes_remaining -= bytes_processed;
        }
        if (++gCORNblocks & 0x3f)
        {
            gCompressedORnot <<= 1;
        }
        else
        {
            //uint64_t cBytes;
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

    fwrite(outVal, 1, (uint64_t)total_out_bytes, f_out);
    fclose(f_out);
    //fclose(f_compressedORnot);
    printf("fbc264d decompression bytes per second=%.0lf   time=%f sec.\n   inbytes=%lld   outbytes=%llu\n", (float)total_out_bytes/(float)minTimeSpent, minTimeSpent, nBytes, total_out_bytes);
    // compare two files someday
COMPRESS_DATA:
    return 0;
}
