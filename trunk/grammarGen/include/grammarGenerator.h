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
 * @file grammarGenerator.h
 * @brief Definition and functions for generating Schema-informed Grammar definitions
 * @date Nov 22, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#ifndef GRAMMARGENERATOR_H_
#define GRAMMARGENERATOR_H_

#include "errorHandle.h"
#include "procTypes.h"

#define SCHEMA_FORMAT_XSD_EXI           0
#define SCHEMA_FORMAT_XSD_XML           1
#define SCHEMA_FORMAT_DTD               2
#define SCHEMA_FORMAT_XSD_EXI_OPTIMIZED 3

/**
 * @brief Generate a Schema-informed Document Grammar and all Schema-informed Element and Type Grammars
 * Initial implementation is targeted at XML Schema definitions encoded with EXI with default options.
 * The grammar of the schema can be further optimized in the future.
 *
 * @param[in] binaryStream the binary representation of XML schema
 * @param[in] bufLen size of binaryStream - number of bytes
 * @param[in] schemaFormat EXI, XSD, DTD or any other schema representation supported
 * @param[out] gStack Generated grammar stack
 * @param[out] gPool Generated grammar stack
 * @return Error handling code
 */
errorCode generateSchemaInformedGrammars(char* binaryStream, uint32_t bufLen, unsigned char schemaFormat,
										EXIGrammarStack* gStack, ElementGrammarPool* gPool);

#endif /* GRAMMARGENERATOR_H_ */
