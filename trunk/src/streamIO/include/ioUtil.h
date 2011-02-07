/*==================================================================================*\
|                                                                                    |
|                    EXIP - Efficient XML Interchange Processor                      |
|                                                                                    |
|------------------------------------------------------------------------------------|
| Copyright (c) 2010, EISLAB - Luleå University of Technology                        |
| All rights reserved.                                                               |
|                                                                                    |
| Redistribution and use in source and binary forms, with or without                 |
| modification, are permitted provided that the following conditions are met:        |
|     * Redistributions of source code must retain the above copyright               |
|       notice, this list of conditions and the following disclaimer.                |
|     * Redistributions in binary form must reproduce the above copyright            |
|       notice, this list of conditions and the following disclaimer in the          |
|       documentation and/or other materials provided with the distribution.         |
|     * Neither the name of the EISLAB - Luleå University of Technology nor the      |
|       names of its contributors may be used to endorse or promote products         |
|       derived from this software without specific prior written permission.        |
|                                                                                    |
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND    |
| ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED      |
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE             |
| DISCLAIMED. IN NO EVENT SHALL EISLAB - LULEÅ UNIVERSITY OF TECHNOLOGY BE LIABLE    |
| FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES |
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;       |
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND        |
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT         |
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS      |
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                       |
|                                                                                    |
|                                                                                    |
|                                                                                    |
\===================================================================================*/

/**
 * @file ioUtil.h
 * @brief Common utilities for StreamIO module
 *
 * @date Oct 26, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#ifndef IOUTIL_H_
#define IOUTIL_H_

#include "procTypes.h"
#include "errorHandle.h"

/**
 * @brief Moves the BitPointer with certain positions. Takes care of byteIndex increasing when
 *        the movement cross a byte boundary
 * @param[in] strm EXI stream of bits
 * @param[in] bitPositions the number of bit positions to move the pointer
 * @return Error handling code
 */
errorCode moveBitPointer(EXIStream* strm, unsigned int bitPositions);

/**
 * @brief Determine the number of bits needed to encode a unsigned integer value
 * ⌈ log 2 m ⌉ from the spec is equal to getBitsNumber(m - 1)
 *
 * @param[in] val unsigned integer value
 *
 * @return The number of bits needed
 */
unsigned char getBitsNumber(unsigned int val);


/**
 * @brief Log2 function. Used to determine the number of bits needed to encode a unsigned integer value
 * The code taken from: http://www-graphics.stanford.edu/~seander/bithacks.html#IntegerLog
 * @param[in] val uint32_t value
 *
 * @return The number of bits needed
 */
uint32_t log2INT(uint32_t val);

#endif /* IOUTIL_H_ */