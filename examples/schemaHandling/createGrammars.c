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
 * @file createGrammars.c
 * @brief Generate EXI grammars from XML schema definition and stores them in EXIP format
 *
 * @date Oct 13, 2010
 * @author Rumen Kyusakov
 * @version 0.1
 * @par[Revision] $Id: decodeTestEXI.c 93 2011-03-30 15:39:41Z kjussakov $
 */

#include "procTypes.h"
#include "stringManipulate.h"
#include "schema.h"
#include "grammarGenerator.h"
#include "memManagement.h"
#include "grammarAugment.h"
#include <stdio.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 200
#define SPRINTF_BUFFER_SIZE 300
#define OUT_EXIP     0
#define OUT_TEXT     1
#define OUT_SRC_DYN  2
#define OUT_SRC_STAT 3

static void printfHelp();

size_t readFileInputStream(void* buf, size_t readSize, void* stream);
size_t writeFileOutputStream(void* buf, size_t readSize, void* stream);

// Converts to NULL terminated ASCII representation
static errorCode stringToASCII(char* outBuf, unsigned int bufSize, StringType inStr);

int main(int argc, char *argv[])
{
	FILE *infile;
	FILE *outfile = stdout;
	char buffer[INPUT_BUFFER_SIZE];
	IOStream inputStrm;
	IOStream outputStrm;
	ExipSchema schema;
	unsigned char outputFormat = OUT_EXIP;
	unsigned int currArgNumber = 1;
	errorCode tmp_err_code = UNEXPECTED_ERROR;

	inputStrm.readWriteToStream = readFileInputStream;
	outputStrm.readWriteToStream = writeFileOutputStream;
	outputStrm.stream = outfile;
	unsigned char mask_specified = FALSE;
	unsigned char mask_strict = FALSE;
	unsigned char mask_sc = FALSE;
	unsigned char mask_preserve = 0;

	if(argc > 1)
	{
		if(strcmp(argv[1], "-help") == 0)
		{
			printfHelp();
			return 0;
		}
		else if(strcmp(argv[1], "-exip") == 0)
		{
			outputFormat = OUT_EXIP;
			currArgNumber++;
		}
		else if(strcmp(argv[1], "-text") == 0)
		{
			outputFormat = OUT_TEXT;
			currArgNumber++;
		}
		else if(argv[1][0] == '-' &&
				argv[1][1] == 's' &&
				argv[1][2] == 'r' &&
				argv[1][3] == 'c')
		{
			if(strcmp(argv[1] + 4, "=static") == 0)
			{
				outputFormat = OUT_SRC_STAT;
			}
			else
				outputFormat = OUT_SRC_DYN;
			currArgNumber++;
		}

		if(argc <= currArgNumber)
		{
			printfHelp();
			return 0;
		}

		if(argv[currArgNumber][0] == '-' &&
		   argv[currArgNumber][1] == 'm' &&
		   argv[currArgNumber][2] == 'a' &&
		   argv[currArgNumber][3] == 's' &&
		   argv[currArgNumber][4] == 'k' &&
		   argv[currArgNumber][5] == '=')
		{
			mask_specified = TRUE;
			if(argv[currArgNumber][6] == '1')
				mask_strict = TRUE;
			else
				mask_strict = FALSE;

			if(argv[currArgNumber][6] == '1')
				mask_sc = TRUE;
			else
				mask_sc = FALSE;

			if(argv[currArgNumber][7] == '1')
				mask_preserve = mask_preserve & PRESERVE_DTD;

			if(argv[currArgNumber][8] == '1')
				mask_preserve = mask_preserve & PRESERVE_PREFIXES;

			if(argv[currArgNumber][9] == '1')
				mask_preserve = mask_preserve & PRESERVE_LEXVALUES;

			if(argv[currArgNumber][10] == '1')
				mask_preserve = mask_preserve & PRESERVE_COMMENTS;

			if(argv[currArgNumber][11] == '1')
				mask_preserve = mask_preserve & PRESERVE_PIS;

			currArgNumber++;
		}

		if(argc <= currArgNumber)
		{
			printfHelp();
			return 0;
		}

		infile = fopen(argv[currArgNumber], "rb" );
		if(!infile)
		{
			fprintf(stderr, "Unable to open file %s", argv[currArgNumber]);
			return 1;
		}
		inputStrm.stream = infile;

		if(argc > currArgNumber + 1)
		{
			outfile = fopen(argv[currArgNumber + 1], "wb" );
			if(!outfile)
			{
				fprintf(stderr, "Unable to open file %s", argv[currArgNumber + 1]);
				return 1;
			}
			outputStrm.stream = outfile;
		}

		tmp_err_code = generateSchemaInformedGrammars(buffer, INPUT_BUFFER_SIZE, 0, &inputStrm, SCHEMA_FORMAT_XSD_EXI, &schema);
		if(tmp_err_code != ERR_OK)
		{
			printf("\n Error occured: %d", tmp_err_code);
			exit(1);
		}
		fclose(infile);

		{
			unsigned int i;
			unsigned int j;
			unsigned int k;
			size_t r;
			size_t p;
			char printfBuf[SPRINTF_BUFFER_SIZE];
			EXIGrammar* tmpGrammar;
			size_t tmp_prod_indx = 0;
			char prefix[20];
			char conv_buff[SPRINTF_BUFFER_SIZE];
			AllocList memList;

			strcpy(prefix, "prfx_"); // The default prefix
			if(ERR_OK != initAllocList(&memList))
			{
				printf("unexpected error!");
				exit(1);
			}

			if(outputFormat == OUT_EXIP)
			{
				printf("\n ERROR: EXIP output format is not implemented yet!");
				exit(1);
			}
			else if(outputFormat == OUT_SRC_STAT)
			{

				// When there is no mask specified this is not correct!
				// TODO: there should be extra rule slot for each grammar to be use if
				// strict == FALSE by addUndeclaredProductions!

				fwrite("#include \"schema.h\"\n\n", 1, strlen("#include \"schema.h\"\n\n"), outfile);

				for(i = 0; i < schema.initialStringTables->rowCount; i++)
				{
					if(schema.initialStringTables->rows[i].pTable != NULL)
					{
						sprintf(printfBuf, "struct PrefixRow %sprows_%d[%d] = {", prefix, i, schema.initialStringTables->rows[i].pTable->rowCount);
						fwrite(printfBuf, 1, strlen(printfBuf), outfile);
						for(k = 0; k < schema.initialStringTables->rows[i].pTable->rowCount; k++)
						{
							if(ERR_OK != stringToASCII(conv_buff, SPRINTF_BUFFER_SIZE, schema.initialStringTables->rows[i].pTable->rows[k].string_val))
							{
								printf("\n ERROR: OUT_SRC_STAT output format!");
								exit(1);
							}
							sprintf(printfBuf, "%s{{\"%s\",%d}}", k==0?"":",", conv_buff, schema.initialStringTables->rows[i].pTable->rows[k].string_val.length);
							fwrite(printfBuf, 1, strlen(printfBuf), outfile);
						}

						fwrite("};\n", 1, strlen("};\n"), outfile);

						sprintf(printfBuf, "PrefixTable %spTable_%d = {%sprows_%d, %d, %d, {NULL, 0}};\n\n", prefix, i, prefix, i, schema.initialStringTables->rows[i].pTable->rowCount, schema.initialStringTables->rows[i].pTable->rowCount);
						fwrite(printfBuf, 1, strlen(printfBuf), outfile);
					}

					for(j = 0; j < schema.initialStringTables->rows[i].lTable->rowCount; j++)
					{
						tmpGrammar = schema.initialStringTables->rows[i].lTable->rows[j].globalGrammar;
						if(tmpGrammar != NULL)
						{
							if(mask_specified == TRUE)
							{
								if(ERR_OK != addUndeclaredProductions(&memList, mask_strict, mask_sc, mask_preserve, tmpGrammar))
								{
									printf("\n ERROR: OUT_SRC_STAT output format!");
									exit(1);
								}
							}

							for(r = 0; r < tmpGrammar->rulesDimension; r++)
							{
								for(k = 0; k < 3; k++)
								{
									sprintf(printfBuf, "Production %sprodArray_%d_%d_%d_%d[%d] = {", prefix, i, j, r, k, tmpGrammar->ruleArray[r].prodCounts[k]);
									fwrite(printfBuf, 1, strlen(printfBuf), outfile);
									for(p = 0; p < tmpGrammar->ruleArray[r].prodCounts[k]; p++)
									{
										sprintf(printfBuf, "%s{{%d,%d}, %d, %d, %d}", p==0?"":",", tmpGrammar->ruleArray[r].prodArrays[k][p].event.eventType, tmpGrammar->ruleArray[r].prodArrays[k][p].event.valueType, tmpGrammar->ruleArray[r].prodArrays[k][p].uriRowID, tmpGrammar->ruleArray[r].prodArrays[k][p].lnRowID, tmpGrammar->ruleArray[r].prodArrays[k][p].nonTermID);
										fwrite(printfBuf, 1, strlen(printfBuf), outfile);
									}
									fwrite("};\n", 1, strlen("};\n"), outfile);
								}
							}

							sprintf(printfBuf, "\nGrammarRule %sruleArray_%d_%d[%d] = {", prefix, i, j, tmpGrammar->rulesDimension);
							fwrite(printfBuf, 1, strlen(printfBuf), outfile);

							for(r = 0; r < tmpGrammar->rulesDimension; r++)
							{
								sprintf(printfBuf, "%s{{%sprodArray_%d_%d_%d_0, %sprodArray_%d_%d_%d_1, %sprodArray_%d_%d_%d_2}, {%d, %d, %d}, {%d, %d, %d}}",
										r==0?"":",", prefix, i, j, r, prefix, i, j, r, prefix, i, j, r,
										tmpGrammar->ruleArray[r].prodCounts[0], tmpGrammar->ruleArray[r].prodCounts[1], tmpGrammar->ruleArray[r].prodCounts[2],
										tmpGrammar->ruleArray[r].bits[0], tmpGrammar->ruleArray[r].bits[1], tmpGrammar->ruleArray[r].bits[2]);
								fwrite(printfBuf, 1, strlen(printfBuf), outfile);
							}

							fwrite("};\n\n", 1, strlen("};\n\n"), outfile);

							sprintf(printfBuf, "EXIGrammar %sgrammar_%d_%d = {%sruleArray_%d_%d, %d, %d, %d};\n\n",
									prefix, i, j, prefix, i, j, tmpGrammar->rulesDimension, tmpGrammar->grammarType, tmpGrammar->contentIndex);
							fwrite(printfBuf, 1, strlen(printfBuf), outfile);
						}
					}

					sprintf(printfBuf, "struct LocalNamesRow %sLNrows_%d[%d] = {", prefix, i, schema.initialStringTables->rows[i].lTable->rowCount);
					fwrite(printfBuf, 1, strlen(printfBuf), outfile);

					for(j = 0; j < schema.initialStringTables->rows[i].lTable->rowCount; j++)
					{
						if(ERR_OK != stringToASCII(conv_buff, SPRINTF_BUFFER_SIZE, schema.initialStringTables->rows[i].lTable->rows[j].string_val))
						{
							printf("\n ERROR: OUT_SRC_STAT output format!");
							exit(1);
						}
						if(schema.initialStringTables->rows[i].lTable->rows[j].globalGrammar != NULL)
						{
							sprintf(printfBuf, "%s{NULL, {\"%s\", %d}, &%sgrammar_%d_%d}", j==0?"":",",
									conv_buff, schema.initialStringTables->rows[i].lTable->rows[j].string_val.length, prefix, i, j);
						}
						else
						{
							sprintf(printfBuf, "%s{NULL, {\"%s\", %d}, NULL}", j==0?"":",",
							conv_buff, schema.initialStringTables->rows[i].lTable->rows[j].string_val.length);
						}
						fwrite(printfBuf, 1, strlen(printfBuf), outfile);
					}

					fwrite("};\n", 1, strlen("};\n"), outfile);

					sprintf(printfBuf, "LocalNamesTable %slTable_%d = { %sLNrows_%d, %d, %d, {NULL, 0}};\n\n", prefix, i, prefix, i, schema.initialStringTables->rows[i].lTable->rowCount,  schema.initialStringTables->rows[i].lTable->rowCount);
					fwrite(printfBuf, 1, strlen(printfBuf), outfile);
				}

				sprintf(printfBuf, "struct URIRow %suriRows[%d] = {", prefix, schema.initialStringTables->rowCount);
				fwrite(printfBuf, 1, strlen(printfBuf), outfile);

				for(i = 0; i < schema.initialStringTables->rowCount; i++)
				{
					if(ERR_OK != stringToASCII(conv_buff, SPRINTF_BUFFER_SIZE, schema.initialStringTables->rows[i].string_val))
					{
						printf("\n ERROR: OUT_SRC_STAT output format!");
						exit(1);
					}
					if(schema.initialStringTables->rows[i].pTable != NULL)
					{
						sprintf(printfBuf, "%s{&%spTable_%d, &%slTable_%d, {\"%s\", %d}}", i==0?"":",", prefix, i, prefix, i, conv_buff, schema.initialStringTables->rows[i].string_val.length);
					}
					else
					{
						sprintf(printfBuf, "%s{NULL, &%slTable_%d, {\"%s\", %d}}", i==0?"":",", prefix, i, conv_buff, schema.initialStringTables->rows[i].string_val.length);
					}
					fwrite(printfBuf, 1, strlen(printfBuf), outfile);
				}

				fwrite("};\n", 1, strlen("};\n"), outfile);

				sprintf(printfBuf, "URITable %suriTbl = {%suriRows, %d, %d, {NULL, 0}};\n\n", prefix, prefix, schema.initialStringTables->rowCount, schema.initialStringTables->rowCount);
				fwrite(printfBuf, 1, strlen(printfBuf), outfile);

				sprintf(printfBuf, "QNameID %sqnames[%d] = {", prefix, schema.globalElemGrammarsCount);
				fwrite(printfBuf, 1, strlen(printfBuf), outfile);

				for(i = 0; i < schema.globalElemGrammarsCount; i++)
				{
					sprintf(printfBuf, "%s{%d, %d}", i==0?"":",", schema.globalElemGrammars[i].uriRowId, schema.globalElemGrammars[i].lnRowId);
					fwrite(printfBuf, 1, strlen(printfBuf), outfile);
				}
				fwrite("};\n\n", 1, strlen("};\n\n"), outfile);

				sprintf(printfBuf, "ExipSchema %sschema = {&%suriTbl, %sqnames, %d, {NULL, NULL}};\n", prefix, prefix, prefix, schema.globalElemGrammarsCount);
				fwrite(printfBuf, 1, strlen(printfBuf), outfile);

			}
			else if(outputFormat == OUT_TEXT)
			{
				for(i = 0; i < schema.initialStringTables->rowCount; i++)
				{
					for(j = 0; j < schema.initialStringTables->rows[i].lTable->rowCount; j++)
					{
						tmpGrammar = schema.initialStringTables->rows[i].lTable->rows[j].globalGrammar;
						if(tmpGrammar != NULL)
						{
							if(mask_specified == TRUE)
							{
								if(ERR_OK != addUndeclaredProductions(&memList, mask_strict, mask_sc, mask_preserve, tmpGrammar))
								{
									printf("\n ERROR: OUT_TEXT output format!");
									exit(1);
								}
							}

							fwrite("Grammar ", 1, strlen("Grammar "), outfile);
							fwrite(schema.initialStringTables->rows[i].string_val.str, 1, schema.initialStringTables->rows[i].string_val.length, outfile);
							fwrite(":", 1, 1, outfile);
							fwrite(schema.initialStringTables->rows[i].lTable->rows[j].string_val.str, 1, schema.initialStringTables->rows[i].lTable->rows[j].string_val.length, outfile);
							fwrite("\n", 1, 1, outfile);

							for(r = 0; r < tmpGrammar->rulesDimension; r++)
							{
								sprintf(printfBuf, "NT-%d: \n", r);
								fwrite(printfBuf, 1, strlen(printfBuf), outfile);
								for(p = 0; p < tmpGrammar->ruleArray[r].prodCounts[0]; p++)
								{
									tmp_prod_indx = tmpGrammar->ruleArray[r].prodCounts[0] - 1 - p;
									switch(tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].event.eventType)
									{
										case EVENT_SD:
											fwrite("\tSD ", 1, strlen("\tSD "), outfile);
											break;
										case EVENT_ED:
											fwrite("\tED ", 1, strlen("\tED "), outfile);
											break;
										case EVENT_SE_QNAME:
											fwrite("\tSE (", 1, strlen("\tSE ("), outfile);
											fwrite(schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].string_val.str, 1, schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].string_val.length, outfile);
											fwrite(":", 1, 1, outfile);
											fwrite(schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].lTable->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].lnRowID].string_val.str, 1, schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].lTable->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].lnRowID].string_val.length, outfile);
											fwrite(") ", 1, 2, outfile);
											break;
										case EVENT_SE_URI:
											fwrite("\tSE (uri) ", 1, strlen("\tSE (uri) "), outfile);
											break;
										case EVENT_SE_ALL:
											fwrite("\tSE (*) ", 1, strlen("\tSE (*) "), outfile);
											break;
										case EVENT_EE:
											fwrite("\tEE ", 1, strlen("\tEE "), outfile);
											break;
										case EVENT_AT_QNAME:
											fwrite("\tAT (", 1, strlen("\tAT ("), outfile);
											fwrite(schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].string_val.str, 1, schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].string_val.length, outfile);
											fwrite(":", 1, 1, outfile);
											fwrite(schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].lTable->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].lnRowID].string_val.str, 1, schema.initialStringTables->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].uriRowID].lTable->rows[tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].lnRowID].string_val.length, outfile);
											fwrite(") ", 1, 2, outfile);
											break;
										case EVENT_AT_URI:
											fwrite("\tAT (uri) ", 1, strlen("\tAT (uri) "), outfile);
											break;
										case EVENT_AT_ALL:
											fwrite("\tAT (*) ", 1, strlen("\tAT (*) "), outfile);
											break;
										case EVENT_CH:
											fwrite("\tCH ", 1, strlen("\tCH "), outfile);
											break;
										case EVENT_NS:
											fwrite("\tNS ", 1, strlen("\tNS "), outfile);
											break;
										case EVENT_CM:
											fwrite("\tCM ", 1, strlen("\tCM "), outfile);
											break;
										case EVENT_PI:
											fwrite("\tPI ", 1, strlen("\tPI "), outfile);
											break;
										case EVENT_DT:
											fwrite("\tDT ", 1, strlen("\tDT "), outfile);
											break;
										case EVENT_ER:
											fwrite("\tER ", 1, strlen("\tER "), outfile);
											break;
										case EVENT_SC:
											fwrite("\tSC ", 1, strlen("\tSC "), outfile);
											break;
										case EVENT_VOID:
											fwrite(" ", 1, 1, outfile);
											break;
										default:
											return UNEXPECTED_ERROR;
									}
									sprintf(printfBuf, "\tNT-%u\t", (unsigned int) tmpGrammar->ruleArray[r].prodArrays[0][tmp_prod_indx].nonTermID);
									fwrite(printfBuf, 1, strlen(printfBuf), outfile);
									sprintf(printfBuf, "%d\n", p);
									fwrite(printfBuf, 1, strlen(printfBuf), outfile);
								}
								fwrite("\n", 1, 1, outfile);
							}
						}
					}
				}
			}
			freeAllocList(&memList);
		}

	}
	else
	{
		printfHelp();
		return 1;
	}
	return 0;
}

static void printfHelp()
{
    printf("\n" );
    printf("  EXIP     Efficient XML Interchange Processor, Rumen Kyusakov, 2011 \n");
    printf("           Copyright (c) 2010 - 2011, EISLAB - Luleå University of Technology Version 0.2 \n");
    printf("  Usage:   exipSchema [options] <schema_in> [<schema_out>] \n\n");
    printf("           Options: [[-help | [-exip | -text | -src[= dynamic | static]] [-mask=<OPTIONS_MASK>]] ] \n");
    printf("           -help   :   Prints this help message\n\n");
    printf("           -exip   :   Format the output schema definitions in EXIP-specific format (Default) \n\n");
    printf("           -text   :   Format the output schema definitions in human readable text format \n\n");
    printf("           -src    :   Create source files for the grammars defined. If you know the EXI options \n\n");
    printf("                       to be used for processing in advance, you can create more efficient representation \n\n");
    printf("                       by specifying STRICT, SELF_CONTAINED and PRESERVE options in the OPTIONS_MASK\n\n");
    printf("                       Only documents for the specified values for this options will be able to be\n\n");
    printf("                       processed correctly by the EXIP instance.\n\n");
    printf("                       -src=dynamic creates a function that dynamically creates the grammar (Default)\n\n");
    printf("                       -src=static the grammar definitions are defined statically as global variables\n\n");
    printf("           <OPTIONS_MASK>:   <STRICT><SELF_CONTAINED><dtd><prefixes><lexicalValues><comments><pis> := <0|1><0|1><0|1><0|1><0|1><0|1><0|1> \n\n");
    printf("           <schema_in>   :   Source XML schema file \n\n");
    printf("           <schema_out>  :   Destination schema file in the particular format (Default is the standard output) \n\n");
    printf("  Purpose: Manipulation of EXIP schemas\n");
    printf("\n" );
}

size_t readFileInputStream(void* buf, size_t readSize, void* stream)
{
	FILE *infile = (FILE*) stream;
	return fread(buf, 1, readSize, infile);
}

size_t writeFileOutputStream(void* buf, size_t readSize, void* stream)
{
	FILE *outfile = (FILE*) stream;
	return fwrite(buf, 1, readSize, outfile);
}

static errorCode stringToASCII(char* outBuf, unsigned int bufSize, StringType inStr)
{
	if(inStr.length >= bufSize)
		return OUT_OF_BOUND_BUFFER;

	memcpy(outBuf, inStr.str, inStr.length);
	outBuf[inStr.length] = '\0';

	return ERR_OK;
}
