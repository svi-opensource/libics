/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright 2015-2017:
 *   Scientific Volume Imaging Holding B.V.
 *   Laapersveld 63, 1213 VB Hilversum, The Netherlands
 *   https://www.svi.nl
 *
 * Copyright (C) 2000-2013 Cris Luengo and others
 *
 * Large chunks of this library written by
 *    Bert Gijsbers
 *    Dr. Hans T.M. van der Voort
 * And also Damir Sudar, Geert van Kempen, Jan Jitze Krol,
 * Chiel Baarslag and Fons Laan.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * FILE : libics_write.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsWriteIcs()
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "libics_intern.h"

static Ics_Error icsToken2Str(Ics_Token  token,
                              char      *cPtr)
{
    ICSINIT;
    int notFound = 1, i;


        /* Search the globally defined categories for a token match: */
    i = 0;
    while (notFound && i < G_Categories.entries) {
        notFound = token != G_Categories.list[i].token;
        if(!notFound) {
            strcpy(cPtr, G_Categories.list[i].name);
        }
        i++;
    }
    i = 0;
    while (notFound && i < G_SubCategories.entries) {
        notFound = token != G_SubCategories.list[i].token;
        if(!notFound) {
            strcpy(cPtr, G_SubCategories.list[i].name);
        }
        i++;
    }
    i = 0;
    while (notFound && i < G_SubSubCategories.entries) {
        notFound = token != G_SubSubCategories.list[i].token;
        if(!notFound) {
            strcpy(cPtr, G_SubSubCategories.list[i].name);
        }
        i++;
    }
    i = 0;
    while (notFound && i < G_Values.entries) {
        notFound = token != G_Values.list[i].token;
        if (!notFound) {
            strcpy(cPtr, G_Values.list[i].name);
        }
        i++;
    }
    ICSTR(notFound, IcsErr_IllIcsToken);

    return error;
}


static Ics_Error icsFirstToken(char      *line,
                               Ics_Token  token)
{
    ICSDECL;
    char tokenName[ICS_STRLEN_TOKEN];


    ICSXR(icsToken2Str(token, tokenName));
    strcpy(line, tokenName);
    IcsAppendChar(line, ICS_FIELD_SEP);

    return error;
}


static Ics_Error icsAddToken(char      *line,
                             Ics_Token  token)
{
    ICSDECL;
    char tokenName[ICS_STRLEN_TOKEN];


    ICSXR(icsToken2Str(token, tokenName));
    ICSTR(strlen(line) + strlen(tokenName) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, tokenName);
    IcsAppendChar(line, ICS_FIELD_SEP);

    return error;
}


static Ics_Error icsAddLastToken(char      *line,
                                 Ics_Token  token)
{
    ICSDECL;
    char tokenName[ICS_STRLEN_TOKEN];


    ICSXR(icsToken2Str(token, tokenName));
    ICSTR(strlen(line) + strlen(tokenName) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, tokenName);
    IcsAppendChar(line, ICS_EOL);

    return error;
}


static Ics_Error icsFirstText(char *line,
                              char *text)
{
    ICSINIT;


    ICSTR(text[0] == '\0', IcsErr_EmptyField);
    ICSTR(strlen(text) + 2 > ICS_LINE_LENGTH, IcsErr_LineOverflow);
    strcpy(line, text);
    IcsAppendChar(line, ICS_FIELD_SEP);

    return error;
}


static Ics_Error icsAddText(char *line,
                            char *text)
{
    ICSINIT;


    ICSTR(text[0] == '\0', IcsErr_EmptyField);
    ICSTR(strlen(line) + strlen(text) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, text);
    IcsAppendChar(line, ICS_FIELD_SEP);

    return error;
}


static Ics_Error icsAddLastText(char *line,
                                char *text)
{
    ICSINIT;


    ICSTR(text[0] == '\0', IcsErr_EmptyField);
    ICSTR(strlen(line) + strlen(text) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, text);
    IcsAppendChar(line, ICS_EOL);

    return error;
}


static Ics_Error icsAddInt(char     *line,
                           long int  i)
{
    ICSINIT;
    char intStr[ICS_STRLEN_OTHER];


    sprintf(intStr, "%ld%c", i, ICS_FIELD_SEP);
    ICSTR(strlen(line) + strlen(intStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, intStr);

    return error;
}


static Ics_Error icsAddLastInt(char     *line,
                               long int  i)
{
    ICSINIT;
    char intStr[ICS_STRLEN_OTHER];


    sprintf(intStr, "%ld%c", i, ICS_EOL);
    ICSTR(strlen(line) + strlen(intStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, intStr);

    return error;
}


static Ics_Error icsAddDouble(char   *line,
                              double  d)
{
    ICSINIT;
    char dStr[ICS_STRLEN_OTHER];


    if (d == 0 ||(fabs(d) < ICS_MAX_DOUBLE && fabs(d) >= ICS_MIN_DOUBLE)) {
        sprintf(dStr, "%f%c", d, ICS_FIELD_SEP);
    } else {
        sprintf(dStr, "%e%c", d, ICS_FIELD_SEP);
    }
    ICSTR(strlen(line) + strlen(dStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, dStr);

    return error;
}


static Ics_Error icsAddLastDouble(char   *line,
                                  double  d)
{
    ICSINIT;
    char dStr[ICS_STRLEN_OTHER];


    if (d == 0 || (fabs(d) < ICS_MAX_DOUBLE && fabs(d) >= ICS_MIN_DOUBLE)) {
        sprintf(dStr, "%f%c", d, ICS_EOL);
    } else {
        sprintf(dStr, "%e%c", d, ICS_EOL);
    }
    ICSTR(strlen(line) + strlen(dStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow);
    strcat(line, dStr);

    return error;
}


static Ics_Error icsAddLine(char *line,
                            FILE *fp)
{
    ICSINIT;


    ICSTR(fputs(line, fp) == EOF, IcsErr_FWriteIcs);

    return error;
}


static Ics_Error writeIcsSource(Ics_Header *icsStruct,
                                FILE       *fp)
{
    ICSINIT;
    int  problem;
    char line[ICS_LINE_LENGTH];


    if ((icsStruct->version >= 2) &&(icsStruct->srcFile[0] != '\0')) {
            /* Write the source filename to the file */
        problem = icsFirstToken(line, ICSTOK_SOURCE);
        problem |= icsAddToken(line, ICSTOK_FILE);
        problem |= icsAddLastText(line, icsStruct->srcFile);
        ICSTR(problem, IcsErr_FailWriteLine);
        ICSXR(icsAddLine(line, fp));

            /* Now write the source file offset to the file */
        problem = icsFirstToken(line, ICSTOK_SOURCE);
        problem |= icsAddToken(line, ICSTOK_OFFSET);
        problem |= icsAddLastInt(line,(long int)icsStruct->srcOffset);
        ICSTR(problem, IcsErr_FailWriteLine);
        ICSXR(icsAddLine(line, fp));
    }

    return error;
}


static Ics_Error writeIcsLayout(Ics_Header *icsStruct,
                                FILE       *fp)
{
    ICSDECL;
    int    problem, i;
    char   line[ICS_LINE_LENGTH];
    size_t size;


        /* Write the number of parameters to the buffer: */
    ICSTR(icsStruct->dimensions < 1, IcsErr_NoLayout);
    ICSTR(icsStruct->dimensions > ICS_MAXDIM, IcsErr_TooManyDims);
    problem = icsFirstToken(line, ICSTOK_LAYOUT);
    problem |= icsAddToken(line, ICSTOK_PARAMS);
    problem |= icsAddLastInt(line, icsStruct->dimensions + 1);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Now write the order identifiers to the buffer: */
    problem = icsFirstToken(line, ICSTOK_LAYOUT);
    problem |= icsAddToken(line, ICSTOK_ORDER);
    problem |= icsAddText(line, ICS_ORDER_BITS);
    for (i = 0; i < icsStruct->dimensions-1; i++) {
        ICSTR(*(icsStruct->dim[i].order) == '\0', IcsErr_NoLayout);
        problem |= icsAddText(line, icsStruct->dim[i].order);
    }
    ICSTR(*(icsStruct->dim[i].order) == '\0', IcsErr_NoLayout);
    problem |= icsAddLastText(line, icsStruct->dim[i].order);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Write the sizes: */
    problem = icsFirstToken(line, ICSTOK_LAYOUT);
    problem |= icsAddToken(line, ICSTOK_SIZES);
    size = IcsGetDataTypeSize(icsStruct->imel.dataType);
    problem |= icsAddInt(line,(long int)size * 8);
    for (i = 0; i < icsStruct->dimensions-1; i++) {
        ICSTR(icsStruct->dim[i].size == 0, IcsErr_NoLayout);
        problem |= icsAddInt(line,(long int) icsStruct->dim[i].size);
    }
    ICSTR(icsStruct->dim[i].size == 0, IcsErr_NoLayout);
    problem |= icsAddLastInt(line,(long int) icsStruct->dim[i].size);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Coordinates class. Video(default) means 0,0 corresponds with
           top-left. */
    if (*(icsStruct->coord) == '\0') {
        strcpy(icsStruct->coord, ICS_COORD_VIDEO);
    }
    problem = icsFirstToken(line, ICSTOK_LAYOUT);
    problem |= icsAddToken(line, ICSTOK_COORD);
    problem |= icsAddLastText(line, icsStruct->coord);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Number of significant bits, default is the number of bits/sample: */
    if (icsStruct->imel.sigBits == 0) {
        size = IcsGetDataTypeSize(icsStruct->imel.dataType);
        icsStruct->imel.sigBits = size * 8;
    }
    problem = icsFirstToken(line, ICSTOK_LAYOUT);
    problem |= icsAddToken(line, ICSTOK_SIGBIT);
    problem |= icsAddLastInt(line,(long int)icsStruct->imel.sigBits);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

    return error;
}


static Ics_Error writeIcsRep(Ics_Header *icsStruct,
                             FILE       *fp)
{
    ICSDECL;
    int        problem, empty, i;
    char       line[ICS_LINE_LENGTH];
    Ics_Format format;
    int        sign;
    size_t     bits;


    IcsGetPropsDataType(icsStruct->imel.dataType, &format, &sign, &bits);

        /* Write basic format, i.e. integer, float or complex, default is
           integer: */
    problem = icsFirstToken(line, ICSTOK_REPRES);
    problem |= icsAddToken(line, ICSTOK_FORMAT);
    switch(format) {
        case IcsForm_integer:
            problem |= icsAddLastToken(line, ICSTOK_FORMAT_INTEGER);
            break;
        case IcsForm_real:
            problem |= icsAddLastToken(line, ICSTOK_FORMAT_REAL);
            break;
        case IcsForm_complex:
            problem |= icsAddLastToken(line, ICSTOK_FORMAT_COMPLEX);
            break;
        default:
            return IcsErr_UnknownDataType;
    }
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Signal whether the 'basic format' is signed or unsigned. Rubbish for
           float or complex, but this seems to be the definition. */
    problem = icsFirstToken(line, ICSTOK_REPRES);
    problem |= icsAddToken(line, ICSTOK_SIGN);
    if (sign == 1) {
        problem |= icsAddLastToken(line, ICSTOK_SIGN_SIGNED);
    } else {
        problem |= icsAddLastToken(line, ICSTOK_SIGN_UNSIGNED);
    }
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Signal whether the entire data array is compressed and if so by what
           compression technique: */
    problem = icsFirstToken(line, ICSTOK_REPRES);
    problem |= icsAddToken(line, ICSTOK_COMPR);
    switch(icsStruct->compression) {
        case IcsCompr_uncompressed:
            problem |= icsAddLastToken(line, ICSTOK_COMPR_UNCOMPRESSED);
            break;
        case IcsCompr_compress:
            problem |= icsAddLastToken(line, ICSTOK_COMPR_COMPRESS);
            break;
        case IcsCompr_gzip:
            problem |= icsAddLastToken(line, ICSTOK_COMPR_GZIP);
            break;
        default:
            return IcsErr_UnknownCompression;
    }
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Define the byteorder. This is supposed to resolve little/big endian
           problems. If the calling function put something here, we'll keep
           it. Otherwise we fill in the machine's byte order. */
    empty = 0;
    for (i = 0; i <(int)IcsGetDataTypeSize(icsStruct->imel.dataType); i++) {
        empty |= !(icsStruct->byteOrder[i]);
    }
    if (empty) {
        IcsFillByteOrder(IcsGetDataTypeSize(icsStruct->imel.dataType),
                         icsStruct->byteOrder);
    }
    problem = icsFirstToken(line, ICSTOK_REPRES);
    problem |= icsAddToken(line, ICSTOK_BYTEO);
    for (i = 0; i <(int)IcsGetDataTypeSize(icsStruct->imel.dataType) - 1; i++) {
        problem |= icsAddInt(line, icsStruct->byteOrder[i]);
    }
    problem |= icsAddLastInt(line, icsStruct->byteOrder[i]);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* SCIL_Image compatability stuff: SCIL_TYPE */
    if (icsStruct->scilType[0] != '\0') {
        problem = icsFirstToken(line, ICSTOK_REPRES);
        problem |= icsAddToken(line, ICSTOK_SCILT);
        problem |= icsAddLastText(line, icsStruct->scilType);
        ICSTR(problem, IcsErr_FailWriteLine);
        ICSXR(icsAddLine(line, fp));
    }

    return error;
}


static Ics_Error writeIcsParam(Ics_Header *icsStruct,
                               FILE       *fp)
{
    ICSINIT;
    int  problem, i;
    char line[ICS_LINE_LENGTH];


        /* Define the origin, scaling factors and the units */
    problem = icsFirstToken(line, ICSTOK_PARAM);
    problem |= icsAddToken(line, ICSTOK_ORIGIN);
    problem |= icsAddDouble(line, icsStruct->imel.origin);
    for (i = 0; i < icsStruct->dimensions-1; i++) {
        problem |= icsAddDouble(line, icsStruct->dim[i].origin);
    }
    problem |= icsAddLastDouble(line, icsStruct->dim[i].origin);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

    problem = icsFirstToken(line, ICSTOK_PARAM);
    problem |= icsAddToken(line, ICSTOK_SCALE);
    problem |= icsAddDouble(line, icsStruct->imel.scale);
    for (i = 0; i < icsStruct->dimensions-1; i++) {
        problem |= icsAddDouble(line, icsStruct->dim[i].scale);
    }
    problem |= icsAddLastDouble(line, icsStruct->dim[i].scale);
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

    problem = icsFirstToken(line, ICSTOK_PARAM);
    problem |= icsAddToken(line, ICSTOK_UNITS);
    if (icsStruct->imel.unit[0] == '\0') {
        problem |= icsAddText(line, ICS_UNITS_RELATIVE);
    } else {
        problem |= icsAddText(line, icsStruct->imel.unit);
    }
    for (i = 0; i < icsStruct->dimensions-1; i++) {
        if (icsStruct->dim[i].unit[0] == '\0') {
            problem |= icsAddText(line, ICS_UNITS_UNDEFINED);
        } else {
            problem |= icsAddText(line, icsStruct->dim[i].unit);
        }
    }
    if (icsStruct->dim[i].unit[0] == '\0') {
        problem |= icsAddLastText(line, ICS_UNITS_UNDEFINED);
    } else {
        problem |= icsAddLastText(line, icsStruct->dim[i].unit);
    }
    ICSTR(problem, IcsErr_FailWriteLine);
    ICSXR(icsAddLine(line, fp));

        /* Write labels associated with the dimensions to the .ics file, if
           any: */
    problem = 0;
    for (i = 0; i <(int)icsStruct->dimensions; i++) {
        problem |= *(icsStruct->dim[i].label) == '\0';
    }
    if (!problem) {
        problem = icsFirstToken(line, ICSTOK_PARAM);
        problem |= icsAddToken(line, ICSTOK_LABELS);
        problem |= icsAddText(line, ICS_LABEL_BITS);
        for (i = 0; i < icsStruct->dimensions-1; i++) {
            problem |= icsAddText(line, icsStruct->dim[i].label);
        }
        problem |= icsAddLastText(line, icsStruct->dim[i].label);
        ICSTR(problem, IcsErr_FailWriteLine);
        ICSXR(icsAddLine(line, fp));
    }

    return error;
}


static Ics_Error writeIcsSensorData(Ics_Header *icsStruct,
                                    FILE       *fp)
{
    ICSINIT;
    int  problem, i, chans;
    char line[ICS_LINE_LENGTH];


    if (icsStruct->writeSensor) {

        chans = icsStruct->sensorChannels;
        ICSTR(chans > ICS_MAX_LAMBDA, IcsErr_TooManyChans);

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_TYPE);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddText(line, icsStruct->type[i]);
        }
        problem |= icsAddLastText(line, icsStruct->type[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_MODEL);
        problem |= icsAddLastText(line, icsStruct->model);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_CHANS);
        problem |= icsAddLastInt(line, chans);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_PINHRAD);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->pinholeRadius[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->pinholeRadius[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_LAMBDEX);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->lambdaEx[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->lambdaEx[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_LAMBDEM);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->lambdaEm[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->lambdaEm[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_PHOTCNT);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddInt(line, icsStruct->exPhotonCnt[i]);
        }
        problem |= icsAddLastInt(line, icsStruct->exPhotonCnt[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_REFRIME);
        problem |= icsAddLastDouble(line, icsStruct->refrInxMedium);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_NUMAPER);
        problem |= icsAddLastDouble(line, icsStruct->numAperture);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_REFRILM);
        problem |= icsAddLastDouble(line, icsStruct->refrInxLensMedium);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_PINHSPA);
        problem |= icsAddLastDouble(line, icsStruct->pinholeSpacing);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

            /* Add STED parameters */
        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_STEDDEPLMODE);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddText(line, icsStruct->stedDepletionMode[i]);
        }
        problem |= icsAddLastText(line, icsStruct->stedDepletionMode[i]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_STEDLAMBDA);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->stedLambda[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->stedLambda[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_STEDSATFACTOR);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->stedSatFactor[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->stedSatFactor[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_STEDIMMFRACTION);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->stedImmFraction[i]);
        }
        problem |= icsAddLastDouble(line,
                                    icsStruct->stedImmFraction[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_STEDVPPM);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->stedVPPM[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->stedVPPM[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

            /* Add detector parameters. */
        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_DETPPU);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->detectorPPU[i]);
        }
        problem |= icsAddLastDouble(line, icsStruct->detectorPPU[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_DETBASELINE);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->detectorBaseline[i]);
        }
        problem |= icsAddLastDouble(line,
                                    icsStruct->detectorBaseline[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }

        problem = icsFirstToken(line, ICSTOK_SENSOR);
        problem |= icsAddToken(line, ICSTOK_SPARAMS);
        problem |= icsAddToken(line, ICSTOK_DETLNAVGCNT);
        for (i = 0; i < chans - 1; i++) {
            problem |= icsAddDouble(line, icsStruct->detectorLineAvgCnt[i]);
        }
        problem |= icsAddLastDouble(line,
                                    icsStruct->detectorLineAvgCnt[chans - 1]);
        if (!problem) {
            ICSXR(icsAddLine(line, fp));
        }


    }

    return error;
}


static Ics_Error writeIcsHistory(Ics_Header *icsStruct,
                                 FILE       *fp)
{
    ICSINIT;
    int          problem, i;
    char         line[ICS_LINE_LENGTH];
    Ics_History* hist = (Ics_History*)icsStruct->history;


    if (hist != NULL) {
        for (i = 0; i < hist->nStr; i++) {
            if (hist->strings[i] != NULL) {
                problem = icsFirstToken(line, ICSTOK_HISTORY);
                problem |= icsAddLastText(line, hist->strings[i]);
                if (!problem) {
                    ICSXR(icsAddLine(line, fp));
                }
            }
        }
    }

    return error;
}


static Ics_Error markEndOfFile(Ics_Header *icsStruct,
                               FILE       *fp)
{
    ICSINIT;
    char line[ICS_LINE_LENGTH];


    if ((icsStruct->version != 1) &&(icsStruct->srcFile[0] == '\0')) {
        error = icsFirstToken(line, ICSTOK_END);
        ICSTR(error, IcsErr_FailWriteLine);
        IcsAppendChar(line, ICS_EOL);
        ICSXR(icsAddLine(line, fp));
    }

    return error;
}


Ics_Error IcsWriteIcs(Ics_Header *icsStruct,
                      const char *filename)
{
    ICSDECL;
    ICS_INIT_LOCALE;
    char  line[ICS_LINE_LENGTH];
    char  buf[ICS_MAXPATHLEN];
    FILE *fp;


    if ((filename != NULL) &&(filename[0] != '\0')) {
        IcsGetIcsName(icsStruct->filename, filename, 0);
    } else if (icsStruct->filename[0] != '\0') {
        IcsStrCpy(buf, icsStruct->filename, ICS_MAXPATHLEN);
        IcsGetIcsName(icsStruct->filename, buf, 0);
    } else {
        return IcsErr_FOpenIcs;
    }

    fp = IcsFOpen(icsStruct->filename, "wb");
    ICSTR(fp == NULL, IcsErr_FOpenIcs);

    ICS_SET_LOCALE;

    line[0] = ICS_FIELD_SEP;
    line[1] = ICS_EOL;
    line[2] = '\0';
    error = icsAddLine(line, fp);

        /* Which ICS version is this file? */
    if (!error) {
        icsFirstText(line, ICS_VERSION);
        if (icsStruct->version == 1) {
            icsAddLastText(line, "1.0");
        } else {
            icsAddLastText(line, "2.0");
        }
        ICSCX(icsAddLine(line, fp));
    }

        /* Write the root of the filename: */
    if (!error) {
        IcsGetFileName(buf, icsStruct->filename);
        icsFirstText(line, ICS_FILENAME);
        icsAddLastText(line, buf);
        ICSCX(icsAddLine(line, fp));
    }

        /* Write all image descriptors: */
    ICSCX(writeIcsSource(icsStruct, fp));
    ICSCX(writeIcsLayout(icsStruct, fp));
    ICSCX(writeIcsRep(icsStruct, fp));
    ICSCX(writeIcsParam(icsStruct, fp));
    ICSCX(writeIcsSensorData(icsStruct, fp));
    ICSCX(writeIcsHistory(icsStruct, fp));
    ICSCX(markEndOfFile(icsStruct, fp));

    ICS_REVERT_LOCALE;

    if (fclose(fp) == EOF) {
        ICSCX(IcsErr_FCloseIcs); /* Don't overwrite any previous error. */
    }
    return error;
}
