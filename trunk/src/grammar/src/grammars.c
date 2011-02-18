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
 * @file grammars.c
 * @brief Defines grammar related functions
 * @date Sep 13, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id$
 */

#ifndef BUILTINDOCGRAMMAR_H_
#define BUILTINDOCGRAMMAR_H_

#include "grammars.h"
#include "procTypes.h"
#include "hashtable.h"
#include "hashUtils.h"
#include "sTables.h"
#include "streamEncode.h"
#include "streamDecode.h"
#include "memManagement.h"
#include "ioUtil.h"
#include <stdio.h>

#define DEF_DOC_GRAMMAR_RULE_NUMBER 3
#define DEF_ELEMENT_GRAMMAR_RULE_NUMBER 2
#define GRAMMAR_POOL_DIMENSION 16

errorCode createDocGrammar(struct EXIGrammar* docGrammar, EXIStream* strm, ExipSchema* schema)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	unsigned int n = 0; // first part of the event codes in the second rule

	//TODO: depends on the EXI fidelity options! Take this into account
	// For now only the default fidelity_opts pruning is supported - all preserve opts are false
	char is_default_fidelity = 0;

	if(strm->header.opts->preserve == 0) //all preserve opts are false
		is_default_fidelity = 1;

	docGrammar->lastNonTermID = GR_VOID_NON_TERMINAL;
	docGrammar->nextInStack = NULL;
	docGrammar->rulesDimension = DEF_DOC_GRAMMAR_RULE_NUMBER;
	docGrammar->ruleArray = (GrammarRule*) memManagedAllocate(&strm->memList, sizeof(GrammarRule)*DEF_DOC_GRAMMAR_RULE_NUMBER);
	if(docGrammar->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	/* Document : SD DocContent	0 */
	tmp_err_code = initGrammarRule(&(docGrammar->ruleArray[0]), &strm->memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	docGrammar->ruleArray[0].nonTermID = GR_DOCUMENT;
	docGrammar->ruleArray[0].bits[0] = 0;
	tmp_err_code = addProduction(&(docGrammar->ruleArray[0]), getEventCode1(0), getEventDefType(EVENT_SD), GR_DOC_CONTENT);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	tmp_err_code = initGrammarRule(&(docGrammar->ruleArray[1]), &strm->memList);
	if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	docGrammar->ruleArray[1].nonTermID = GR_DOC_CONTENT;

	if(schema != NULL)   // Creates Schema Informed Grammar
	{
		unsigned int e = 0;

		/*
		   DocContent :
					   	SE (G-0)   DocEnd	0
						SE (G-1)   DocEnd	1
						⋮	⋮      ⋮
						SE (G-n−1) DocEnd n-1
					//	SE (*)     DocEnd	n		//  This is created as part of the Build-In grammar down
					//	DT DocContent   	n+1.0	//  This is created as part of the Build-In grammar down
					//	CM DocContent		n+1.1.0	//  This is created as part of the Build-In grammar down
					//	PI DocContent		n+1.1.1	//  This is created as part of the Build-In grammar down
		 */

		for(e = 0; e < schema->globalElemGrammars.count; e++)
		{
			tmp_err_code = addProduction(&(docGrammar->ruleArray[1]), getEventCode1(e), getEventDefType(EVENT_SE_QNAME), GR_DOC_END);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			docGrammar->ruleArray[1].prodArray[docGrammar->ruleArray[1].prodCount - 1].lnRowID = schema->globalElemGrammars.elems[e].lnRowId;
			docGrammar->ruleArray[1].prodArray[docGrammar->ruleArray[1].prodCount - 1].uriRowID = schema->globalElemGrammars.elems[e].uriRowId;
		}
		n = schema->globalElemGrammars.count;
	}

	/*
	   DocContent :
					SE (*) DocEnd	0
					DT DocContent	1.0
					CM DocContent	1.1.0
					PI DocContent	1.1.1
	 */


	if(is_default_fidelity)
	{
		docGrammar->ruleArray[1].bits[0] = getBitsNumber(n);
	}
	else
	{
		docGrammar->ruleArray[1].bits[0] = getBitsNumber(n + 1);
		docGrammar->ruleArray[1].bits[1] = 1;
		docGrammar->ruleArray[1].bits[2] = 1;
	}

	/* SE (*) DocEnd	0 */
	tmp_err_code = addProduction(&(docGrammar->ruleArray[1]), getEventCode1(n), getEventDefType(EVENT_SE_ALL), GR_DOC_END);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(is_default_fidelity == 0)
	{
		/* DT DocContent	1.0 */
		tmp_err_code = addProduction(&(docGrammar->ruleArray[1]), getEventCode2(n + 1, 0), getEventDefType(EVENT_DT), GR_DOC_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* CM DocContent	1.1.0 */
		tmp_err_code = addProduction(&(docGrammar->ruleArray[1]), getEventCode3(n + 1, 1, 0), getEventDefType(EVENT_CM), GR_DOC_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* PI DocContent	1.1.1 */
		tmp_err_code = addProduction(&(docGrammar->ruleArray[1]), getEventCode3(n + 1, 1, 1), getEventDefType(EVENT_PI), GR_DOC_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}


	/* DocEnd :
				ED	        0
				CM DocEnd	1.0
				PI DocEnd	1.1 */
	tmp_err_code = initGrammarRule(&(docGrammar->ruleArray[2]), &strm->memList);
	if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	docGrammar->ruleArray[2].nonTermID = GR_DOC_END;
	if(is_default_fidelity == 1)
	{
		docGrammar->ruleArray[2].bits[0] = 0;
	}
	else
	{
		docGrammar->ruleArray[2].bits[0] = 1;
		docGrammar->ruleArray[2].bits[1] = 1;
	}

	/* ED	0 */
	tmp_err_code = addProduction(&(docGrammar->ruleArray[2]), getEventCode1(0), getEventDefType(EVENT_ED), GR_VOID_NON_TERMINAL);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(is_default_fidelity == 0)
	{
		/* CM DocEnd	1.0  */
		tmp_err_code = addProduction(&(docGrammar->ruleArray[2]), getEventCode2(1, 0), getEventDefType(EVENT_CM), GR_DOC_END);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* PI DocEnd	1.1 */
		tmp_err_code = addProduction(&(docGrammar->ruleArray[2]), getEventCode2(1, 1), getEventDefType(EVENT_PI), GR_DOC_END);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	return ERR_OK;
}

errorCode createBuildInElementGrammar(struct EXIGrammar* elementGrammar, EXIStream* strm)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;

	//TODO: depends on the EXI fidelity options! Take this into account
	// For now only the default fidelity_opts pruning is supported - all preserve opts are false
	// and selfContained is also false
	char is_default_fidelity = 0;

	if(strm->header.opts->preserve == 0 && strm->header.opts->selfContained == 0) //all preserve opts are false and selfContained is also false
		is_default_fidelity = 1;

	elementGrammar->lastNonTermID = GR_VOID_NON_TERMINAL;
	elementGrammar->nextInStack = NULL;
	elementGrammar->rulesDimension = DEF_ELEMENT_GRAMMAR_RULE_NUMBER;
	elementGrammar->ruleArray = (GrammarRule*) memManagedAllocate(&strm->memList, sizeof(GrammarRule)*DEF_ELEMENT_GRAMMAR_RULE_NUMBER);
	if(elementGrammar->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	/* StartTagContent :
							EE	                    0.0
							AT (*) StartTagContent	0.1
							NS StartTagContent	    0.2
							SC Fragment	            0.3
							SE (*) ElementContent	0.4
							CH ElementContent	    0.5
							ER ElementContent	    0.6
							CM ElementContent	    0.7.0
							PI ElementContent	    0.7.1 */
	tmp_err_code = initGrammarRule(&(elementGrammar->ruleArray[0]), &strm->memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	elementGrammar->ruleArray[0].nonTermID = GR_START_TAG_CONTENT;
	if(is_default_fidelity == 1)
	{
		elementGrammar->ruleArray[0].bits[0] = 0;
		elementGrammar->ruleArray[0].bits[1] = 2;
	}
	else
	{
		elementGrammar->ruleArray[0].bits[0] = 0;
		elementGrammar->ruleArray[0].bits[1] = 4;
		elementGrammar->ruleArray[0].bits[2] = 1;
	}

	/* EE	                    0.0 */
	tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	/* AT (*) StartTagContent	0.1 */
	tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,1), getEventDefType(EVENT_AT_ALL), GR_START_TAG_CONTENT);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(is_default_fidelity == 1)
	{
		/* SE (*) ElementContent	0.2 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,2), getEventDefType(EVENT_SE_ALL), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* CH ElementContent	    0.3 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,3), getEventDefType(EVENT_CH), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

	}
	else
	{
		/* NS StartTagContent	    0.2 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,2), getEventDefType(EVENT_NS), GR_START_TAG_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* SC Fragment	            0.3 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,3), getEventDefType(EVENT_SC), GR_FRAGMENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* SE (*) ElementContent	0.4 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,4), getEventDefType(EVENT_SE_ALL), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* CH ElementContent	    0.5 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,5), getEventDefType(EVENT_CH), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* ER ElementContent	    0.6 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode2(0,6), getEventDefType(EVENT_ER), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* CM ElementContent	    0.7.0 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode3(0,7,0), getEventDefType(EVENT_CM), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* PI ElementContent	    0.7.1 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[0]), getEventCode3(0,7,1), getEventDefType(EVENT_PI), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	/* ElementContent :
							EE	                    0
							SE (*) ElementContent	1.0
							CH ElementContent	    1.1
							ER ElementContent	    1.2
							CM ElementContent	    1.3.0
							PI ElementContent	    1.3.1 */
	tmp_err_code = initGrammarRule(&(elementGrammar->ruleArray[1]), &strm->memList);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	elementGrammar->ruleArray[1].nonTermID = GR_ELEMENT_CONTENT;
	if(is_default_fidelity == 1)
	{
		elementGrammar->ruleArray[1].bits[0] = 1;
		elementGrammar->ruleArray[1].bits[1] = 1;
	}
	else
	{
		elementGrammar->ruleArray[1].bits[0] = 1;
		elementGrammar->ruleArray[1].bits[1] = 2;
		elementGrammar->ruleArray[1].bits[2] = 1;
	}

	/* EE	                  0 */
	tmp_err_code = addProduction(&(elementGrammar->ruleArray[1]), getEventCode1(0), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	/* SE (*) ElementContent	1.0 */
	tmp_err_code = addProduction(&(elementGrammar->ruleArray[1]), getEventCode2(1,0), getEventDefType(EVENT_SE_ALL), GR_ELEMENT_CONTENT);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	/* CH ElementContent	    1.1 */
	tmp_err_code = addProduction(&(elementGrammar->ruleArray[1]), getEventCode2(1,1), getEventDefType(EVENT_CH), GR_ELEMENT_CONTENT);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(is_default_fidelity == 0)
	{
		/* ER ElementContent	    1.2 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[1]), getEventCode2(1,2), getEventDefType(EVENT_ER), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* CM ElementContent	    1.3.0 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[1]), getEventCode3(1,3,0), getEventDefType(EVENT_CM), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		/* PI ElementContent	    1.3.1 */
		tmp_err_code = addProduction(&(elementGrammar->ruleArray[1]), getEventCode3(1,3,1), getEventDefType(EVENT_PI), GR_ELEMENT_CONTENT);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}

	return ERR_OK;
}

errorCode copyGrammar(AllocList* memList, struct EXIGrammar* src, struct EXIGrammar** dest)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	int i = 0;
	if(src->nextInStack != NULL)  // Only single grammars can be copied - not grammar stacks
		return INVALID_OPERATION;

	*dest = memManagedAllocate(memList, sizeof(struct EXIGrammar));
	if(*dest == NULL)
		return MEMORY_ALLOCATION_ERROR;

	(*dest)->lastNonTermID = src->lastNonTermID;
	(*dest)->nextInStack = NULL;
	(*dest)->rulesDimension = src->rulesDimension;

	(*dest)->ruleArray = memManagedAllocate(memList, sizeof(GrammarRule) * (*dest)->rulesDimension);
	if((*dest)->ruleArray == NULL)
		return MEMORY_ALLOCATION_ERROR;

	for(; i < (*dest)->rulesDimension; i++)
	{
		tmp_err_code = copyGrammarRule(memList, &src->ruleArray[i], &(*dest)->ruleArray[i], 0);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	return ERR_OK;
}

errorCode pushGrammar(EXIGrammarStack** gStack, struct EXIGrammar* grammar)
{
	grammar->nextInStack = *gStack;
	*gStack = grammar;
	return ERR_OK;
}

errorCode popGrammar(EXIGrammarStack** gStack, struct EXIGrammar** grammar)
{
	*grammar = *gStack;
	*gStack = (*gStack)->nextInStack;
	(*grammar)->nextInStack = NULL;
	return ERR_OK;
}

static errorCode decodeQName(EXIStream* strm, QName* qname);

static errorCode decodeQName(EXIStream* strm, QName* qname)
{
	//TODO: add the case when Preserve.prefixes is true
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	uint32_t tmp_val_buf = 0;
	unsigned char uriBits = getBitsNumber(strm->uriTable->rowCount);
	uint32_t uriID = 0; // The URI id in the URI string table
	uint32_t flag_StringLiteralsPartition = 0;
	uint32_t lnID = 0;

	DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">Decoding QName\n"));
	tmp_err_code = decodeNBitUnsignedInteger(strm, uriBits, &tmp_val_buf);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;
	if(tmp_val_buf == 0) // uri miss
	{
		StringType str;
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">URI miss\n"));
		tmp_err_code = decodeString(strm, &str);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = addURIRow(strm->uriTable, str, &uriID, &strm->memList);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		qname->uri = &(strm->uriTable->rows[uriID].string_val);
	}
	else // uri hit
	{
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">URI hit\n"));
		qname->uri = &(strm->uriTable->rows[tmp_val_buf-1].string_val);
		uriID = tmp_val_buf-1;
	}

	tmp_err_code = decodeUnsignedInteger(strm, &flag_StringLiteralsPartition);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(flag_StringLiteralsPartition == 0) // local-name table hit
	{
		unsigned char lnBits = getBitsNumber(strm->uriTable->rows[uriID].lTable->rowCount - 1);
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">local-name table hit\n"));
		tmp_err_code = decodeNBitUnsignedInteger(strm, lnBits, &lnID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		qname->localName = &(strm->uriTable->rows[uriID].lTable->rows[lnID].string_val);
	}
	else // local-name table miss
	{
		StringType lnStr;
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">local-name table miss\n"));
		tmp_err_code = decodeStringOnly(strm, flag_StringLiteralsPartition - 1, &lnStr);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		if(strm->uriTable->rows[uriID].lTable == NULL)
		{
			tmp_err_code = createLocalNamesTable(&strm->uriTable->rows[uriID].lTable, &strm->memList, 0);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
		tmp_err_code = addLNRow(strm->uriTable->rows[uriID].lTable, lnStr, &lnID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		qname->localName = &(strm->uriTable->rows[uriID].lTable->rows[lnID].string_val);
	}
	strm->sContext.curr_uriID = uriID;
	strm->sContext.curr_lnID = lnID;
	return ERR_OK;
}

static errorCode decodeStringValue(EXIStream* strm, StringType** value);

static errorCode decodeStringValue(EXIStream* strm, StringType** value)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	uint32_t uriID = strm->sContext.curr_uriID;
	uint32_t lnID = strm->sContext.curr_lnID;
	uint32_t flag_StringLiteralsPartition = 0;
	tmp_err_code = decodeUnsignedInteger(strm, &flag_StringLiteralsPartition);
	if(tmp_err_code != ERR_OK)
		return tmp_err_code;

	if(flag_StringLiteralsPartition == 0) // "local" value partition table hit
	{
		uint32_t lvID = 0;
		uint32_t value_table_rowID;

		unsigned char lvBits = getBitsNumber(strm->uriTable->rows[uriID].lTable->rows[lnID].vCrossTable->rowCount - 1);
		tmp_err_code = decodeNBitUnsignedInteger(strm, lvBits, &lvID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		value_table_rowID = strm->uriTable->rows[uriID].lTable->rows[lnID].vCrossTable->valueRowIds[lvID];
		(*value) = &(strm->vTable->rows[value_table_rowID].string_val);
	}
	else if(flag_StringLiteralsPartition == 1)// global value partition table hit
	{
		uint32_t gvID = 0;
		unsigned char gvBits = getBitsNumber(strm->vTable->rowCount - 1);
		tmp_err_code = decodeNBitUnsignedInteger(strm, gvBits, &gvID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		(*value) = &(strm->vTable->rows[gvID].string_val);
	}
	else  // "local" value partition and global value partition table miss
	{
		StringType gvStr;
		uint32_t gvID = 0;
		tmp_err_code = decodeStringOnly(strm, flag_StringLiteralsPartition - 2, &gvStr);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		//TODO: Take into account valuePartitionCapacity parameter for setting globalID variable

		tmp_err_code = addGVRow(strm->vTable, gvStr, &gvID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = addLVRow(&(strm->uriTable->rows[uriID].lTable->rows[lnID]), gvID, &strm->memList);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		(*value) = &(strm->vTable->rows[gvID].string_val);
	}
	return ERR_OK;
}

static errorCode decodeEventContent(EXIStream* strm, EXIEvent event, ContentHandler* handler,
									unsigned int* nonTermID_out, GrammarRule* currRule, void* app_data);

static errorCode decodeEventContent(EXIStream* strm, EXIEvent event, ContentHandler* handler,
									unsigned int* nonTermID_out, GrammarRule* currRule,  void* app_data)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	// TODO: implement all cases
	QName qname;
	if(event.eventType == EVENT_SE_ALL)
	{
		unsigned char isDocGr = 0;
		struct EXIGrammar* res = NULL;
		unsigned char is_found = 0;

		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">SE(*) event\n"));
		// The content of SE event is the element qname
		tmp_err_code = decodeQName(strm, &qname);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		if(handler->startElement != NULL)  // Invoke handler method passing the element qname
		{
			if(handler->startElement(qname, app_data) == EXIP_HANDLER_STOP)
				return HANDLER_STOP_RECEIVED;
		}

		tmp_err_code = isDocumentGrammar(strm->gStack, &isDocGr);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		if(!isDocGr)  // If the current grammar is Element grammar ...
		{
			tmp_err_code = insertZeroProduction(currRule, getEventDefType(EVENT_SE_QNAME), *nonTermID_out, strm->sContext.curr_lnID, strm->sContext.curr_uriID);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}

		// New element grammar is pushed on the stack
		tmp_err_code = checkGrammarInPool(strm->ePool, strm->sContext.curr_uriID, strm->sContext.curr_lnID, &is_found, &res);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		strm->gStack->lastNonTermID = *nonTermID_out;
		if(is_found)
		{
			*nonTermID_out = GR_START_TAG_CONTENT;
			tmp_err_code = pushGrammar(&(strm->gStack), res);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
		else
		{
			struct EXIGrammar* elementGrammar = (struct EXIGrammar*) memManagedAllocate(&strm->memList, sizeof(struct EXIGrammar));
			if(elementGrammar == NULL)
				return MEMORY_ALLOCATION_ERROR;
			tmp_err_code = createBuildInElementGrammar(elementGrammar, strm);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			tmp_err_code = addGrammarInPool(strm->ePool, strm->sContext.curr_uriID, strm->sContext.curr_lnID, elementGrammar);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			*nonTermID_out = GR_START_TAG_CONTENT;
			tmp_err_code = pushGrammar(&(strm->gStack), elementGrammar);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}

	}
	else if(event.eventType == EVENT_AT_ALL)
	{
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">AT(*) event\n"));
		tmp_err_code = decodeQName(strm, &qname);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		if(handler->attribute != NULL)  // Invoke handler method
		{
			if(handler->attribute(qname, app_data) == EXIP_HANDLER_STOP)
				return HANDLER_STOP_RECEIVED;
		}
		if(event.valueType == VALUE_TYPE_STRING || event.valueType == VALUE_TYPE_NONE)
		{
			StringType* value;
			tmp_err_code = decodeStringValue(strm, &value);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			if(handler->stringData != NULL)  // Invoke handler method
			{
				if(handler->stringData(*value, app_data) == EXIP_HANDLER_STOP)
					return HANDLER_STOP_RECEIVED;
			}
		}
		tmp_err_code = insertZeroProduction(currRule, getEventDefType(EVENT_AT_QNAME), *nonTermID_out, strm->sContext.curr_lnID, strm->sContext.curr_uriID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else if(event.eventType == EVENT_SE_QNAME)
	{
		struct EXIGrammar* res = NULL;
		unsigned char is_found = 0;

		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">SE(qname) event\n"));
		qname.uri = &(strm->uriTable->rows[strm->sContext.curr_uriID].string_val);
		qname.localName = &(strm->uriTable->rows[strm->sContext.curr_uriID].lTable->rows[strm->sContext.curr_lnID].string_val);
		if(handler->startElement != NULL)  // Invoke handler method passing the element qname
		{
			if(handler->startElement(qname, app_data) == EXIP_HANDLER_STOP)
				return HANDLER_STOP_RECEIVED;
		}

		// New element grammar is pushed on the stack
		tmp_err_code = checkGrammarInPool(strm->ePool, strm->sContext.curr_uriID, strm->sContext.curr_lnID, &is_found, &res);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		strm->gStack->lastNonTermID = *nonTermID_out;
		if(is_found)
		{
			*nonTermID_out = GR_START_TAG_CONTENT;
			tmp_err_code = pushGrammar(&(strm->gStack), res);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
		else
		{
			return INCONSISTENT_PROC_STATE;  // The event require the presence of Element Grammar In the Pool
		}
	}
	else if(event.eventType == EVENT_AT_QNAME)
	{
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">AT(qname) event\n"));
		qname.uri = &(strm->uriTable->rows[strm->sContext.curr_uriID].string_val);
		qname.localName = &(strm->uriTable->rows[strm->sContext.curr_uriID].lTable->rows[strm->sContext.curr_lnID].string_val);
		if(handler->attribute != NULL)  // Invoke handler method
		{
			if(handler->attribute(qname, app_data) == EXIP_HANDLER_STOP)
				return HANDLER_STOP_RECEIVED;
		}
		if(event.valueType == VALUE_TYPE_STRING || event.valueType == VALUE_TYPE_NONE)
		{
			StringType* value;
			tmp_err_code = decodeStringValue(strm, &value);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			if(handler->stringData != NULL)  // Invoke handler method
			{
				if(handler->stringData(*value, app_data) == EXIP_HANDLER_STOP)
					return HANDLER_STOP_RECEIVED;
			}
		}
	}
	else if(event.eventType == EVENT_CH)
	{
		DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">CH event\n"));
		if(event.valueType == VALUE_TYPE_STRING || event.valueType == VALUE_TYPE_NONE)
		{
			StringType* value;
			tmp_err_code = decodeStringValue(strm, &value);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			if(handler->stringData != NULL)  // Invoke handler method
			{
				if(handler->stringData(*value, app_data) == EXIP_HANDLER_STOP)
					return HANDLER_STOP_RECEIVED;
			}
		}
	}
	return ERR_OK;
}

/*
 * #1#:
 * All productions in the built-in element grammar of the form LeftHandSide : EE are evaluated as follows:
 * - If a production of the form, LeftHandSide : EE with an event code of length 1 does not exist in
 *   the current element grammar, create one with event code 0 and increment the first part of the
 *   event code of each production in the current grammar with the non-terminal LeftHandSide on the left-hand side.
 * - Add the production created in step 1 to the grammar
 *
 * #2#
 * All productions in the built-in element grammar of the form LeftHandSide : CH RightHandSide are evaluated as follows:
 * - If a production of the form, LeftHandSide : CH RightHandSide with an event code of length 1 does not exist in
 *   the current element grammar, create one with event code 0 and increment the first part of the event code of
 *   each production in the current grammar with the non-terminal LeftHandSide on the left-hand side.
 * - Add the production created in step 1 to the grammar
 * - Evaluate the remainder of event sequence using RightHandSide.
 * */

errorCode processNextProduction(EXIStream* strm, EXIEvent* event,
							    unsigned int* nonTermID_out, ContentHandler* handler, void* app_data)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	uint32_t tmp_bits_val = 0;
	unsigned int currProduction = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int b = 0;

	DEBUG_MSG(INFO, DEBUG_GRAMMAR, (">Next production non-term-id: %d\n", strm->nonTermID));

	for(i = 0; i < strm->gStack->rulesDimension; i++)
	{
		if(strm->nonTermID == strm->gStack->ruleArray[i].nonTermID)
		{
			for(b = 0; b < 3; b++)
			{
				if(strm->gStack->ruleArray[i].bits[b] == 0 &&
						strm->gStack->ruleArray[i].prodCount > b) // zero bits encoded part of event code with more parts available
				{
					continue;
				}
				else if(strm->gStack->ruleArray[i].bits[b] == 0) // encoded with zero bits
				{
					*event = strm->gStack->ruleArray[i].prodArray[currProduction].event;
					*nonTermID_out = strm->gStack->ruleArray[i].prodArray[currProduction].nonTermID;
					if(event->eventType == EVENT_SD)
					{
						if(handler->startDocument != NULL)
						{
							if(handler->startDocument(app_data) == EXIP_HANDLER_STOP)
								return HANDLER_STOP_RECEIVED;
						}
					}
					else if(event->eventType == EVENT_ED)
					{
						if(handler->endDocument != NULL)
						{
							if(handler->endDocument(app_data) == EXIP_HANDLER_STOP)
								return HANDLER_STOP_RECEIVED;
						}
					}
					else if(event->eventType == EVENT_EE)
					{
						if(handler->endElement != NULL)
						{
							if(handler->endElement(app_data) == EXIP_HANDLER_STOP)
								return HANDLER_STOP_RECEIVED;
						}

					}
					else if(event->eventType == EVENT_SC)
					{
						if(handler->selfContained != NULL)
						{
							if(handler->selfContained(app_data) == EXIP_HANDLER_STOP)
								return HANDLER_STOP_RECEIVED;
						}
					}
					else // The event has content!
					{
						if(event->eventType != EVENT_CH) // CH events do not have QName in their content
						{
							strm->sContext.curr_uriID = strm->gStack->ruleArray[i].prodArray[currProduction].uriRowID;
							strm->sContext.curr_lnID = strm->gStack->ruleArray[i].prodArray[currProduction].lnRowID;
						}
						tmp_err_code = decodeEventContent(strm, *event, handler,
								        nonTermID_out, &(strm->gStack->ruleArray[i]), app_data);
						if(tmp_err_code != ERR_OK)
							return tmp_err_code;
					}
					return ERR_OK;
				}
				else
				{
					tmp_err_code = decodeNBitUnsignedInteger(strm, strm->gStack->ruleArray[i].bits[b], &tmp_bits_val);
					if(tmp_err_code != ERR_OK)
						return tmp_err_code;
					for(j = 0; j < strm->gStack->ruleArray[i].prodCount; j++)
					{
						if(strm->gStack->ruleArray[i].prodArray[j].code.size < b + 1)
							continue;
						if(strm->gStack->ruleArray[i].prodArray[j].code.code[b] == tmp_bits_val)
						{
							if(strm->gStack->ruleArray[i].prodArray[j].code.size == b + 1)
							{
								*event = strm->gStack->ruleArray[i].prodArray[j].event;
								*nonTermID_out = strm->gStack->ruleArray[i].prodArray[j].nonTermID;
								if(event->eventType == EVENT_SD)
								{
									if(handler->startDocument != NULL)
									{
										if(handler->startDocument(app_data) == EXIP_HANDLER_STOP)
											return HANDLER_STOP_RECEIVED;
									}
								}
								else if(event->eventType == EVENT_ED)
								{
									if(handler->endDocument != NULL)
									{
										if(handler->endDocument(app_data) == EXIP_HANDLER_STOP)
											return HANDLER_STOP_RECEIVED;
									}
								}
								else if(event->eventType == EVENT_EE)
								{
									unsigned char isDocGr = 0;
									
									if(handler->endElement != NULL)
									{
										if(handler->endElement(app_data) == EXIP_HANDLER_STOP)
											return HANDLER_STOP_RECEIVED;
									}
									tmp_err_code = isDocumentGrammar(strm->gStack, &isDocGr);
									if(tmp_err_code != ERR_OK)
										return tmp_err_code;

									if(!isDocGr)  // If the current grammar is Element grammar ...
									{
										if(b > 0)   // #1# COMMENT
										{
											strm->sContext.curr_uriID = strm->gStack->ruleArray[i].prodArray[j].uriRowID;
											strm->sContext.curr_lnID = strm->gStack->ruleArray[i].prodArray[j].lnRowID;
											tmp_err_code = insertZeroProduction(&(strm->gStack->ruleArray[i]), getEventDefType(EVENT_EE), GR_VOID_NON_TERMINAL,
																				strm->sContext.curr_lnID, strm->sContext.curr_uriID);
											if(tmp_err_code != ERR_OK)
												return tmp_err_code;
										}
									}
								}
								else if(event->eventType == EVENT_SC)
								{
									if(handler->selfContained != NULL)
									{
										if(handler->selfContained(app_data) == EXIP_HANDLER_STOP)
											return HANDLER_STOP_RECEIVED;
									}
								}
								else // The event has content!
								{
									if(event->eventType != EVENT_CH) // CH events do not have QName in their content
									{
										strm->sContext.curr_uriID = strm->gStack->ruleArray[i].prodArray[j].uriRowID;
										strm->sContext.curr_lnID = strm->gStack->ruleArray[i].prodArray[j].lnRowID;
									}
									if(event->eventType == EVENT_CH)
									{
										unsigned char isDocGr = 0;
										tmp_err_code = isDocumentGrammar(strm->gStack, &isDocGr);
										if(tmp_err_code != ERR_OK)
											return tmp_err_code;

										if(!isDocGr)  // If the current grammar is Element grammar ...
										{
											if(b > 0)   // #2# COMMENT
											{
												tmp_err_code = insertZeroProduction(&(strm->gStack->ruleArray[i]),getEventDefType(EVENT_CH), *nonTermID_out,
																					strm->sContext.curr_lnID, strm->sContext.curr_uriID);
												if(tmp_err_code != ERR_OK)
													return tmp_err_code;
											}
										}
									}
									tmp_err_code = decodeEventContent(strm, *event, handler,
											        nonTermID_out, &(strm->gStack->ruleArray[i]), app_data);
									if(tmp_err_code != ERR_OK)
										return tmp_err_code;
								}
								return ERR_OK;
							}
							else
							{
								currProduction = j;
							}
							break;
						}
					}
				}
			}
			break;
		}
	}
	return tmp_err_code;
}

errorCode createGrammarPool(GrammarPool** pool)
{
	*pool = (GrammarPool*) create_hashtable(GRAMMAR_POOL_DIMENSION, djbHash, keyEqual);
	if(*pool == NULL)
		return MEMORY_ALLOCATION_ERROR;

	return ERR_OK;
}

errorCode checkGrammarInPool(GrammarPool* pool, uint32_t uriRowID,
									uint32_t lnRowID, unsigned char* is_found, struct EXIGrammar** result)
{
	char key[8];
	createKey64bits(uriRowID, lnRowID, key);

	*result = hashtable_search(pool, key, 8);
	if(*result == NULL)
		*is_found = 0;
	else
		*is_found = 1;

	return ERR_OK;
}

errorCode addGrammarInPool(GrammarPool* pool, uint32_t uriRowID,
								uint32_t lnRowID, struct EXIGrammar* newGr)
{
	char* key = (char*) EXIP_MALLOC(8); // Keys are freed from the hash table
	if(key == NULL)
		return MEMORY_ALLOCATION_ERROR;
	createKey64bits(uriRowID, lnRowID, key);

	if (! hashtable_insert(pool, key, 8, newGr) )
		return HASH_TABLE_ERROR;

	return ERR_OK;
}

errorCode isDocumentGrammar(struct EXIGrammar* grammar, unsigned char* bool_result)
{
	if(grammar == NULL || grammar->ruleArray == NULL)
		return NULL_POINTER_REF;
	else
	{
		if(grammar->ruleArray[0].nonTermID == GR_DOCUMENT)
			*bool_result = 1;
		else
			*bool_result = 0;
	}
	return ERR_OK;
}

errorCode encodeQName(EXIStream* strm, QName qname)
{
	//TODO: add the case when Preserve.prefixes is true
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	uint32_t lnID = 0;

/******* Start: URI **********/
	uint32_t uriID = 0;
	unsigned char uriBits = getBitsNumber(strm->uriTable->rowCount);
	if(lookupURI(strm->uriTable, *(qname.uri), &uriID)) // uri hit
	{
		tmp_err_code = encodeNBitUnsignedInteger(strm, uriBits, uriID + 1);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else  // uri miss
	{
		tmp_err_code = encodeNBitUnsignedInteger(strm, uriBits, 0);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		tmp_err_code = encodeString(strm, qname.uri);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = addURIRow(strm->uriTable, *(qname.uri), &uriID, &strm->memList);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	strm->sContext.curr_uriID = uriID;
/******* End: URI **********/

/******* Start: Local name **********/
	if(lookupLN(strm->uriTable->rows[uriID].lTable, *(qname.localName), &lnID)) // local-name table hit
	{
		unsigned char lnBits = getBitsNumber(strm->uriTable->rows[uriID].lTable->rowCount - 1);
		tmp_err_code = encodeUnsignedInteger(strm, 0);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = encodeNBitUnsignedInteger(strm, lnBits, lnID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else // local-name table miss
	{
		tmp_err_code = encodeUnsignedInteger(strm, qname.localName->length + 1);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		tmp_err_code = encodeStringOnly(strm,  qname.localName);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;

		if(strm->uriTable->rows[uriID].lTable == NULL)
		{
			tmp_err_code = createLocalNamesTable(&strm->uriTable->rows[uriID].lTable, &strm->memList, 0);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
		tmp_err_code = addLNRow(strm->uriTable->rows[uriID].lTable, *(qname.localName), &lnID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	strm->sContext.curr_lnID = lnID;

/******* End: Local name **********/
	return ERR_OK;
}

errorCode encodeStringData(EXIStream* strm, StringType strng)
{
	errorCode tmp_err_code = UNEXPECTED_ERROR;
	unsigned char flag_StringLiteralsPartition = 0;
	uint32_t p_uriID = strm->sContext.curr_uriID;
	uint32_t p_lnID = strm->sContext.curr_lnID;
	uint32_t lvRowID = 0;
	flag_StringLiteralsPartition = lookupLV(strm->vTable, strm->uriTable->rows[p_uriID].lTable->rows[p_lnID].vCrossTable, strng, &lvRowID);
	if(flag_StringLiteralsPartition) //  "local" value partition table hit
	{
		unsigned char lvBits;

		tmp_err_code = encodeUnsignedInteger(strm, 0);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
		lvBits = getBitsNumber(strm->uriTable->rows[p_uriID].lTable->rows[p_lnID].vCrossTable->rowCount - 1);
		tmp_err_code = encodeNBitUnsignedInteger(strm, lvBits, lvRowID);
		if(tmp_err_code != ERR_OK)
			return tmp_err_code;
	}
	else //  "local" value partition table miss
	{
		uint32_t gvRowID = 0;
		flag_StringLiteralsPartition = lookupVal(strm->vTable, strng, &gvRowID);
		if(flag_StringLiteralsPartition) // global value partition table hit
		{
			unsigned char gvBits;
			
			tmp_err_code = encodeUnsignedInteger(strm, 1);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			gvBits = getBitsNumber(strm->vTable->rowCount - 1);
			tmp_err_code = encodeNBitUnsignedInteger(strm, gvBits, gvRowID);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
		else // "local" value partition and global value partition table miss
		{
			tmp_err_code = encodeUnsignedInteger(strm, strng.length + 2);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
			tmp_err_code = encodeStringOnly(strm, &strng);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;

			//TODO: Take into account valuePartitionCapacity parameter for setting globalID variable

			tmp_err_code = addGVRow(strm->vTable, strng, &gvRowID);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;

			tmp_err_code = addLVRow(&(strm->uriTable->rows[p_uriID].lTable->rows[p_lnID]), gvRowID, &strm->memList);
			if(tmp_err_code != ERR_OK)
				return tmp_err_code;
		}
	}

	return ERR_OK;
}

#endif /* BUILTINDOCGRAMMAR_H_ */
