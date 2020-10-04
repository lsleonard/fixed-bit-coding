//
//  fbc.h
//  fixed bit coding implementation in static functions
//  version 1.0
//
//  Created by Stevan Leonard on 7/24/20.
//  Copyright © 2020 L. Stevan Leonard. All rights reserved.
/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.//
*/
// Notes for version 1.0:
//   1. The encoding of first byte value could be removed as the first byte is always
//   the first unique character. Changes envisioned for a future release will
//   require encoding the first value.
//
//   2. For 3 input values, the encoding of 2 nibbles is planned for a future release.

#ifndef fbc_h
#define fbc_h

#include <stdint.h>
#include <string.h>

#define MAX_FBC_BYTES 64  // max input vals supported
#define MIN_FBC_BYTES 2  // min input vals supported
#define MAX_UNIQUES 16 // max uniques supported in input

// ----------------------------------------------
// for the number of uniques in input, the minimum number of input values for 25% compression
// uniques   1  2  3  4  5   6   7   8   9   10  11  12  13  14  15  16
// nvalues   2, 4, 7, 9, 15, 17, 19, 23, 40, 44, 48, 52, 56, 60, 62, 64};
static const uint32_t uniqueLimits[MAX_FBC_BYTES+1]=
//      2    4      7    9            15   17   19       23
{ 0, 0, 1,1, 2,2,2, 3,3, 4,4,4,4,4,4, 5,5, 6,6, 7,7,7,7, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    // 40    44           48           52           56           60     62     64
    9,9,9,9, 10,10,10,10, 11,11,11,11, 12,12,12,12, 13,13,13,13, 14,14, 15,15, 16};

// -----------------------------------------------------------------------------------
static inline int32_t fbc25(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// -----------------------------------------------------------------------------------
// Compress 2 to 5 values with 1 or 2 unique values
// Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Decode first byte:
// 1 = single unique, followed by 1 if upper two bits are 00 and low 6 bits of input; otherwise, high two bits in second byte
// 0 = encoding by number of values:
//    2 bytes: two nibbles the same
//    3 bytes: only supports single unique
//    4 or 5 bytes: two uniques
// Arguments:
//   inVals   input data
//   outVals  compressed data
//   nValues  number of values to compress
// returns number of bits compressed, 0 if not compressible, or -1 if error
{
    switch (nValues)
    {
        case 2:
        {
            const uint32_t ival1=(unsigned char)inVals[0];
            // encode for 1 unique or 2 nibbles
            if (ival1 == (unsigned char)inVals[1])
            {
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
            int32_t outBits=0; // keep track of which nibbles match nibble1 or otherVal
            int32_t otherVal=-1;
            int32_t nibble1=ival1 >> 4;
            int32_t nibble2=ival1 & 0xf;
            int32_t nibble3=(unsigned char)inVals[1] >> 4;
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
            int32_t nibble4=(unsigned char)inVals[1] & 0xf;
            if (nibble4 != nibble1)
            {
                if ((nibble4 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = nibble4;
                outBits |= 8;  // set this bit to other value
            }
            outVals[0] = (unsigned char)((nibble1 << 4) | outBits);
            outVals[1] = (unsigned char)otherVal;
            return 12; // save 4 bits
        }
        case 3:
        {
            // encode for 1 unique
            const uint32_t ival1=(unsigned char)inVals[0];
            if ((ival1 == (unsigned char)inVals[1]) && (ival1 == (unsigned char)inVals[2]))
            {
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
            const int32_t ival1=(unsigned char)inVals[0];
            const int32_t ival2=(unsigned char)inVals[1];
            const int32_t ival3=(unsigned char)inVals[2];
            const int32_t ival4=(unsigned char)inVals[3];
            int32_t outBits=0; // first bit is 1 for one unique
            // encode 0 for first val, 1 for other val, starting at second bit
            int32_t otherVal=-1;
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
                outVals[0] = (unsigned char)(outBits | (ival1 << 4));
                outVals[1] = (unsigned char)((ival1 >> 4) | (otherVal << 4));
                outVals[2] = (unsigned char)(otherVal >> 4);
                return 20;
            }
            if (nValues == 4)
            {
                // all 4 values equal
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
            const int32_t ival5=(unsigned char)inVals[4];
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
                outVals[0] = (unsigned char)(outBits | (ival1 << 5));
                outVals[1] = (unsigned char)(((ival1 >> 3)) | (otherVal << 5));
                outVals[2] = (unsigned char)(otherVal >> 3);
                return 21;
            }
            // all 5 values equal
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
static inline int32_t fbc25d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, int32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
// Decode 2 to 5 values encoded by fbc25.
// Decode first byte:
// 1 = single unique, followed by 1 if upper two bits are 00 and low 6 bits of input; otherwise, high two bits in second byte
// 0 = encoding by number of values:
//    2 bytes: two nibbles the same
//    3 bytes: only supports single unique
//    4 or 5 bytes: two uniques
// Arguments:
//   inVals   compressed data with fewer bits than in original values
//   outVals  decompressed data
//   nOriginalValues  number of values in the original input to fbc25: required input
//   bytesProcessed  number of input bytes processed, which can be used by caller to position past these bytes
// returns number of bytes output or -1 if error
{
    const int32_t firstByte=(unsigned char)inVals[0];
    if (firstByte & 1)
    {
        // process single unique
        int32_t unique = (unsigned char)firstByte >> 2;
        if (!(firstByte & 2))
            unique |= ((unsigned char)inVals[1]) << 6;
        memset(outVals, (unsigned char)unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return (int)nOriginalValues;
    }
    int32_t cbits;
    int32_t nibble1;
    int32_t nibble2;
    int32_t val1;
    int32_t val2;
    const int32_t secondByte = (unsigned char)inVals[1];
    switch (nOriginalValues)
    {
            // special encoding for 2 to 5 bytes
        case 2:
            // decode nibble compression for 2 bytes
            // nibble1 nibble 2   nibble3 nibble4
            // high    low        high    low
            // first byte         second byte
            *bytesProcessed = 2;
            cbits = (firstByte >> 1) & 0x7; // 3 control bits
            nibble1 = (unsigned char)firstByte >> 4;
            nibble2 = (unsigned char)secondByte & 0xf;
            outVals[0] = (unsigned char)((nibble1 << 4) | ((cbits & 1)) ? nibble2 : nibble1);
            outVals[1] = (unsigned char)((((cbits & 2) ? nibble2 : nibble1) << 4) |
            ((cbits & 4) ? nibble2 : nibble1));
            return 2;
        case 3:
            // only single unique supported for 3 bytes
            return -1;
        case 4: // 2 uniques
            *bytesProcessed = 3;
            const int32_t thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 4) | (unsigned char)(secondByte << 4);
            val2 = (unsigned char)(secondByte >> 4) | (unsigned char)(thirdByte << 4);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            return 4;
        case 5: // 2 uniques
            *bytesProcessed = 3;
            const int32_t thirdByte5 = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 5) | (unsigned char)(secondByte << 3);
            val2 = (unsigned char)(secondByte >> 5) | (unsigned char)(thirdByte5 << 3);
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
static inline int32_t fbc264(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// -----------------------------------------------------------------------------------
// fbc264: Compress nValues bytes. Return 0 if not compressible (no output bytes),
//    -1 if error; otherwise, number of bits written to outVals.
//    Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Arguments:
//   inVals   input byte values
//   outVals  output byte values if compressed, max of inVals bytes
//   nValues  number of input byte values
// Returns number of bits compressed, 0 if not compressed, or -1 if error
{
    if (nValues <= 5)
        return fbc25(inVals, outVals, nValues);
    
    if (nValues > MAX_FBC_BYTES)
        return -1; // error
    
    uint32_t uniqueOccurrence[256]; // order of occurrence of uniques
    uint32_t nUniqueVals=0; // count of unique vals encountered
    unsigned char val256[256];
    memset(val256, 0, 256);
    
    // process input vals by finding and outputting the uniques starting at 2nd outVal
    // and encoding the repeats to be output later
    uint32_t inPos=0;
    uint32_t uniqueLimit=uniqueLimits[nValues]-1; // return uncompressible if exceeded
    uint32_t iVal; // current inVals[inPos]
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
            outVals[nUniqueVals] = (unsigned char)iVal; // store unique in output starting at second byte
        }
    } // end of loop finding unique values
 
    uint32_t i;
    uint32_t nextOut;
    uint32_t encodingByte;
    uint32_t ebShift;
    switch (nUniqueVals)
    {
        case 1:
        {
        // ********************** ALL BYTES SAME VALUE *********************
        // first byte: low-order bit indicates single value if 1
        // if single value, next bit indicates whether coded in next 6 bits or next byte
            const uint32_t ival1=inVals[0];
            if (ival1 < 64)
            {
                outVals[0] = (unsigned char)((ival1 << 2) | 3); // indicate single val and encode in high 6 bits
                return 8; // return number of bits output
            }
            else
            {
                outVals[0] = (unsigned char)(1 | (ival1 << 2));
                outVals[1] = (unsigned char)ival1 >> 6;
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
            outVals[0] = (unsigned char)encodingByte;
            ebShift=1;
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
                outVals[nextOut++] = (unsigned char)encodingByte;
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
                outVals[nextOut] = (unsigned char)encodingByte;
            }
            return (int)nValues + 21; // one bit encoding for each value + 5 indicator bits + 2 uniques
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
            outVals[0] = (unsigned char)encodingByte;
            nextOut = nUniqueVals + 1; // start output past uniques
            i = 1; // start input on second value
            while (i+3 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 6;
                outVals[nextOut++] = (unsigned char)encodingByte;
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
                outVals[nextOut] = (unsigned char)encodingByte; // output last partial byte
            }
            return (int)((nValues * 2) + 6 + (nUniqueVals * 8)); // two bits for each value plus 6 indicator bits + 3 or 4 uniques
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3 bits to encode
            encodingByte = (nUniqueVals-1) << 1; // 5 to 8 uniques
            encodingByte |= uniqueOccurrence[inVals[0]] << 5; // first val
            outVals[0] = (unsigned char)encodingByte; // save first byte
            nextOut=nUniqueVals + 1; // start output past uniques
            // process 8 values and output 3 8-bit values
            i = 1; // first val in first byte
            uint32_t partialVal;
            while (i + 7 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                encodingByte |= uniqueOccurrence[inVals[i++]] << 3;
                partialVal = inVals[i++];
                encodingByte |= uniqueOccurrence[partialVal] << 6;
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 1;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                partialVal = inVals[i++];
                encodingByte |= uniqueOccurrence[partialVal] << 7;
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 1;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 5;
                outVals[nextOut++] = (unsigned char)encodingByte;
            }
            // 0 to 7 vals left to process, check after each one for completion
            uint32_t partialByte=0;
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
                outVals[nextOut++] = (unsigned char)encodingByte;
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
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 1;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
            }
            if (partialByte)
                outVals[nextOut] = (unsigned char)encodingByte;

            return (int)((nValues * 3) + 5 + (nUniqueVals * 8)); // three bits for each value plus 5 indicator bits
            }
        default: // nUniques 9 through 16
        {
            if (nUniqueVals > MAX_UNIQUES)
                return -1;
            // cases 9 through 16 take 4 bits to encode
            // skipping last 3 bits in first byte to be on even byte boundary
            outVals[0] = (unsigned char)((nUniqueVals-1) << 1);
            nextOut = nUniqueVals + 1; // start output past uniques
            i=0;
            while (i + 3 < nValues)
            {
                outVals[nextOut++] = (unsigned char)(uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4);
                i += 2;
                outVals[nextOut++] = (unsigned char)(uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4);
                i += 2;
                outVals[nextOut++] = (unsigned char)(uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4);
                i += 2;
                outVals[nextOut++] = (unsigned char)(uniqueOccurrence[inVals[i]] | uniqueOccurrence[inVals[i+1]] << 4);
                i += 2;
            }
            if (i < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i < nValues)
                {
                    outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i+1]] << 4));
                    if (i < nValues)
                        outVals[nextOut] = (unsigned char)uniqueOccurrence[inVals[i]];
                }
                else
                    outVals[nextOut] = (unsigned char)encodingByte;
            }
            return (int)((nValues * 4) + 8 + (nUniqueVals * 8)); // four bits for each value plus 8 indicator bits + 9 to 16 uniques
        }
    }
    return -1;
} // end fbc264

// -----------------------------------------------------------------------------------
static inline int32_t fbc264d(unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, int32_t *bytesProcessed)
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
        
    const uint32_t firstByte=inVals[0];
    if (firstByte & 1)
    {
        // process single unique
        uint32_t unique = firstByte >> 2;
        if (!(firstByte & 2))
            unique |= (uint32_t)inVals[1] << 6;
        memset(outVals, unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return (int)nOriginalValues;
    }
    
    const uint32_t nUniques = ((firstByte >> 1) & 0xf) + 1;
    uint32_t uniques[MAX_UNIQUES];
    uint32_t uniques1;
    uint32_t uniques2;
    uint32_t nextInVal;
    int32_t nextOutVal;
    uint32_t inByte;
    switch (nUniques)
    {
        case 2:
        {
            // 1-bit values
            uniques1 = inVals[1];
            uniques2 = inVals[2];
            outVals[0] = (unsigned char)(((firstByte >> 5) & 1) ? uniques2 : uniques1);
            outVals[1] = (unsigned char)(((firstByte >> 6) & 1) ? uniques2 : uniques1);
            outVals[2] = (unsigned char)(((firstByte >> 7) & 1) ? uniques2 : uniques1);
            nextInVal=3;
            nextOutVal=3;
            while (nextOutVal+7 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)((inByte & 1) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 2) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 4) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 8) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 16) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 32) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 64) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 128) ? uniques2 : uniques1);
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)((inByte & 1) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 2) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 4) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 8) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 16) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 32) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 64) ? uniques2 : uniques1);
            }
            *bytesProcessed = (int)nextInVal;
            return (int)nOriginalValues;
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
            outVals[0] = (unsigned char)uniques[(firstByte >> 5)&3];
            nextOutVal = 1; // skip high bit of first byte
            while (nextOutVal + 3 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&3];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>2)&3];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)&3];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>6)&3];
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&3];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>2)&3];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)&3];
            }
            *bytesProcessed = (int)nextInVal;
            return (int)nOriginalValues;
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3-bit values for 5 to 8 uniques
            for (uint32_t i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // for 4 uniques
            outVals[0] = (unsigned char)uniques[(firstByte >> 5)&7];
            nextOutVal = 1;
            uint32_t inByte2;
            uint32_t inByte3;
            while (nextOutVal + 7 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                inByte2 = inVals[nextInVal++];
                inByte3 = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>3)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte>>6) | (inByte2<<2))&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>1)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>4)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte2>>7) | (inByte3<<1))&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte3>>2)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte3>>5)&7];
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>3)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                inByte2 = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte>>6) | (inByte2<<2))&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>1)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>4)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                inByte3 = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte2>>7) | (inByte3<<1))&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte3>>2)&7];
            }
            *bytesProcessed = (int)nextInVal;
            return (int)nOriginalValues;
        }
        default:
        {
            // 4-bit values for 9 to 16 uniques
            if (nUniques > MAX_UNIQUES)
                return -1;
            for (uint32_t i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // skip past uniques
            nextOutVal = 0;
            while (nextOutVal + 1 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
            }
            if (nextOutVal < nOriginalValues)
            {
                outVals[nextOutVal++] = (unsigned char)uniques[inVals[nextInVal++] & 0xf];
            }
            *bytesProcessed = (int)nextInVal;
            return (int)nOriginalValues;
        }
    }
    return -1;
} // end fbc264d

#endif /* fbc_h */

