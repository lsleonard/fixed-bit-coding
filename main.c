//
//  main.c
//  Fixed Bit Coding FBC
//  Support 2 to 64 input values.
//  Handles 2 to 5 input values directly.
//  Uses FBC for 6 to 64 input values.

//  Created by Stevan Leonard on 7/16/20.
//  Copyright © 2020 Oxford House Software. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define MAX_FBC_BYTES 64  // max input vals supported
#define MIN_FBC_BYTES 2  // min input vals supported
#define MAX_UNIQUES 16 // max uniques supported in input
#define BLOCK_BYTES 64 // number of input values per block

int gCountBlocks=0;
int gCountUnableToCompress=0;

//#define genStats
#ifdef genStats
int gEarlyCountUnableToCompress=0;
static int gCountUniques[MAX_UNIQUES];
int gCountAverageUniques=0;
int gCountNibbles=0;
#endif

// globals that should be initialized when loaded
// ----------------------------------------------

// for the number of uniques in input, the minimum number of input values for 25% compression
// uniques   1  2  3  4  5   6   7   8   9   10  11  12  13  14  15  16
// nvalues   2, 4, 7, 9, 15, 17, 19, 23, 36, 42, 46, 50, 54, 58, 62, 64};
int uniqueLimits[MAX_FBC_BYTES+1]=
//      2    4      7    9            15   17   19       23
{ 0, 0, 1,1, 2,2,2, 3,3, 4,4,4,4,4,4, 5,5, 6,6, 7,7,7,7, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    // 40    44           48           52           56           60     62     64
    9,9,9,9, 10,10,10,10, 11,11,11,11, 12,12,12,12, 13,13,13,13, 14,14, 15,15, 16};

// -----------------------------------------------------------------------------------
static int fbc25(unsigned char *inVals, unsigned char *outVals, unsigned int nValues)
// -----------------------------------------------------------------------------------
// handle 2 to 5 values with high compression for 1 or 2 unique values and no ibe
// decode takes into account the number of bytes to encode first byte:
// 1 = single unique, followed by 1 if upper two bits are 00 and low 6 bits of input; otherwise, high two bits in second byte
// 0 = encoding by number of values:
//    2 bytes: two nibbles the same
//    3 bytes: only supports single unique
//    4 or 5 bytes: two uniques
{
    switch (nValues)
    {
        case 2:
        {
            int ival1=(unsigned char)inVals[0];
            // encode for 1 unique or 2 nibbles
            if (ival1 == (unsigned char)inVals[1])
            {
#ifdef genStats
                gCountUniques[0]++;
#endif
                if (ival1 > 63)
                {
                    outVals[0] = (unsigned char)(ival1 << 2) | 1;
                    outVals[1] = (unsigned char)ival1 >> 6;
                    return 10;
                }
                outVals[0] = (unsigned char) (ival1 << 2) | 3;
                return 8;
            }
            // check for two nibbles the same
            int outBits=0; // keep track of which nibbles match nibble1 or otherVal
            int otherVal=-1;
            int nibble1=ival1 >> 4;
            int nibble2=ival1 & 0xf;
            int nibble3=(unsigned char)inVals[1] >> 4;
            if (nibble2 != nibble1)
            {
                otherVal = nibble2;
                outBits = 2;
            }
            if (nibble3 != nibble1)
            {
                if ((nibble3 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = nibble3;
                outBits |= 4; // set this bit to other value
            }
            int nibble4=(unsigned char)inVals[1] & 0xf;
            if (nibble4 != nibble1)
            {
                if ((nibble4 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = nibble4;
                outBits |= 8;  // set this bit to other value
            }
            outVals[0] = (unsigned char)(nibble1 << 4) | outBits;
            outVals[1] = (unsigned char)otherVal;
#ifdef genStats
            gCountNibbles++;
#endif
            return 12; // save 4 bits
        }
        case 3:
        {
            // encode for 1 unique
            int ival1=(unsigned char)inVals[0];
            if ((ival1 == (unsigned char)inVals[1]) && (ival1 == (unsigned char)inVals[2]))
            {
#ifdef genStats
                gCountUniques[0]++;
#endif
                if (ival1 > 63)
                {
                    outVals[0] = (unsigned char)(ival1 << 2) | 1;
                    outVals[1] = (unsigned char)ival1 >> 6;
                    return 10;
                }
                outVals[0] = (unsigned char)(ival1 << 2) | 3;
                return 8;
            }
            return 0;
        }
        case 4:
        case 5:
        {
            // encode for 1 or 2 uniques
            int ival1=(unsigned char)inVals[0];
            int ival2=(unsigned char)inVals[1];
            int ival3=(unsigned char)inVals[2];
            int ival4=(unsigned char)inVals[3];
            int outBits=0; // first bit is 1 for one unique
            // encode 0 for first val, 1 for other val, starting at second bit
            int otherVal=-1;
            if (ival2 != ival1)
            {
                otherVal=ival2;
                outBits |= 2;
            }
            if (ival3 != ival1)
            {
                if ((ival3 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal ==  -1)
                    otherVal = ival3;
                outBits |= 4;
            }
            if (ival4 != ival1)
            {
                if ((ival4 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = ival4;
                outBits |= 8;
            }
            if ((nValues == 4) && (outBits))
            {
                // or in low 4 bits second unique in top bits
                outVals[0] = (unsigned char)outBits | (ival1 << 4);
                outVals[1] = (unsigned char)(ival1 >> 4) | (otherVal << 4);
                outVals[2] = (unsigned char)otherVal >> 4;
#ifdef genStats
                gCountUniques[1]++;
#endif
                return 20;
            }
            if (nValues == 4)
            {
                // all 4 values equal
#ifdef genStats
                gCountUniques[0]++;
#endif
                if (ival1 > 63)
                {
                    outVals[0] = (unsigned char)(ival1 << 2) | 1;
                    outVals[1] = (unsigned char)ival1 >> 6;
                    return 10;
                }
                outVals[0] = (unsigned char)(ival1 << 2) | 3;
                return 8;
            }
            // continue to check fifth value
            int ival5=(unsigned char)inVals[4];
            if (ival5 != ival1)
            {
                if ((ival5 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = ival5;
                outBits |= 0x10;
            }
            if (outBits)
            {
                // clear low bit byte 0 (not single unique) and or in low 3 bits ival1
                outVals[0] = (unsigned char)outBits | (ival1 << 5);
                outVals[1] = (unsigned char)((ival1 >> 3)) | (otherVal << 5);
                outVals[2] = (unsigned char)otherVal >> 3;
#ifdef genStats
                gCountUniques[1]++;
#endif
                return 21;
            }
            // all 5 values equal
#ifdef genStats
            gCountUniques[0]++;
#endif
            if (ival1 > 63)
            {
                outVals[0] = (unsigned char)(ival1 << 2) | 1;
                outVals[1] = (unsigned char)ival1 >> 6;
                return 10;
            }
            outVals[0] = (unsigned char)(ival1 << 2) | 3;
            return 8;
        }
        default:
            return -1; // number of values specified not supported (2 to 5 handled here)
    }
} // end fbc25

// -----------------------------------------------------------------------------------
static int fbc25d(unsigned char *inVals, unsigned char *outVals, int nOriginalValues, int *bytesProcessed)
// -----------------------------------------------------------------------------------
// Two compression modes: high compression and incremental encoding
// First two bits indicate high compression:
//    bit 0 is set: single unique value for all bytes, if bit 1 is set, 6 bits follow;
//       otherwise, high two bits in next byte
//    bit 0 is 0: next four bits indicate 2 to 16 uniques
//       remaining 3 bits in this byte are encoding, followed by uniques
//       then the encoding for remaining values. All values encoded with same number of bits
// inVals   compressed data with fewer bits than in original values
// outVals  decompressed data
// nOriginalalues  number of values in the original input to fbc25: required input
// return number of bytes output or -1 if error
{
    int firstByte=(unsigned char)inVals[0];
    if (firstByte & 1)
    {
        // process single unique
        int unique = (unsigned char)firstByte >> 2;
        if (!(firstByte & 2))
            unique |= ((unsigned char)inVals[1]) << 6;
        memset(outVals, (unsigned char)unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return nOriginalValues;
    }
    int secondByte;
    int thirdByte;
    int cbits;
    int nibble1;
    int nibble2;
    int val1;
    int val2;
    switch (nOriginalValues)
    {
            // special encoding for 2 to 5 bytes
        case 2:
            // decode nibble compression for 2 bytes
            // nibble1 nibble 2   nibble3 nibble4
            // high    low        high    low
            // first byte         second byte
            *bytesProcessed = 2;
            secondByte=(unsigned char)inVals[1];
            cbits = (firstByte >> 1) & 0x7; // 3 control bits
            nibble1 = (unsigned char)firstByte >> 4;
            nibble2 = (unsigned char)secondByte & 0xf;
            outVals[0] = (unsigned char)(nibble1 << 4) | ((cbits & 1) ? nibble2 : nibble1);
            outVals[1] = (unsigned char)(((cbits & 2) ? nibble2 : nibble1) << 4) |
            ((cbits & 4) ? nibble2 : nibble1);
            return 2;
        case 3:
            // only single unique supported for 3 bytes
            return -1;
        case 4: // 2 uniques
            *bytesProcessed = 3;
            secondByte = (unsigned char)inVals[1];
            thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 4) | (unsigned char)(secondByte << 4);
            val2 = (unsigned char)(secondByte >> 4) | (unsigned char)(thirdByte << 4);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            return 4;
        case 5: // 2 uniques
            *bytesProcessed = 3;
            secondByte = (unsigned char)inVals[1];
            thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 5) | (unsigned char)(secondByte << 3);
            val2 = (unsigned char)(secondByte >> 5) | (unsigned char)(thirdByte << 3);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[4] = (firstByte & 0x10) ? (unsigned char)val2 : (unsigned char)val1;
            return 5;
        default:
            return -1;
    }
} // end fbc25d

// -----------------------------------------------------------------------------------
static int fbc264(unsigned char *inVals, unsigned char *outVals, unsigned int nValues)
// -----------------------------------------------------------------------------------
// fbc264: Compress nValues bytes. Return 0 if not compressible (no output bytes),
//    -1 if error; otherwise, number of bits written to outVals.
//    Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// inVals   input byte values
// outVals  output byte values if compressed, max of inVals bytes
// nValues  number of input byte values
{
    if (nValues <= 5)
        return fbc25(inVals, outVals, nValues);
    
    if (nValues > MAX_FBC_BYTES)
        return -1; // error
    
    unsigned int uniqueOccurrence[256]; // order of occurrence of uniques
    unsigned int nUniqueVals=0; // count of unique vals encountered
    unsigned char val256[256];
    memset(val256, 0, 256);
    
    // process input vals by finding and outputting the uniques starting at 2nd outVal
    // and encoding the repeats to be output later
    unsigned int inPos=0;
    unsigned int uniqueLimit=uniqueLimits[nValues]-1; // return uncompressible if exceeded
    unsigned int iVal; // current inVals[inPos]
    while (inPos < nValues)
    {
        iVal = inVals[inPos++]; // get this value and inc to next
        // if this is first occurrence of value, add to uniques and set control bit to 0
        if (val256[iVal] == 0)
        {
            // first encounter of this value
            if (nUniqueVals > uniqueLimit)
            {
                return 0; // not compressible
            }
            val256[iVal] = 1; // mark encountered
            uniqueOccurrence[iVal] = nUniqueVals; // occurrence count for this unique 0-based
            nUniqueVals++;
            outVals[nUniqueVals] = iVal; // store unique in output starting at second byte
        }
    } // end of loop finding unique values
 
#ifdef genStats
    gCountUniques[nUniqueVals-1]++;
    gCountAverageUniques += nUniqueVals;
#endif
    int i;
    int nextOut;
    unsigned int encodingByte;
    unsigned long encodingInt;
    unsigned int ebShift;
    switch (nUniqueVals)
    {
        case 1:
        {
        // ********************** ALL BYTES SAME VALUE *********************
        // first byte: low-order bit indicates single value if 1
        // if single value, next bit indicates whether coded in next 6 bits or next byte
            unsigned int ival1=inVals[0];
            if (ival1 < 64)
            {
                outVals[0] = ((ival1 << 2) | 3); // indicate single val and encode in high 6 bits
                return 8; // return number of bits output
            }
            else
            {
                outVals[0] = (1 | (ival1 << 2));
                outVals[1] = ival1 >> 6;
                return 10; // return number of bits output
            }
        }
        case 2:
        {
            // 2 uniques so 1 bit required for each input value
            encodingByte=2; // indicate 2 uniques in bits 1-4
            // fill in upper 3 bits and output
            encodingByte |= uniqueOccurrence[inVals[0]] << 5;
            encodingByte |= uniqueOccurrence[inVals[1]] << 6;
            encodingByte |= uniqueOccurrence[inVals[2]] << 7;
            outVals[0] = encodingByte;
            ebShift=1;
            encodingInt = 0;
            i = 3;
            nextOut = 3;
            while (i + 7 < nValues)
            {
                encodingByte = (uniqueOccurrence[inVals[i++]]);
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift++;
                outVals[nextOut++] = encodingByte;
                ebShift = 1;
            }
            if (i < nValues)
            {
                ebShift = 0;
                encodingByte = 0;
                while (i < nValues)
                {
                    encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift;
                    ebShift++;
                }
                outVals[nextOut] = encodingByte;
            }
            return nValues + 21; // one bit encoding for each value + 5 indicator bits + 2 uniques
        }
        case 3:
        {
            // include with 4 as two bits are used to encode each value
        }
        case 4:
        {
            // 3 or 4 uniques so 2 bits required for each input value
            if (nUniqueVals == 3)
                encodingByte = 4; // 3 uniques
            else
                encodingByte = 6; // 4 uniques
            encodingByte |= uniqueOccurrence[inVals[0]] << 5;
            // skipping last bit in first byte to be on even byte boundary
            outVals[0] = encodingByte;
            int nextOut=nUniqueVals + 1; // start output past uniques
            int i=1; // start input on second value
            while (i+3 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 6;
                outVals[nextOut++] = encodingByte;
            }
            if (i < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i < nValues)
                {
                    encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                    if (i < nValues)
                    {
                        encodingByte |= uniqueOccurrence[inVals[i]] << 4;
                    }
                }
                outVals[nextOut] = encodingByte; // output last partial byte
            }
            return (nValues * 2) + 6 + (nUniqueVals * 8); // two bits for each value plus 6 indicator bits + 3 or 4 uniques
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3 bits to encode
            encodingByte = (nUniqueVals-1) << 1; // 5 to 8 uniques
            encodingByte |= uniqueOccurrence[inVals[0]] << 5; // first val
            outVals[0] = encodingByte; // save first byte
            nextOut=nUniqueVals + 1; // start output past uniques
            // process 8 values and output 3 8-bit values
            i = 1; // first val in first byte
            unsigned int partialVal;
            while (i + 7 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                encodingByte |= uniqueOccurrence[inVals[i++]] << 3;
                partialVal = inVals[i++];
                encodingByte |= uniqueOccurrence[partialVal] << 6;
                outVals[nextOut++] = encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 1;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                partialVal = inVals[i++];
                encodingByte |= uniqueOccurrence[partialVal] << 7;
                outVals[nextOut++] = encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 1;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 5;
                outVals[nextOut++] = encodingByte;
            }
            // 0 to 7 vals left to process, check after each one for completion
            unsigned int partialByte=0;
            while (i < nValues)
            {
                partialByte = 1;
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 3;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[(partialVal=inVals[i++])] << 6;
                outVals[nextOut++] = encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 2;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 1;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[(partialVal=inVals[i++])] << 7;
                outVals[nextOut++] = encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 1;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
            }
            if (partialByte)
                outVals[nextOut] = encodingByte;

            return (nValues * 3) + 5 + (nUniqueVals * 8); // three bits for each value plus 5 indicator bits
            }
        default: // nUniques 9 through 16
        {
            if (nUniqueVals > MAX_UNIQUES)
                return -1;
            // cases 9 through 16 take 4 bits to encode
            // skipping last 3 bits in first byte to be on even byte boundary
            outVals[0] = (nUniqueVals-1) << 1;
            nextOut = nUniqueVals + 1; // start output past uniques
            i=0;
            while (i + 3 < nValues)
            {
                outVals[nextOut++] = uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4;
                i += 2;
                outVals[nextOut++] = uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4;
                i += 2;
                outVals[nextOut++] = uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4;
                i += 2;
                outVals[nextOut++] = uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4;
                i += 2;
            }
            if (i < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i < nValues)
                {
                    outVals[nextOut++] = encodingByte | (uniqueOccurrence[inVals[i+1]] << 4);
                    if (i < nValues)
                        outVals[nextOut] = uniqueOccurrence[inVals[i]];
                }
                else
                    outVals[nextOut] = encodingByte;
            }
            return (nValues * 4) + 8 + (nUniqueVals * 8); // four bits for each value plus 8 indicator bits + 9 to 16 uniques
        }
    }
    return -1;
} // end fbc264

// -----------------------------------------------------------------------------------
static int fbc264d(unsigned char *inVals, unsigned char *outVals, int nOriginalValues, int *bytesProcessed)
// -----------------------------------------------------------------------------------
// fixed bit decoding requires number of original values and encoded bytes
// uncompressed data is not acceppted
// 2 to 16 unique values were encoded for 2 to 64 input values.
// 2 to 5 input values are handled separately.
// inVals   compressed data with fewer bits than in original values
// outVals  decompressed data
// nOriginalalues  number of values in the original input to fbc264: required input
// return number of bytes output or -1 if error
{
    if (nOriginalValues <= 5)
        return fbc25d(inVals, outVals, nOriginalValues, bytesProcessed);
    
    if (nOriginalValues > MAX_FBC_BYTES)
        return -1;
        
    unsigned int firstByte=inVals[0];
    if (firstByte & 1)
    {
        // process single unique
        unsigned int unique = firstByte >> 2;
        if (!(firstByte & 2))
            unique |= inVals[1] << 6;
        memset(outVals, unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return nOriginalValues;
    }
    
    unsigned int nUniques = ((firstByte >> 1) & 0xf) + 1;
    unsigned int uniques[MAX_UNIQUES];
    unsigned int uniques1;
    unsigned int uniques2;
    int nextInVal;
    int nextOutVal;
    unsigned int inByte;
    switch (nUniques)
    {
        case 2:
        {
            // 1-bit values
            uniques1 = inVals[1];
            uniques2 = inVals[2];
            outVals[0] = ((firstByte >> 5) & 1) ? uniques2 : uniques1;
            outVals[1] = ((firstByte >> 6) & 1) ? uniques2 : uniques1;
            outVals[2] = ((firstByte >> 7) & 1) ? uniques2 : uniques1;
            nextInVal=3;
            nextOutVal=3;
            while (nextOutVal+7 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (inByte & 1) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 2) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 4) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 8) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 16) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 32) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 64) ? uniques2 : uniques1;
                outVals[nextOutVal++] = (inByte & 128) ? uniques2 : uniques1;
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (inByte & 1) ? uniques2 : uniques1;
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (inByte & 2) ? uniques2 : uniques1;
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (inByte & 4) ? uniques2 : uniques1;
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (inByte & 8) ? uniques2 : uniques1;
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (inByte & 16) ? uniques2 : uniques1;
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (inByte & 32) ? uniques2 : uniques1;
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (inByte & 64) ? uniques2 : uniques1;
            }
            *bytesProcessed = nextInVal;
            return nOriginalValues;
        }
        case 3:
        case 4:
        {
            // 2-bit values
            uniques[0] = inVals[1];
            uniques[1] = inVals[2];
            uniques[2] = inVals[3];
            if (nUniques == 4)
            {
                uniques[3] = inVals[4];
                nextInVal = 5; // for 4 uniques
            }
            else
                nextInVal = 4; // for 3 uniques
            outVals[0] = uniques[(firstByte >> 5)&3];
            nextOutVal = 1; // skip high bit of first byte
            while (nextOutVal + 3 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[inByte&3];
                outVals[nextOutVal++] = uniques[(inByte>>2)&3];
                outVals[nextOutVal++] = uniques[(inByte>>4)&3];
                outVals[nextOutVal++] = uniques[(inByte>>6)&3];
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[inByte&3];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = uniques[(inByte>>2)&3];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = uniques[(inByte>>4)&3];
            }
            *bytesProcessed = nextInVal;
            return nOriginalValues;
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3-bit values for 5 to 8 uniques
            for (int i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // for 4 uniques
            outVals[0] = uniques[(firstByte >> 5)&7];
            nextOutVal = 1;
            unsigned int inByte2;
            unsigned int inByte3;
            while (nextOutVal + 7 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                inByte2 = inVals[nextInVal++];
                inByte3 = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[inByte&7];
                outVals[nextOutVal++] = uniques[(inByte>>3)&7];
                outVals[nextOutVal++] = uniques[((inByte>>6) | (inByte2<<2))&7];
                outVals[nextOutVal++] = uniques[(inByte2>>1)&7];
                outVals[nextOutVal++] = uniques[(inByte2>>4)&7];
                outVals[nextOutVal++] = uniques[((inByte2>>7) | (inByte3<<1))&7];
                outVals[nextOutVal++] = uniques[(inByte3>>2)&7];
                outVals[nextOutVal++] = uniques[(inByte3>>5)&7];
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[inByte&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = uniques[(inByte>>3)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                inByte2 = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[((inByte>>6) | (inByte2<<2))&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = uniques[(inByte2>>1)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = uniques[(inByte2>>4)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                inByte3 = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[((inByte2>>7) | (inByte3<<1))&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = uniques[(inByte3>>2)&7];
            }
            *bytesProcessed = nextInVal;
            return nOriginalValues;
        }
        default:
        {
            // 4-bit values for 9 to 16 uniques
            if (nUniques > MAX_UNIQUES)
                return -1;
            for (int i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // skip past uniques
            nextOutVal = 0;
            while (nextOutVal + 1 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = uniques[inByte&0xf];
                outVals[nextOutVal++] = uniques[(inByte>>4)];
            }
            if (nextOutVal < nOriginalValues)
            {
                outVals[nextOutVal++] = uniques[inVals[nextInVal++] & 0xf];
            }
            *bytesProcessed = nextInVal;
            return nOriginalValues;
        }
    }
    return -1;
} // end fbc264d


#include "time.h"
#include <unistd.h>
#define MAX_FILE_SIZE 20000000
unsigned char inVal[MAX_FILE_SIZE]; // read entire file into memory
unsigned char outVal[MAX_FILE_SIZE]; // encode into memory
long total_out_bytes;
double fTotalOutBytes;
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
        printf("fixed bit coding error: input file must be specified\n");
        return 14;
    }
    strcpy(fName, argv[1]);
    f_input = fopen(fName, "r");
    if (!f_input)
    {
        printf("fixed bit coding error: file not found: %s\n", fName);
        return 9;
    }
    fseek(f_input, 0, SEEK_END); // set to end of file
    if (ftell(f_input) > MAX_FILE_SIZE)
    {
        printf("fixed bit coding error: file exceeds max size of %d\n", MAX_FILE_SIZE);
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
    unsigned char blockSize=BLOCK_BYTES;
    if (fwrite(&blockSize, 1, sizeof(blockSize), f_compressedORnot) < 0)
        return -13;
    int intBlockSize=blockSize;
    if (argc == 3)
        sscanf(argv[2], "%d", &loopCntForTime);

START_TIMED_LOOP:
    begin = clock();
    nBytes_remaining = nBytes;
    total_out_bytes = 0;
    fTotalOutBytes = 1.0; // block size
    start_inVal = 0;
    gCORNindex = 0;
    gCORNblocks = 0;
    while (nBytes_remaining > 0)
    {
        nBytes_to_compress = nBytes_remaining<BLOCK_BYTES ? nBytes_remaining : BLOCK_BYTES;
        gCountBlocks++; // global count of blocks processed
        if (nBytes_to_compress < BLOCK_BYTES)
        {
            // last bytes less than BLOCK_BYTES are output uncompressed
            memcpy(outVal+total_out_bytes, inVal+start_inVal, nBytes_to_compress);
            total_out_bytes += nBytes_to_compress;
#ifdef genStats
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
        nbout = fbc264(inVal+start_inVal, outVal+total_out_bytes, BLOCK_BYTES);
        if (nbout < 0)
        {
            printf("Error from fbc %d: Number of values must be from 2 to 64\n", nbout);
            return -2;
        }
        else if (nbout == 0)
        {
            // unable to compress: output original vals and use gCompressedORnot to record
            gCountUnableToCompress++;
            memcpy(outVal+total_out_bytes, inVal+start_inVal, BLOCK_BYTES);
            total_out_bytes += BLOCK_BYTES;
            gCompressedORnot <<= 1; // 0 indicates not compressed
#ifdef genStats
            if (BLOCK_BYTES < 6)
                gEarlyCountUnableToCompress++;
            fTotalOutBytes += BLOCK_BYTES;
#endif
        }
        else
        {
            // compressed
            gCompressedORnot <<= 1;
            gCompressedORnot |= 1; // 1 indicates compressed
            compressedInBytes += BLOCK_BYTES;
            compressedOutBytes += (float)nbout / 8.0;
            compressedBlockCount++;
            total_out_bytes += nbout / 8;
            if (nbout & 7)
                total_out_bytes++; // add partial byte for byte boundary
#ifdef genStats
            fTotalOutBytes += (float)nbout / 8.0;
            if (BLOCK_BYTES < 6)
            {
                // accumulate stats for 2-5 values
                if (nbout <= 10)
                    gCountAverageUniques++;
                else
                    gCountAverageUniques += 2;
            }
#endif
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
    usleep(10); // sleep 10 us
    if (++loopCnt < loopCntForTime)
        goto START_TIMED_LOOP;
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
    
    printf("Fixed Bit Coding   compressed byte output=%.2f%%   compressed blocks=%.2lf%%\n   time=%f sec.   %.0f bytes per second   inbytes=%lu   outbytes=%ld\n   outbytes/block=%.2f   block size=%d\n   loop count=%d   file=%s\n", (float)100*(1.0-(float)(total_out_bytes+gCORNbytes)/nBytes), (float)100*(1.0-(float)compressedOutBytes/(float)compressedInBytes),  minTimeSpent, (float)nBytes/minTimeSpent, nBytes, total_out_bytes+gCORNbytes, (float)(total_out_bytes+gCORNbytes)/nBytes*(float)BLOCK_BYTES, BLOCK_BYTES, loopCnt, fName);
#ifdef genStats
    long int compressedBlocks=gCountBlocks-gCountUnableToCompress;
    printf("   compressed bit output=%.2f%%   uncompressed blocks=%.2f%%\n   average # uniques=%.2f  1 unique=%.2f%%  2 nibbles=%.2f%%  2 u=%.2f%%  3 u=%.2f%%  4 u=%.2f%%  5 u=%.2f%%  6 u=%.2f%%  7 u=%.2f%%  8 u=%.2f%%  9 u=%.2f%%  10 u=%.2f%%  11 u=%.2f%%  12 u=%.2f%%  13 u=%.2f%%  14 u=%.2f%%  15 u=%.2f%%  16 u=%.2f%%\n", (1.0-(fTotalOutBytes+gCORNbytes)/(float)nBytes)*100,   (float)gCountUnableToCompress/(float)gCountBlocks*100,
        (float)gCountAverageUniques/compressedBlocks, (1.0-(float)gCountUniques[0]/compressedBlocks*100), (float)gCountNibbles/gCountBlocks*100, (float)gCountUniques[1]/compressedBlocks *100, (float)gCountUniques[2]/compressedBlocks *100, (float)gCountUniques[3]/compressedBlocks *100, (float)gCountUniques[4]/compressedBlocks *100, (float)gCountUniques[5]/compressedBlocks *100, (float)gCountUniques[6]/compressedBlocks *100, (float)gCountUniques[7]/compressedBlocks *100, (float)gCountUniques[8]/compressedBlocks *100, (float)gCountUniques[9]/compressedBlocks *100, (float)gCountUniques[10]/compressedBlocks *100, (float)gCountUniques[11]/compressedBlocks *100, (float)gCountUniques[12]/compressedBlocks *100, (float)gCountUniques[13]/compressedBlocks *100, (float)gCountUniques[14]/compressedBlocks *100, (float)gCountUniques[15]/compressedBlocks *100);
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
START_DECOMPRESS_TIMED_LOOP:
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
    usleep(10); // sleep 10 us
    if (++loopCnt < loopCntForTime)
        goto START_DECOMPRESS_TIMED_LOOP;

    fwrite(outVal, 1, total_out_bytes, f_out);
    fclose(f_out);
    //fclose(f_compressedORnot);
    printf("fbc264d decompression bytes per second=%.0lf   time=%f sec.\ninbytes=%lu   outbytes=%ld\n", (float)total_out_bytes/(float)minTimeSpent, minTimeSpent, nBytes, total_out_bytes);
    // compare two files someday
COMPRESS_DATA:
    return 0;
}
