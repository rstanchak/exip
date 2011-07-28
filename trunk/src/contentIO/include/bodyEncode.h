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
 * @file bodyEncode.h
 * @brief API for encoding EXI stream body
 * @date Sep 7, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#ifndef BODYENCODE_H_
#define BODYENCODE_H_

#include "errorHandle.h"
#include "procTypes.h"
#include "schema.h"

/**** START: Serializer API implementation  ****/

// For handling the meta-data (document structure)
errorCode startDocumentSer(EXIStream* strm, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode endDocumentSer(EXIStream* strm, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode startElementSer(EXIStream* strm, QName* qname, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode endElementSer(EXIStream* strm, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode attributeSer(EXIStream* strm, QName* qname, ValueType valueType, unsigned char fastSchemaMode, size_t schemaProduction);

// For handling the data
errorCode intDataSer(EXIStream* strm, int32_t int_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode bigIntDataSer(EXIStream* strm, const BigSignedInt int_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode booleanDataSer(EXIStream* strm, unsigned char bool_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode stringDataSer(EXIStream* strm, const StringType str_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode floatDataSer(EXIStream* strm, double float_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode bigFloatDataSer(EXIStream* strm, BigFloat float_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode binaryDataSer(EXIStream* strm, const char* binary_val, size_t nbytes, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode dateTimeDataSer(EXIStream* strm, struct tm dt_val, uint16_t presenceMask, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode decimalDataSer(EXIStream* strm, decimal dec_val, unsigned char fastSchemaMode, size_t schemaProduction);
errorCode bigDecimalDataSer(EXIStream* strm, bigDecimal dec_val, unsigned char fastSchemaMode, size_t schemaProduction);

// Miscellaneous
errorCode processingInstructionSer(EXIStream* strm); // TODO: define the parameters!

// EXI specific
errorCode initStream(EXIStream* strm, char* buf, size_t bufSize, IOStream* ioStrm, EXIOptions* opts, ExipSchema *schema);
errorCode selfContainedSer(EXIStream* strm);  // Used for indexing independent elements for random access

// EXIP specific
errorCode closeEXIStream(EXIStream* strm);

/****  END: Serializer API implementation  ****/


/**
 * @brief Encodes String value into EXI stream
 * @param[in, out] strm EXI stream
 * @param[in] strng string to be written
 * @return Error handling code
 */
errorCode encodeStringData(EXIStream* strm, StringType strng);

/**
 * @brief Encodes SD, ED, EE, CH events
 * @param[in, out] strm EXI stream
 * @param[in] event event to be encoded
 * @param[in] fastSchemaMode - TRUE/FALSE, require valid schemaProduction order number
 * @param[in] schemaProduction the order number of the schema production (starting from 0), only needed if fastSchemaMode == TRUE
 * @param[out] prodType the valueType of the production hit in the grammar
 * @return Error handling code
 */
errorCode encodeSimpleEXIEvent(EXIStream* strm, EXIEvent event, unsigned char fastSchemaMode, size_t schemaProduction, ValueType* prodType);

/**
 * @brief Encodes SE, AT events
 * @param[in, out] strm EXI stream
 * @param[in] qname element or attribute QName
 * @param[in] valueType used for AT events
 * @param[in] event_all EVENT_SE_ALL or EVENT_AT_ALL
 * @param[in] event_uri EVENT_SE_URI or EVENT_AT_URI
 * @param[in] event_qname EVENT_SE_QNAME or EVENT_AT_QNAME
 * @param[in] fastSchemaMode - TRUE/FALSE, require valid schemaProduction order number
 * @param[in] schemaProduction the order number of the schema production (starting from 0), only needed if fastSchemaMode == TRUE
 * @return Error handling code
 */
errorCode encodeComplexEXIEvent(EXIStream* strm, QName* qname, EventType event_all, EventType event_uri,
						EventType event_qname, ValueType valueType, unsigned char fastSchemaMode, size_t schemaProduction);

/**
 * @brief Encodes QName into EXI stream
 * @param[in, out] strm EXI stream
 * @param[in] qname qname to be written
 * @return Error handling code
 */
errorCode encodeQName(EXIStream* strm, QName qname);

/**
 * @brief Encodes Integer value into EXI stream
 * @param[in, out] strm EXI stream
 * @param[in] int_val integer to be written
 * @param[in] intType how the integer is represented: unsigned int, small int or regular int
 * @return Error handling code
 */
errorCode encodeIntData(EXIStream* strm, int32_t int_val, ValueType intType);

#endif /* BODYENCODE_H_ */