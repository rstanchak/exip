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
 * @file headerEncode.c
 * @brief Implementing the interface of EXI header encoder
 *
 * @date Aug 23, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#include "headerEncode.h"
#include "streamWrite.h"
#include "schema.h"
#include "memManagement.h"
#include "grammars.h"
#include "sTables.h"
#include "EXISerializer.h"
#include "stringManipulate.h"

/** This is the statically generated EXIP schema definition for the EXI Options document*/
extern const ExipSchema ops_schema;

extern const EXISerializer serEXI;

errorCode encodeHeader(EXIStream* strm, EXIheader* header)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Start EXI header encoding\n"));
	if(header->has_cookie)
	{
		tmp_err_code = writeNBits(strm, 8, 36); // ASCII code for $ = 00100100  (36)
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		tmp_err_code = writeNBits(strm, 8, 69); // ASCII code for E = 01000101  (69)
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		tmp_err_code = writeNBits(strm, 8, 88); // ASCII code for X = 01011000  (88)
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		tmp_err_code = writeNBits(strm, 8, 73); // ASCII code for I = 01001001  (73)
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Encoding the header Distinguishing Bits\n"));
	tmp_err_code = writeNBits(strm, 2, 2);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Write the Presence Bit for EXI Options\n"));
	tmp_err_code = writeNextBit(strm, header->has_options);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Encode EXI version\n"));
	tmp_err_code = writeNextBit(strm, header->is_preview_version);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(header->version_number > 15)
	{
		tmp_err_code = writeNBits(strm, 4, 15);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		tmp_err_code = writeNBits(strm, 4, header->version_number - 15 - 1);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else
	{
		tmp_err_code = writeNBits(strm, 4, header->version_number - 1);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	DEBUG_MSG(INFO, DEBUG_CONTENT_IO, (">Encode EXI options\n"));
	if(header->has_options)
	{
		EXIStream options_strm;
		EXIOptions o_ops;
		EXIGrammar docGr;
		unsigned char hasUncommon = FALSE;
		unsigned char hasLesscommon = FALSE;
		unsigned char hasCommon = FALSE;

		makeDefaultOpts(&o_ops);
		o_ops.strict = TRUE;
		tmp_err_code = initAllocList(&options_strm.memList);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		options_strm.buffer = strm->buffer;
		options_strm.context.bitPointer = strm->context.bitPointer;
		options_strm.context.bufferIndx = strm->context.bufferIndx;
		options_strm.bufLen = strm->bufLen;
		options_strm.header.opts = &o_ops;
		options_strm.context.nonTermID = GR_DOCUMENT;
		options_strm.context.curr_lnID = 0;
		options_strm.context.curr_uriID = 0;
		options_strm.context.expectATData = 0;
		options_strm.bufContent = strm->bufContent;
		options_strm.ioStrm = strm->ioStrm;

		options_strm.uriTable = ops_schema.initialStringTables;
		tmp_err_code = createValueTable(&(options_strm.vTable), &(options_strm.memList));
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = createDocGrammar(&docGr, &options_strm, &ops_schema);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = pushGrammar(&options_strm.gStack, &docGr);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code += serEXI.startDocumentSer(&options_strm, TRUE, 0);
		tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);

		// uncommon options
		if(header->opts->alignment != BIT_PACKED ||
				header->opts->selfContained != FALSE ||
				header->opts->valueMaxLength != SIZE_MAX ||
				header->opts->valuePartitionCapacity != SIZE_MAX ||
				header->opts->drMap != NULL)
		{
			hasUncommon = TRUE;
			hasLesscommon = TRUE;
		}
		else if(header->opts->preserve != 0 || header->opts->blockSize != 1000000)
		{
			// lesscommon options
			hasLesscommon = TRUE;
		}

		if(hasLesscommon)
		{
			tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);
			if(hasUncommon)
			{
				tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);
				if(header->opts->alignment != BIT_PACKED)
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);
					if(header->opts->alignment == BYTE_ALIGNMENT)
						tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);
					else
						tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 1);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(header->opts->selfContained != FALSE)
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 1 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(header->opts->valueMaxLength != SIZE_MAX)
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 2 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.intDataSer(&options_strm, header->opts->valueMaxLength, TRUE, 0);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(header->opts->valuePartitionCapacity != SIZE_MAX)
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 3 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.intDataSer(&options_strm, header->opts->valuePartitionCapacity, TRUE, 0);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(header->opts->drMap != NULL)
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 4 - options_strm.context.nonTermID);
					// TODO: not ready yet!
					return NOT_IMPLEMENTED_YET;
				}
				tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 5 - options_strm.context.nonTermID);
			}
			if(header->opts->preserve != 0)
			{
				tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 1 - options_strm.context.nonTermID);
				if(IS_PRESERVED(header->opts->preserve, PRESERVE_DTD))
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(IS_PRESERVED(header->opts->preserve, PRESERVE_PREFIXES))
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 1 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(IS_PRESERVED(header->opts->preserve, PRESERVE_LEXVALUES))
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 2 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(IS_PRESERVED(header->opts->preserve, PRESERVE_COMMENTS))
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 3 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				if(IS_PRESERVED(header->opts->preserve, PRESERVE_PIS))
				{
					tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 4 - options_strm.context.nonTermID);
					tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
				}
				tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 5 - options_strm.context.nonTermID);
			}
			if(header->opts->blockSize != 1000000)
			{
				tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 2 - options_strm.context.nonTermID);
				tmp_err_code += serEXI.intDataSer(&options_strm, header->opts->blockSize, TRUE, 0);
				tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
			}
			tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 3 - options_strm.context.nonTermID);
		}

		// common options if any...
		if(header->opts->compression == TRUE || header->opts->fragment == TRUE || !isStringEmpty(&header->opts->schemaID))
		{
			hasCommon = TRUE;
		}

		if(hasCommon)
		{
			tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 1 - options_strm.context.nonTermID);
			if(header->opts->compression == TRUE)
			{
				tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 0);
				tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
			}
			if(header->opts->fragment == TRUE)
			{
				tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 1 - options_strm.context.nonTermID);
				tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
			}
			if(!isStringEmpty(&header->opts->schemaID))
			{
				tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 2 - options_strm.context.nonTermID);
				// TODO: not implemented yet
				return NOT_IMPLEMENTED_YET;
			}
			tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 3 - options_strm.context.nonTermID);
		}

		if(header->opts->strict == TRUE)
		{
			tmp_err_code += serEXI.startElementSer(&options_strm, NULL, TRUE, 2 - options_strm.context.nonTermID);
			tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 0);
		}

		tmp_err_code += serEXI.endElementSer(&options_strm, TRUE, 3 - options_strm.context.nonTermID);

		tmp_err_code += serEXI.endDocumentSer(&options_strm, TRUE, 0);

		strm->bufContent = options_strm.bufContent;
		strm->context.bitPointer = options_strm.context.bitPointer;
		strm->context.bufferIndx = options_strm.context.bufferIndx;

		if(strm->header.opts->compression == TRUE ||
					strm->header.opts->alignment != BIT_PACKED)
		{
			// Padding bits
			if(strm->context.bitPointer != 0)
			{
				strm->context.bitPointer = 0;
				strm->context.bufferIndx += 1;
			}
		}

		freeAllMem(&options_strm);

		return tmp_err_code;
	}

	return ERR_OK;
}