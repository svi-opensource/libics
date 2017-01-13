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
 * FILE : libics_read.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsReadIcs()
 *   IcsVersion()
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libics_intern.h"


/* Find the index for "bits", which should be the first parameter. */
static int icsGetBitsParam(char order[ICS_MAXDIM+1][ICS_STRLEN_TOKEN],
                           int  parameters)
{
    int i;


    for (i = 0; i < parameters; i++) {
        if (strcmp(order[i], ICS_ORDER_BITS) == 0) {
            return i;
        }
    }

    return -1;
}


/* Like fgets(), gets a string from a stream. However, does not stop at newline
   character, but at 'sep'. It retains the 'sep' character at the end of the
   string; a null byte is appended. Also, it implements the solution to the
   CR/LF pair problem caused by some windows applications. If 'sep' is LF, it
   might be prepended by a CR. */
static char *icsFGetStr(char *line,
                        int   n,
                        FILE *fi,
                        char  sep)
{
        /* n == ICS_LINE_LENGTH */
    int i = 0;
    int ch;


        /* Imitate fgets() */
    while (i < n - 1) {
        ch = getc(fi);
        if (ch == EOF)
            break;

            /* Skip CR if next char is LF and sep is LF. */
        if (ch == '\r' && sep == '\n') {
            ch = getc(fi);
            if (ch != sep && ch != EOF) {
                ungetc(ch, fi);
                ch = '\r';
            }
        }

        line[i] =(char)ch;
        i++;
        if ((char)ch == sep)
            break;
    }
    line[i] = '\0';
    if (i != 0) {
            /* Read at least a 'sep' character */
        return line;
    } else {
            /* EOF at first getc() call */
        return NULL;
    }
}


/* Read the two ICS separators from file. There is a special case for ICS
   headers which are erroneously written under Windows in text mode causing a
   newline separator to be prepended by a carriage return.  Therefore when the
   second separator is a carriage return and the first separator is not a
   newline then peek at the third character to see if it is a newline.  If so
   then use newline as the second separator. Return IcsErr_FReadIcs on read
   errors and IcsErr_NotIcsFile on premature end-of-file. */
static Ics_Error getIcsSeparators(FILE *fi,
                                  char *seps)
{
    int sep1;
    int sep2;
    int sep3;


    sep1 = fgetc(fi);
    if (sep1 == EOF) {
        return (ferror(fi)) ? IcsErr_FReadIcs : IcsErr_NotIcsFile;
    }
    sep2 = fgetc(fi);
    if (sep2 == EOF) {
        return (ferror(fi)) ? IcsErr_FReadIcs : IcsErr_NotIcsFile;
    }
    if (sep1 == sep2) {
        return IcsErr_NotIcsFile;
    }
    if (sep2 == '\r' && sep1 != '\n') {
        sep3 = fgetc(fi);
        if (sep3 == EOF) {
            return (ferror(fi)) ? IcsErr_FReadIcs : IcsErr_NotIcsFile;
        } else {
            if (sep3 == '\n') {
                sep2 = '\n';
            } else {
                ungetc(sep3, fi);
            }
        }
    }
    seps[0] = sep1;
    seps[1] = sep2;
    seps[2] = '\0';

    return IcsErr_Ok;
}


static Ics_Error getIcsVersion(FILE       *fi,
                               const char *seps,
                               int        *ver)
{
    ICSINIT;
    char *word;
    char  line[ICS_LINE_LENGTH];


    ICSTR(icsFGetStr(line, ICS_LINE_LENGTH, fi, seps[1]) == NULL,
          IcsErr_FReadIcs);
    word = strtok(line, seps);
    ICSTR(word == NULL, IcsErr_NotIcsFile);
    ICSTR(strcmp(word, ICS_VERSION) != 0, IcsErr_NotIcsFile);
    word = strtok(NULL, seps);
    ICSTR(word == NULL, IcsErr_NotIcsFile);
    if (strcmp(word, "1.0") == 0) {
        *ver = 1;
    } else if (strcmp(word, "2.0") == 0) {
        *ver = 2;
    } else {
        error = IcsErr_NotIcsFile;
    }

    return error;
}


static Ics_Error getIcsFileName(FILE       *fi,
                                const char *seps)
{
    ICSINIT;
    char *word;
    char  line[ICS_LINE_LENGTH];


    ICSTR(icsFGetStr(line, ICS_LINE_LENGTH, fi, seps[1]) == NULL,
          IcsErr_FReadIcs);
    word = strtok(line, seps);
    ICSTR(word == NULL, IcsErr_NotIcsFile);
    ICSTR(strcmp(word, ICS_FILENAME) != 0, IcsErr_NotIcsFile);

    return error;
}


static Ics_Token getIcsToken(char           *str,
                              Ics_SymbolList *listSpec)
{
    int i;
    Ics_Token token = ICSTOK_NONE;


    if (str != NULL) {
        for (i = 0; i < listSpec->entries; i++) {
            if (strcmp(listSpec->list[i].name, str) == 0) {
                token = listSpec->list[i].token;
            }
        }
    }

    return token;
}


static Ics_Error getIcsCat(char       *str,
                           const char *seps,
                           Ics_Token  *cat,
                           Ics_Token  *subCat,
                           Ics_Token  *subSubCat)
{
    ICSINIT;
    char *token, buffer[ICS_LINE_LENGTH];


    *subCat = *subSubCat = ICSTOK_NONE;

    IcsStrCpy(buffer, str, ICS_LINE_LENGTH);
    token = strtok(buffer, seps);
    *cat = getIcsToken(token, &G_Categories);
    ICSTR(*cat == ICSTOK_NONE, IcsErr_MissCat);
    if ((*cat != ICSTOK_HISTORY) &&(*cat != ICSTOK_END)) {
        token = strtok(NULL, seps);
        *subCat = getIcsToken(token, &G_SubCategories);
        ICSTR(*subCat == ICSTOK_NONE, IcsErr_MissSubCat);
        if (*subCat == ICSTOK_SPARAMS) {
            token = strtok(NULL, seps);
            *subSubCat = getIcsToken(token, &G_SubSubCategories);
            ICSTR(*subSubCat == ICSTOK_NONE , IcsErr_MissSensorSubSubCat);
        }
    }

        /* Copy the remaining stuff into 'str' */
    if ((token = strtok(NULL, seps)) != NULL) {
        strcpy(str, token);
    }
    while ((token = strtok(NULL, seps)) != NULL) {
        IcsAppendChar(str, seps[0]);
        strcat(str, token);
    }

    return error;
}


Ics_Error IcsReadIcs(Ics_Header *icsStruct,
                     const char *filename,
                     int         forceName,
                     int         forceLocale)
{
    ICSDECL;
    ICS_INIT_LOCALE;
    FILE       *fp;
    int         end        = 0, i, j, bits;
    char        seps[3], *ptr, *data;
    char        line[ICS_LINE_LENGTH];
    Ics_Token   cat, subCat, subSubCat;
        /* These are temporary buffers to hold the data read until it is copied
           to the Ics_Header structure. This is needed because the Ics_Header
           structure is made to look more like we like to see images, compared
           to the way the data is written in the ICS file. */
    Ics_Format  format     = IcsForm_unknown;
    int         sign       = 1;
    int         parameters = 0;
    char        order[ICS_MAXDIM+1][ICS_STRLEN_TOKEN];
    size_t      sizes[ICS_MAXDIM+1];
    double      origin[ICS_MAXDIM+1];
    double      scale[ICS_MAXDIM+1];
    char        label[ICS_MAXDIM+1][ICS_STRLEN_TOKEN];
    char        unit[ICS_MAXDIM+1][ICS_STRLEN_TOKEN];


    for (i = 0; i < ICS_MAXDIM+1; i++) {
        sizes[i] = 1;
        origin[i] = 0.0;
        scale[i] = 1.0;
        order[i][0] = '\0';
        label[i][0] = '\0';
        unit[i][0] = '\0';
    }

    IcsInit(icsStruct);
    icsStruct->fileMode = IcsFileMode_read;

    IcsStrCpy(icsStruct->filename, filename, ICS_MAXPATHLEN);
    ICSXR(IcsOpenIcs(&fp, icsStruct->filename, forceName));

    if (forceLocale) {
        ICS_SET_LOCALE;
    }

    ICSCX(getIcsSeparators(fp, seps));

    ICSCX(getIcsVersion(fp, seps, &(icsStruct->version)));
    ICSCX(getIcsFileName(fp, seps));

    while (!end && !error
           && (icsFGetStr(line, ICS_LINE_LENGTH, fp, seps[1]) != NULL)) {
        if (getIcsCat(line, seps, &cat, &subCat, &subSubCat) != IcsErr_Ok)
            continue;
        ptr = strtok(line, seps);
        i = 0;
        switch (cat) {
            case ICSTOK_END:
                end = 1;
                if (icsStruct->srcFile[0] == '\0') {
                    icsStruct->srcOffset =(size_t) ftell(fp);
                    IcsStrCpy(icsStruct->srcFile, icsStruct->filename,
                              ICS_MAXPATHLEN);
                }
                break;
            case ICSTOK_SOURCE:
                switch (subCat) {
                    case ICSTOK_FILE:
                        if (ptr != NULL) {
                            IcsStrCpy(icsStruct->srcFile, ptr, ICS_MAXPATHLEN);
                        }
                        break;
                    case ICSTOK_OFFSET:
                        if (ptr != NULL) {
                            icsStruct->srcOffset = IcsStrToSize(ptr);
                        }
                        break;
                    default:
                        break;
                }
                break;
            case ICSTOK_LAYOUT:
                switch (subCat) {
                    case ICSTOK_PARAMS:
                        if (ptr != NULL) {
                            parameters = atoi(ptr);
                            if (parameters > ICS_MAXDIM+1) {
                                error = IcsErr_TooManyDims;
                            }
                        }
                        break;
                    case ICSTOK_ORDER:
                        while (ptr!= NULL && i < ICS_MAXDIM+1) {
                            IcsStrCpy(order[i++], ptr, ICS_STRLEN_TOKEN);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    case ICSTOK_SIZES:
                        while (ptr!= NULL && i < ICS_MAXDIM+1) {
                            sizes[i++] = IcsStrToSize(ptr);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    case ICSTOK_COORD:
                        if (ptr != NULL) {
                            IcsStrCpy(icsStruct->coord, ptr, ICS_STRLEN_TOKEN);
                        }
                        break;
                    case ICSTOK_SIGBIT:
                        if (ptr != NULL) {
                            icsStruct->imel.sigBits = IcsStrToSize(ptr);
                        }
                        break;
                    default:
                        error = IcsErr_MissLayoutSubCat;
                }
                break;
            case ICSTOK_REPRES:
                switch (subCat) {
                    case ICSTOK_FORMAT:
                        switch (getIcsToken(ptr, &G_Values)) {
                            case ICSTOK_FORMAT_INTEGER:
                                format = IcsForm_integer;
                                break;
                            case ICSTOK_FORMAT_REAL:
                                format = IcsForm_real;
                                break;
                            case ICSTOK_FORMAT_COMPLEX:
                                format = IcsForm_complex;
                                break;
                            default:
                                format = IcsForm_unknown;
                        }
                        break;
                    case ICSTOK_SIGN:
                    {
                        Ics_Token tok = getIcsToken(ptr, &G_Values);
                        if (tok == ICSTOK_SIGN_UNSIGNED) {
                            sign = 0;
                        } else {
                            sign = 1;
                        }
                        break;
                    }
                    case ICSTOK_SCILT:
                        if (ptr!= NULL) {
                            IcsStrCpy(icsStruct->scilType, ptr,
                                      ICS_STRLEN_TOKEN);
                        }
                        break;
                    case ICSTOK_COMPR:
                        switch (getIcsToken(ptr, &G_Values)) {
                            case ICSTOK_COMPR_UNCOMPRESSED:
                                icsStruct->compression = IcsCompr_uncompressed;
                                break;
                            case ICSTOK_COMPR_COMPRESS:
                                if (icsStruct->version == 1) {
                                    icsStruct->compression = IcsCompr_compress;
                                } else { /* A version 2.0 file never uses
                                            COMPRESS, maybe it means GZIP? */
                                    icsStruct->compression = IcsCompr_gzip;
                                }
                                break;
                            case ICSTOK_COMPR_GZIP:
                                icsStruct->compression = IcsCompr_gzip;
                                break;
                            default:
                                error = IcsErr_UnknownCompression;
                        }
                        break;
                    case ICSTOK_BYTEO:
                        while (ptr!= NULL && i < ICS_MAX_IMEL_SIZE) {
                            icsStruct->byteOrder[i++] = atoi(ptr);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    default:
                        error = IcsErr_MissRepresSubCat;
                        break;
                }
                break;
            case ICSTOK_PARAM:
                switch (subCat) {
                    case ICSTOK_ORIGIN:
                        while (ptr!= NULL && i < ICS_MAXDIM+1) {
                            origin[i++] = atof(ptr);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    case ICSTOK_SCALE:
                        while (ptr!= NULL && i < ICS_MAXDIM+1) {
                            scale[i++] = atof(ptr);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    case ICSTOK_UNITS:
                        while (ptr!= NULL && i < ICS_MAXDIM+1) {
                            IcsStrCpy(unit[i++], ptr, ICS_STRLEN_TOKEN);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    case ICSTOK_LABELS:
                        while (ptr!= NULL && i < ICS_MAXDIM+1) {
                            IcsStrCpy(label[i++], ptr, ICS_STRLEN_TOKEN);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    default:
                        error = IcsErr_MissParamSubCat;
                }
                break;
            case ICSTOK_HISTORY:
                if (ptr != NULL) {
                    data = strtok(NULL, seps+1); /* This will get the rest of
                                                    the line */
                    if (data == NULL) { /* data is not allowed to be "", but ptr
                                           is */
                        data = ptr;
                        ptr = "";
                    }
                        /* The next portion is to avoid having
                           IcsInternAddHistory return IcsErr_LineOverflow. */
                    i = strlen(ptr);
                    if (i+1 > ICS_STRLEN_TOKEN) {
                        ptr[ICS_STRLEN_TOKEN-1] = '\0';
                        i = ICS_STRLEN_TOKEN-1;
                    }
                    j = strlen(ICS_HISTORY);
                    if ((strlen(data) + i + j + 4) > ICS_LINE_LENGTH) {
                        data[ICS_LINE_LENGTH - i - j - 4] = '\0';
                    }
                    error = IcsInternAddHistory(icsStruct, ptr, data, seps);
                }
                break;
            case ICSTOK_SENSOR:
                switch (subCat) {
                    case ICSTOK_TYPE:
                        while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                            IcsStrCpy(icsStruct->type[i++], ptr,
                                      ICS_STRLEN_TOKEN);
                            ptr = strtok(NULL, seps);
                        }
                        break;
                    case ICSTOK_MODEL:
                        if (ptr != NULL) {
                            IcsStrCpy(icsStruct->model, ptr, ICS_STRLEN_OTHER);
                        }
                        break;
                    case ICSTOK_SPARAMS:
                        switch (subSubCat) {
                            case ICSTOK_CHANS:
                                if (ptr != NULL) {
                                    int v = atoi(ptr);
                                    icsStruct->sensorChannels = v;
                                    if (v > ICS_MAX_LAMBDA) {
                                        error = IcsErr_TooManyChans;
                                    }
                                }
                                break;
                            case ICSTOK_PINHRAD:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->pinholeRadius[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_LAMBDEX:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->lambdaEx[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_LAMBDEM:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->lambdaEm[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_PHOTCNT:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->exPhotonCnt[i++] = atoi(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_REFRIME:
                                if (ptr != NULL) {
                                    icsStruct->refrInxMedium = atof(ptr);
                                }
                                break;
                            case ICSTOK_NUMAPER:
                                if (ptr != NULL) {
                                    icsStruct->numAperture = atof(ptr);
                                }
                                break;
                            case ICSTOK_REFRILM:
                                if (ptr != NULL) {
                                    icsStruct->refrInxLensMedium = atof(ptr);
                                }
                                break;
                            case ICSTOK_PINHSPA:
                                if (ptr != NULL) {
                                    icsStruct->pinholeSpacing = atof(ptr);
                                }
                                break;
                            case ICSTOK_STEDDEPLMODE:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    IcsStrCpy(icsStruct->stedDepletionMode[i++],
                                              ptr, ICS_STRLEN_TOKEN);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_STEDLAMBDA:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->stedLambda[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_STEDSATFACTOR:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->stedSatFactor[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_STEDIMMFRACTION:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->stedImmFraction[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_STEDVPPM:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->stedVPPM[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_DETPPU:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    icsStruct->detectorPPU[i++] = atof(ptr);
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_DETBASELINE:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    double v = atof(ptr);
                                    icsStruct->detectorBaseline[i++] = v;
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            case ICSTOK_DETLNAVGCNT:
                                while (ptr != NULL && i < ICS_MAX_LAMBDA) {
                                    double v = atof(ptr);
                                    icsStruct->detectorLineAvgCnt[i++] = v;
                                    ptr = strtok(NULL, seps);
                                }
                                break;
                            default:
                                error = IcsErr_MissSensorSubSubCat;
                        }
                        break;
                    default:
                        error = IcsErr_MissSensorSubCat;
                }
                break;
            default:
                error = IcsErr_MissCat;
        }
    }

        /* In newer libics versions(> 1.5.2) a microscope type is specified per
           sensor channel. For files from previous libics versions a single
           microscope type is stored. To allow compatibility. when reading older
           files in which a single microscope type is defined and multiple
           sensor channels, the microscope type will be duplicated to all sensor
           channels. */
    for (j = 1; j < icsStruct->sensorChannels; j++) {
        if (strlen(icsStruct->type[j]) == 0) {
            IcsStrCpy(icsStruct->type[j], icsStruct->type[0],
                       ICS_STRLEN_TOKEN);
        }
    }

    if (!error) {
        bits = icsGetBitsParam(order, parameters);
        if (bits < 0) {
            error = IcsErr_MissBits;
        } else {
            IcsGetDataTypeProps(&(icsStruct->imel.dataType), format, sign,
                                sizes[bits]);
            for (j = 0, i = 0; i < parameters; i++) {
                if (i == bits) {
                    icsStruct->imel.origin = origin[i];
                    icsStruct->imel.scale = scale[i];
                    strcpy(icsStruct->imel.unit, unit[i]);
                } else {
                    icsStruct->dim[j].size = sizes[i];
                    icsStruct->dim[j].origin = origin[i];
                    icsStruct->dim[j].scale = scale[i];
                    strcpy(icsStruct->dim[j].order, order[i]);
                    strcpy(icsStruct->dim[j].label, label[i]);
                    strcpy(icsStruct->dim[j].unit, unit[i]);
                    j++;
                }
            }
            icsStruct->dimensions = parameters - 1;
        }
    }

    if (forceLocale) {
        ICS_REVERT_LOCALE;
    }

    if (fclose(fp) == EOF) {
        ICSCX(IcsErr_FCloseIcs); /* Don't overwrite any previous error. */
    }
    return error;
}


/* Read the first 3 lines of an ICS file to see which version it is. It returns
   0 if it is not an ICS file, or the version number if it is. */
int IcsVersion(const char *filename,
               int         forceName)
{
    ICSDECL;
    ICS_INIT_LOCALE;
    int   version;
    FILE *fp;
    char  FileName[ICS_MAXPATHLEN];
    char  seps[3];


    IcsStrCpy(FileName, filename, ICS_MAXPATHLEN);
    error = IcsOpenIcs(&fp, FileName, forceName);
    ICSTR(error, 0);
    version = 0;
    ICS_SET_LOCALE;
    if (!error) error = getIcsSeparators(fp, seps);
    if (!error) error = getIcsVersion(fp, seps, &version);
    if (!error) error = getIcsFileName(fp, seps);
    ICS_REVERT_LOCALE;
    if (fclose(fp) == EOF) {
        return 0;
    }
    return error ? 0 : version;
}
