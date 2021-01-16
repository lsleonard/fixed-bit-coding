//
//  fbc.h
//  fixed bit coding implementation in static functions
//  version 1.2
//
//  Created by Stevan Leonard on 11/09/20.
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
// Notes for version 1.1:
//   1. The encoding of first byte value could be removed as the first byte is always
//   the first unique character. Changes envisioned for a future release will
//   require encoding the first value.
//
// Notes for version 1.2:
//   1. Removed output of the first input value as described in note 1 of version 1.1.
//   2. Added the macro COMPRESS_1_PERCENT to support compression of at least 1%,
//      baswed on new array uniqueLimits1.
//
// Notes for version 1.3:
//   1. Improved coding of main loop for speed.
//   2. Tweaked a few lines in encoding.
//   3. Unrolled one section in decoding of 4-bit values.
//
// Notes for version 1.4:
//   1. Added a text char mode that looks for the 15 most common characters in English plus space.
//      Defined minimum number of text chars to get 20% compression, though usually it's 25%.
//      Main loop processes 1/4 of values to determine if text mode should be processed
//      for the remaining values, which limits overhead.
//      Added static functions encodeTextMode and decodeTextMode.
//   3. Used unique count 0 to define text mode.
//   4. Defined macro TEXT_MODE to suppress text mode unless it's defined.
//   5. Removed the COMPRESS_1_PERCENT macro as results were none to minimal and speed too slow.
//   6. For next version, add text mode to 2 to 5 input values.
//
// Notes for version 1.5:
//   1. Integrated text mode into the algorithm and removed interface to enable or disable
//      text mode.
//   2. Moved the check for unique limit outside the loops on input values.
//      Limited initial loop to check 5/16 of input values for text mode and cut short
//      checking most random data.
//   3. For next version:
//      a. Add text mode to 2 to 5 input values.
//      b. Add specific error returns rather than -1 for all.
//
// Notes for version 1.6:
//   1. Added single value mode to algorithm. Repeat counts are accumulated for each value
//      and any value reaching minRepeatsSingleValueMode is identified as the single value.
//      Encoding is similar to text mode using 64 control bits. Minimum compression for 64 is 11%.
//   2. Decided not to implement text mode for 2 to 5 values. Time and space tradeoff.
//   3. Added specific error return numbers for each unexpected program error.
//   4. Changed 'f' to 'g' in the textChars predefined chars as 'g' is more common in recent text.
//
// Notes for version 1.7:
//   1. For single value mode, replace fixed bit mode if singleValueOverFixexBitRepeats.
//   2. Added 7-bit mode to catch any blocks before failure where all data have high bit clear.

#ifndef fbc_h
#define fbc_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FBC_BYTES 64  // max input vals supported
#define MIN_FBC_BYTES 2  // min input vals supported
#define MAX_UNIQUES 16 // max uniques supported in input

// ----------------------------------------------
// for the number of uniques in input, the minimum number of input values for 25% compression
// uniques   1  2  3  4  5   6   7   8   9   10  11  12  13  14  15  16
// nvalues   2, 4, 7, 9, 15, 17, 19, 23, 40, 44, 48, 52, 56, 60, 62, 64};
static const uint32_t uniqueLimits25[MAX_FBC_BYTES+1]=
//      2    4      7    9            15   17   19       23
{ 0, 0, 1,1, 2,2,2, 3,3, 4,4,4,4,4,4, 5,5, 6,6, 7,7,7,7, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
//  40       44           48           52           56           60     62     64
    9,9,9,9, 10,10,10,10, 11,11,11,11, 12,12,12,12, 13,13,13,13, 14,14, 15,15, 16
};

// number of predefined text characters in inVals for number of input values (2 to 64) to have 20% compression
static const uint32_t textLimits25[MAX_FBC_BYTES+1]=
{
//  0  1  2  3  4  5  6  7  8  9 10 11 12  13  14  15  16
    0, 0, 2, 2, 3, 3, 3, 5, 6, 7, 7, 8, 9, 10, 11, 12, 12,
//  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32
    13, 14, 14, 15, 16, 16, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23,
//  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48
    24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34,
//  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64
    34, 35, 36, 36, 37, 38, 38, 39, 40, 40, 41, 42, 42, 43, 44, 44
};

#define MAX_PREDEFINED_CHAR_COUNT 16 // based on frequency of characters from Morse code
static const uint32_t textChars[MAX_PREDEFINED_CHAR_COUNT]={
    ' ', 'e', 't', 'a', 'i', 'n', 'o', 's', 'h', 'r', 'd', 'l', 'u', 'c', 'm', 'g'
};

// a one indicates a character from the most frequent characters based on Morse code set
static const uint32_t predefinedTextChars[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// index to predefined char or 16 if another value
static const uint32_t textEncoding[256]={
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 3, 16, 13, 10, 1, 16, 15, 8, 4, 16, 16, 11, 14, 5, 6,
    16, 16, 9, 7, 2, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};

// -----------------------------------------------------------------------------------
static inline int32_t fbc25(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// -----------------------------------------------------------------------------------
// Compress 2 to 5 values with 1 or 2 unique values
// Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Decode first byte:
// 1 = single unique, followed by 1 if upper two bits are 00 and low 6 bits of input; otherwise, high two bits in second byte
// 0 = encoding by number of values:
//    2 bytes: two unique nibbles
//    3 bytes: two unique nibbles
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
            // check for two unique nibbles
            uint32_t outBits=0; // keep track of which nibbles match nibble1 (0) or otherVal (1)
            uint32_t otherVal=256;
            const uint32_t nibble1=ival1 >> 4;
            const uint32_t nibble2=ival1 & 0xf;
            const uint32_t nibble3=(unsigned char)inVals[1] >> 4;
            if (nibble2 != nibble1)
            {
                otherVal = nibble2;
                outBits = 2;
            }
            if (nibble3 != nibble1)
            {
                if ((nibble3 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble3;
                outBits |= 4; // set this bit to other value
            }
            const uint32_t nibble4=(unsigned char)inVals[1] & 0xf;
            if (nibble4 != nibble1)
            {
                if ((nibble4 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == 256)
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
            // check for two unique nibbles
            uint32_t outBits=0; // keep track of which nibbles match nibble1 or otherVal
            uint32_t otherVal=256;
            const uint32_t nibble1=(unsigned char)ival1 >> 4;
            const uint32_t nibble2=(unsigned char)ival1 & 0xf;
            const uint32_t nibble3=(unsigned char)inVals[1] >> 4;
            if (nibble2 != nibble1)
            {
                otherVal = nibble2;
                outBits = 2;
            }
            if (nibble3 != nibble1)
            {
                if ((nibble3 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble3;
                outBits |= 4; // set this bit to other value
            }
            const uint32_t nibble4=(unsigned char)inVals[1] & 0xf;
            if (nibble4 != nibble1)
            {
                if ((nibble4 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble4;
                outBits |= 8;  // set this bit to other value
            }
            const uint32_t nibble5=(unsigned char)inVals[2] >> 4;
            if (nibble5 != nibble1)
            {
                if ((nibble5 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble5;
                outBits |= 16;  // set this bit to other value
            }
            const uint32_t nibble6=(unsigned char)inVals[2] & 0xf;
            if (nibble6 != nibble1)
            {
                if ((nibble6 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble6;
                outBits |= 32;  // set this bit to other value
            }
            outVals[0] = (unsigned char)(nibble1 << 6) | (unsigned char)outBits;
            outVals[1] = (unsigned char)(otherVal << 2) | (unsigned char)(nibble1 >> 2);
            return 14; // save 10 bits
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
            return -2; // number of values specified not supported (2 to 5 handled here)
    }
} // end fbc25

// -----------------------------------------------------------------------------------
static inline int32_t fbc25d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
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
    const uint32_t firstByte=(unsigned char)inVals[0];
    if (firstByte & 1)
    {
        // process single unique
        uint32_t unique = firstByte >> 2;
        if (!(firstByte & 2))
            unique |= (unsigned char)(inVals[1] << 6);
        memset(outVals, (unsigned char)unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return (int)nOriginalValues;
    }
    uint32_t val1;
    uint32_t val2;
    const uint32_t secondByte = (unsigned char)inVals[1];
    switch (nOriginalValues)
    {
            // special encoding for 2 to 5 bytes
        case 2:
        {
            // decode nibble compression for 2 bytes
            // nibble1 nibble 2   nibble3 nibble4
            // high    low        high    low
            // first byte         second byte
            *bytesProcessed = 2;
            const uint32_t cbits = (unsigned char)(firstByte >> 1) & 0x7; // 3 control bits
            const uint32_t nibble1 = (unsigned char)firstByte >> 4;
            const uint32_t nibble2 = (unsigned char)secondByte & 0xf;
            outVals[0] = (unsigned char)(nibble1 << 4) | (unsigned char)((cbits & 1) ? nibble2 : nibble1);
            outVals[1] = (unsigned char)((((cbits & 2) ? nibble2 : nibble1) << 4) |
            (unsigned char)((cbits & 4) ? nibble2 : nibble1));
            return 2;
        }
        case 3:
        {
            // decode nibble compression for 3 bytes
            // nibble1 nibble 2   nibble3 nibble4   nibble5 nibble6
            // high    low        high    low       high    low
            // first byte         second byte       third byte
            *bytesProcessed = 2;
            const uint32_t cbits = ((unsigned char)firstByte >> 1) & 0x1f; // 5 control bits
            const uint32_t nibble1 = ((unsigned char)firstByte >> 6 | ((unsigned char)secondByte << 2)) & 0xf;
            const uint32_t nibble2 = ((unsigned char)secondByte >> 2) & 0xf;
            outVals[0] = (unsigned char)(nibble1 << 4) | (unsigned char)((cbits & 1) ? nibble2 : nibble1);
            outVals[1] = (unsigned char)((((cbits & 2) ? nibble2 : nibble1) << 4) |
            (unsigned char)((cbits & 4) ? nibble2 : nibble1));
            outVals[2] = (unsigned char)((((cbits & 8) ? nibble2 : nibble1) << 4) |
            (unsigned char)((cbits & 16) ? nibble2 : nibble1));
            return 3;
        }
        case 4: // 2 uniques
        {
            *bytesProcessed = 3;
            const int32_t thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 4) | (unsigned char)(secondByte << 4);
            val2 = (unsigned char)(secondByte >> 4) | (unsigned char)(thirdByte << 4);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            return 4;
        }
        case 5: // 2 uniques
        {
            *bytesProcessed = 3;
            const int32_t thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 5) | (unsigned char)(secondByte << 3);
            val2 = (unsigned char)(secondByte >> 5) | (unsigned char)(thirdByte << 3);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[4] = (firstByte & 0x10) ? (unsigned char)val2 : (unsigned char)val1;
            return 5;
        }
        default:
            return -3; // unexpected program error
    }
} // end fbc25d

// -----------------------------------------------------------------------------------
static inline int32_t encodeTextMode(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// -----------------------------------------------------------------------------------
{
    // if value is predefined, use its index; otherwise, output 8-bit value
    // generate control bit 1 if predefined text char, 0 if 8-bit value
    unsigned char *pInVal=inVals;
    unsigned char *pLastInValPlusOne=inVals+nValues;
    uint32_t inVal;
    uint32_t nextOutVal=(nValues-1)/8+2; // allocate space for control bits in bytes following first
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    uint32_t predefinedTCs=0; // two predefined text chars (PTCs), on init, written to outVals[0]
    uint32_t predefinedTCnt=2; // indicate whether first PTC is encoded for output
    uint32_t predefinedTCsOut=0; // write a 0 to outVals[0] when first PTC encountered
    
    while (pInVal < pLastInValPlusOne)
    {
        if (textEncoding[(inVal=*(pInVal++))] < 16)
        {
            // encode 4-bit predefined index textIndex
            controlBit <<= 1; // control bit gets 0 for text index
            if (predefinedTCnt == 2)
            {
                // write a 0 to outVals[0] when first PTC encountered
                outVals[predefinedTCsOut] = (unsigned char)predefinedTCs;
                predefinedTCsOut = nextOutVal++;
                predefinedTCs = textEncoding[inVal];
                predefinedTCnt = 1;
            }
            else
            {
                predefinedTCs |= textEncoding[inVal] << 4;
                predefinedTCnt++;
            }
        }
        else
        {
            // encode 8-bit value
            // controlByte gets a 1 at this bit position for next undefined value
            controlByte |= controlBit;
            controlBit <<= 1;
            outVals[nextOutVal++] = (unsigned char)inVal;
        }
    }
    if (nextOutVal >= nValues)
    {
        // have to check this as at least 1/4 of values must be text chars, and main loop
        // verifies only 1/2 of 5/16 of data values
        return 0; // data failed to compress
    }

    // output control bytes for nValues
    switch ((nValues-1)/8)
    {
        case 0:
            outVals[1] = (unsigned char)controlByte;
            break;
        case 1:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            break;
        case 2:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            break;
        case 3:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            break;
        case 4:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            break;
        case 5:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            break;
        case 6:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            break;
        case 7:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            outVals[8] = (unsigned char)(controlByte>>56);
            break;
    }
    // output last byte of text char encoding
    outVals[predefinedTCsOut] = (unsigned char)predefinedTCs;
    
    return (int32_t)nextOutVal * 8; // round up to full byte
} // end encodeTextMode

// -----------------------------------------------------------------------------------
static inline int32_t encodeSingleValueMode(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, int32_t singleValue)
// -----------------------------------------------------------------------------------
{
    // generate control bit 1 if single value, otherwise 0 plus 8-bit value
    unsigned char *pInVal=inVals;
    unsigned char *pLastInValPlusOne=inVals+nValues;
    uint32_t inVal;
    uint32_t nextOutVal=(nValues-1)/8+2; // allocate space for control bits in bytes following first
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    
    // output indicator byte, number uniques is 0, followed by next bit set
    outVals[0] = 0x20; // set 6th bit to indicate single value mode versus text mode
    outVals[nextOutVal++] = (unsigned char)singleValue;
    while (pInVal < pLastInValPlusOne)
    {
        if ((inVal=*(pInVal++)) != (uint32_t)singleValue)
        {
            // encode 4-bit predefined index textIndex
            controlBit <<= 1;
            outVals[nextOutVal++] = (unsigned char)inVal;
        }
        else
        {
            // controlByte gets a 1 at this bit position to indicate single value
            controlByte |= controlBit;
            controlBit <<= 1;
        }
    }

    // output control bytes for nValues
    switch ((nValues-1)/8)
    {
        case 0:
            outVals[1] = (unsigned char)controlByte;
            break;
        case 1:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            break;
        case 2:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            break;
        case 3:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            break;
        case 4:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            break;
        case 5:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            break;
        case 6:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            break;
        case 7:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            outVals[8] = (unsigned char)(controlByte>>56);
            break;
    }
    
    return (int32_t)nextOutVal * 8; // round up to full byte
} // end encodeSingleValueMode

static inline int32_t encode7bits(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
{
    uint32_t nextOutVal=1;
    uint32_t nextInVal=0;
    uint32_t val1;
    uint32_t val2;
    
    outVals[0] = 0x40; // indicate 7-bit mode
    // process groups of 8 bytes to output 7 bytes
    while (nextInVal + 7 < nValues)
    {
        // copy groups of 8 7-bit vals into 7 bytes
        val1 = inVals[nextInVal++];
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(val1 | (val2 << 7));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val2 >> 1) | (val1 << 6));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val1 >> 2) | (val2 << 5));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val2 >> 3) | (val1 << 4));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val1 >> 4) | (val2 << 3));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val2 >> 5) | (val1 << 2));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val1 >> 6) | (val2 << 1));
    }
    while (nextInVal < nValues)
    {
        // output final values as full bytes because no bytes saved, only bits
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
            break;
    }
    return (int32_t)nextOutVal*8;
} // end encode7bits

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
        return -1; // only values 2 to 64 supported
    
    unsigned char *pInVal;
    uint32_t highBitCheck=0;
    uint32_t predefinedTextCharCnt=0; // count of text chars encountered
    uint32_t uniqueOccurrence[256]; // order of occurrence of uniques
    uint32_t nUniqueVals=0; // count of unique vals encountered
    unsigned char val256[256];
    const uint32_t uniqueLimit=uniqueLimits25[nValues]; // if exceeded, return uncompressible by fixed bit coding
    memset(val256, 0, sizeof(val256));
    const uint32_t nValsInitLoop=(nValues*5/16)+1;
    const unsigned char *pLastInValPlusOne=inVals+nValues;
    const unsigned char *pLimitInVals=inVals+nValsInitLoop; // fewest values to test for text mode

    // process enough input vals to eliminate most random data and to check for text mode
    // for fixed bit coding find and output the uniques starting at outVal[1]
    //    and saving the unique occurrence value to be used when values are output
    // for 7 bit mode OR every value
    // for text mode count number of text characters
    // for single value mode accumulate frequency counts
    pInVal = inVals;
    while (pInVal < pLimitInVals)
    {
        uint32_t inVal=*(pInVal++);
        highBitCheck |= inVal;
        predefinedTextCharCnt += predefinedTextChars[inVal]; // count text chars for text char mode
        if (val256[inVal]++ == 0)
        {
            // first occurrence of value, for fixed bit coding:
            uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
            outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
        }
    }
    if (nUniqueVals > uniqueLimit)
    {
        // supported unique values exceeded
        if ((highBitCheck & 0x80) == 0)
        {
            // attempt to compress based on high bit clear across all values
            // confirm remaining values have high bit clear
            while (pInVal < pLastInValPlusOne)
                highBitCheck |= *pInVal++;
            if ((highBitCheck & 0x80) == 0)
                return encode7bits(inVals, outVals, nValues);
        }
        return 0; // too many uniques to compress with fixed bit coding, random data fails here
    }
    if (nUniqueVals > uniqueLimits25[nValsInitLoop] * 3/4 + 1)
    {
        // make sure at least 3/4 of uniques for the initial number of values is exceeded
        //    because fixed bit coding will produce higher compression
        // check text mode validity as it's not considered after this
        // text mode will have 3/4 text values, but for smaller numbers, use .5
        // this number of values will almost always compress, though 1/4 of data must be text
        // chars to compress; encodeTextMode verifies compression occurs
        if (predefinedTextCharCnt > nValsInitLoop / 2)
        {
            // compress in text mode
            return encodeTextMode(inVals, outVals, nValues);
        }
    }
    // continue fixed bit loop with checks for high bit set and repeat counts
    // look for minimum count to validate single value mode
    const uint32_t singleValueOverFixexBitRepeats=nValsInitLoop*3/2;
    const uint32_t minRepeatsSingleValueMode=(unsigned char)nValues/4+1;
    int32_t singleValue=-1; // look for value that repeats MIN_REPEATS_SINGLE_VALUE_MODE
    while (pInVal < pLastInValPlusOne)
    {
        uint32_t inVal=*(pInVal++);
        highBitCheck |= inVal;
        if (val256[inVal]++ == 0)
        {
            // first occurrence of value, for fixed bit coding:
            uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
            outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
        }
        else if (val256[inVal] >= minRepeatsSingleValueMode)
        {
            singleValue = (int32_t)inVal;
            break; // continue loop without further checking
        }
    }
    // continue fixed bit loop with checks for high bit set and repeat counts
    while (pInVal < pLastInValPlusOne)
    {
        uint32_t inVal=*(pInVal++);
        highBitCheck |= inVal;
        if (val256[inVal]++ == 0)
        {
            // first occurrence of value, for fixed bit coding:
            uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
            outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
        }
    }
    if (nUniqueVals > uniqueLimit)
    {
        // fixed bit coding fails, try for other compression modes
        if (singleValue >= 0)
        {
            return encodeSingleValueMode(inVals, outVals, nValues, singleValue);
        }
        if ((highBitCheck & 0x80) == 0)
            return encode7bits(inVals, outVals, nValues);
        return 0; // too many uniques to compress
    }
    else if ((nUniqueVals > 8) && (val256[singleValue] >= singleValueOverFixexBitRepeats))
    {
        // favor single value over 12 value fixed 4-bit encoding
        return encodeSingleValueMode(inVals, outVals, nValues, singleValue);
    }

    // process fixed bit coding
    uint32_t i;
    uint32_t nextOut;
    uint32_t encodingByte;
    uint32_t ebShift;

    switch (nUniqueVals)
    {
        case 0:
            return -4; // unexpected program error
        case 1:
        {
        // ********************** ALL BYTES SAME VALUE *********************
        // first byte: low-order bit indicates single value if 1
        // if single value, next bit indicates whether coded in next 6 bits or next byte
            if (inVals[0] < 64)
            {
                outVals[0] = (unsigned char)(inVals[0] << 2) | 3; // indicate single val and encode in high 6 bits
                return 8; // return number of bits output
            }
            else
            {
                outVals[0] = (unsigned char)(1 | (inVals[0] << 2));
                outVals[1] = inVals[0] >> 6;
                return 10; // return number of bits output
            }
        }
        case 2:
        {
            // 2 uniques so 1 bit required for each input value
            encodingByte=2; // indicate 2 uniques in bits 1-4
            // fill in upper 3 bits for inputs 2, 3 and 4 (first is implied by first unique) and output
            encodingByte |= uniqueOccurrence[inVals[1]] << 5;
            encodingByte |= uniqueOccurrence[inVals[2]] << 6;
            encodingByte |= uniqueOccurrence[inVals[3]] << 7;
            outVals[0] = (unsigned char)encodingByte;
            i = 4;
            nextOut = 3;
            while (i + 7 < nValues)
            {
                encodingByte = (uniqueOccurrence[inVals[i++]]);
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 1;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 2;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 3;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 4;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 5;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 6;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 7;
                outVals[nextOut++] = (unsigned char)encodingByte;
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
            return (int)nValues-1 + 21; // one bit encoding for each value + 5 indicator bits + 2 uniques
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
            encodingByte |= uniqueOccurrence[inVals[1]] << 5;
            // skipping last bit in first byte to be on even byte boundary
            outVals[0] = (unsigned char)encodingByte;
            nextOut = nUniqueVals + 1; // start output past uniques
            i = 2; // start input on third value (first is implied by first unique)
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
            return (int)(((nValues-1) * 2) + 6 + (nUniqueVals * 8)); // two bits for each value plus 6 indicator bits + 3 or 4 uniques
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3 bits to encode
            encodingByte = (nUniqueVals-1) << 1; // 5 to 8 uniques
            encodingByte |= uniqueOccurrence[inVals[1]] << 5; // first val
            outVals[0] = (unsigned char)encodingByte; // save first byte
            nextOut=nUniqueVals + 1; // start output past uniques
            // process 8 values and output 3 8-bit values
            i = 2; // second val in first byte (first val is implied by first unique)
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

            return (int)(((nValues-1) * 3) + 5 + (nUniqueVals * 8)); // three bits for each value plus 5 indicator bits
            }
        default: // nUniques 9 through 16
        {
            if (nUniqueVals > MAX_UNIQUES)
                return -5; // unexpected program error
            // cases 9 through 16 take 4 bits to encode
            // skipping last 3 bits in first byte to be on even byte boundary
            outVals[0] = (unsigned char)((nUniqueVals-1) << 1);
            nextOut = nUniqueVals + 1; // start output past uniques
            i = 1; // first value is implied by first unique
            while (i + 7 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
            }
            while (i < nValues)
            {
                // 1 to 7 values remain
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i < nValues)
                {
                    outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                }
                else
                {
                    outVals[nextOut] = (unsigned char)encodingByte;
                    break;
                }
            }
            return (int)(((nValues-1) * 4) + 8 + (nUniqueVals * 8)); // four bits for each value plus 8 indicator bits + 9 to 16 uniques
        }
    }
    return -6; // unexpected program error
} // end fbc264

// -----------------------------------------------------------------------------------
static inline int32_t decodeTextMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
{
    uint32_t nextInVal=(nOriginalValues-1)/8+2;
    uint32_t nextOutVal=0;
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    uint32_t predefinedTCs=0;
    uint32_t predefinedTCnt=1; // 1 = first 4-bit PTC is encoded for output, otherwise no

    // read in control bits starting from second byte
    switch (nextInVal-1)
    {
        case 1:
            controlByte = inVals[1];
            break;
        case 2:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            break;
        case 3:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            break;
        case 4:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            break;
        case 5:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            break;
        case 6:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            break;
        case 7:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            break;
        case 8:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            controlByte |= (uint64_t)inVals[8]<<56;
            break;
    }
    while (nextOutVal < nOriginalValues)
    {
        if (controlByte & controlBit)
        {
            // read in and output next 8-bit value
            outVals[nextOutVal++] = inVals[nextInVal++];
        }
        else
        {
            // use predefined text chars based on 4-bit index
            if (predefinedTCnt == 1)
            {
                predefinedTCs = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)textChars[(unsigned char)predefinedTCs & 15];
                predefinedTCnt = 0;
            }
            else
            {
                outVals[nextOutVal++] = (unsigned char)textChars[predefinedTCs >> 4];
                predefinedTCnt++;
            }
        }
        controlBit <<= 1;
    }
    *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decodeTextMode

// -----------------------------------------------------------------------------------
static inline int32_t decodeSingleValueMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
{
    uint32_t nextInVal=(nOriginalValues-1)/8+2;
    uint32_t nextOutVal=0;
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    unsigned char singleValue;

    // read in control bits starting from second byte
    switch (nextInVal-1)
    {
        case 1:
            controlByte = inVals[1];
            break;
        case 2:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            break;
        case 3:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            break;
        case 4:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            break;
        case 5:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            break;
        case 6:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            break;
        case 7:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            break;
        case 8:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            controlByte |= (uint64_t)inVals[8]<<56;
            break;
    }
    singleValue = inVals[nextInVal++]; // single value output when control bit is 1
    while (nextOutVal < nOriginalValues)
    {
        if (controlByte & controlBit)
        {
            // output single value
            outVals[nextOutVal++] = singleValue;
        }
        else
        {
            // output next value from input
            outVals[nextOutVal++] = inVals[nextInVal++];
        }
        controlBit <<= 1;
    }
    *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decodeTextMode

static inline int32_t decode7bits(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextOutVal=0;
    uint32_t nextInVal=1;
    uint32_t val1;
    uint32_t val2;
    
    // process groups of 8 bytes to output 7 bytes
    while (nextOutVal + 7 < nOriginalValues)
    {
        // move groups of 7-bit vals into 8 bytes
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(val1 & 127);
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val2 << 1) & 127) | (val1 >> 7));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val1 << 2) & 127) | (val2 >> 6));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val2 << 3) & 127) | (val1 >> 5));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val1 << 4) & 127) | (val2 >> 4));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val2 << 5) & 127) | (val1 >> 3));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val1 << 6) & 127) | (val2 >> 2));
        outVals[nextOutVal++] = (unsigned char)val1 >> 1;
    }
    while (nextOutVal++ < nOriginalValues)
    {
        // output final values as full bytes because no bytes saved, only bits
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
            break;
    }
    *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decode7bits

// -----------------------------------------------------------------------------------
static inline int32_t fbc264d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
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
        
    const unsigned char firstByte=inVals[0];
    if (firstByte & 1)
    {
        // process single unique
        unsigned char unique = firstByte >> 2;
        if (!(firstByte & 2))
            unique |= inVals[1] << 6;
        memset(outVals, unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return (int)nOriginalValues;
    }
    
    const uint32_t nUniques = ((firstByte >> 1) & 0xf) + 1;
    uint32_t uniques[MAX_UNIQUES];
    unsigned char uniques1;
    unsigned char uniques2;
    uint32_t nextInVal;
    int32_t nextOutVal;
    unsigned char inByte;
    switch (nUniques)
    {
        case 1:
            if (firstByte & 0x20)
            {
                // single value mode
                return decodeSingleValueMode(inVals, outVals, nOriginalValues, bytesProcessed);
            }
            if (firstByte & 0x40)
            {
                // 7-bit mode
                return decode7bits(inVals, outVals, nOriginalValues, bytesProcessed);
            }
            // 0 in 6th bit: text mode using predefined text chars
            return decodeTextMode(inVals, outVals, nOriginalValues, bytesProcessed);
        case 2:
        {
            // 1-bit values
            uniques1 = inVals[1];
            uniques2 = inVals[2];
            outVals[0] = (unsigned char)uniques1;
            outVals[1] = (unsigned char)(((firstByte >> 5) & 1) ? uniques2 : uniques1);
            outVals[2] = (unsigned char)(((firstByte >> 6) & 1) ? uniques2 : uniques1);
            outVals[3] = (unsigned char)(((firstByte >> 7) & 1) ? uniques2 : uniques1);
            nextInVal=3;
            nextOutVal=4;
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
            *bytesProcessed = nextInVal;
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
            outVals[0] = (unsigned char)uniques[0];
            outVals[1] = (unsigned char)uniques[(firstByte >> 5)&3];
            nextOutVal = 2; // skip high bit of first byte
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
            *bytesProcessed = nextInVal;
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
            outVals[0] = (unsigned char)uniques[0];
            outVals[1] = (unsigned char)uniques[(firstByte >> 5)&7];
            nextOutVal = 2;
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
            *bytesProcessed = nextInVal;
            return (int)nOriginalValues;
        }
        default:
        {
            // 4-bit values for 9 to 16 uniques
            if (nUniques > MAX_UNIQUES)
                return -7; // unexpected program error
            for (uint32_t i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // skip past uniques
            outVals[0] = (unsigned char)uniques[0];
            nextOutVal = 1;
            while (nextOutVal + 3 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
            }
            if (nextOutVal < nOriginalValues)
            {
                // 1 to 3 values remain
                if (nextOutVal + 1 < nOriginalValues)
                {
                    // 2 values
                    inByte = inVals[nextInVal++];
                    outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                    outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
                }
                if (nextOutVal < nOriginalValues)
                    outVals[nextOutVal++] = (unsigned char)uniques[inVals[nextInVal++] & 0xf];
            }
            *bytesProcessed = nextInVal;
            return (int)nOriginalValues;
        }
    }
    return -8; // unexpected program error
} // end fbc264d

#endif /* fbc_h */

