/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright (C) 2000-2013 Cris Luengo and others
 * Copyright 2015-2017:
 *   Scientific Volume Imaging Holding B.V.
 *   Laapersveld 63, 1213 VB Hilversum, The Netherlands
 *   https://www.svi.nl
 *
 * Contact: libics@svi.nl
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
 * FILE : libics_binary.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsWriteIds()
 *   IcsCopyIds()
 *   IcsOpenIds ()
 *   IcsCloseIds ()
 *   IcsReadIdsBlock ()
 *   IcsSkipIdsBlock ()
 *   IcsSetIdsBlock ()
 *   IcsReadIds()
 *
 * The following internal functions are contained in this file:
 *
 *   IcsWritePlainWithStrides()
 *   IcsFillByteOrder()
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libics_intern.h"


#if defined(ICS_DO_GZEXT) && !defined(ICS_ZLIB)
#undef ICS_DO_GZEXT
#endif


/* Write uncompressed data, with strides. */
Ics_Error IcsWritePlainWithStrides(const void   *src,
                                   const size_t *dim,
                                   const size_t *stride,
                                   int           ndims,
                                   int           nbytes,
                                   FILE         *file)
{
    ICSINIT;
    size_t      curpos[ICS_MAXDIM];
    const char *data;
    int         i;
    size_t      j;


    for (i = 0; i < ndims; i++) {
        curpos[i] = 0;
    }
    while (1) {
        data = (char const*)src;
        for (i = 1; i < ndims; i++) { /* curpos[0]==0 here */
            data += curpos[i] * stride[i] * nbytes;
        }
        if (stride[0] == 1) {
            if (fwrite(data, nbytes, dim[0], file) != dim[0]) {
                return IcsErr_FWriteIds;
            }
        } else {
            for (j = 0; j < dim[0]; j++) {
                if (fwrite(data, nbytes, 1, file) != 1) {
                    return IcsErr_FWriteIds;
                }
                data += stride[0] * nbytes;
         }
        }
        for (i = 1; i < ndims; i++) {
            curpos[i]++;
            if (curpos[i] < dim[i]) {
                break;
            }
            curpos[i] = 0;
        }
        if (i == ndims) {
            break; /* we're done writing */
        }
    }

    return error;
}


/* Write the data to an IDS file. */
Ics_Error IcsWriteIds(const Ics_Header *IcsStruct)
{
    ICSINIT;
    FILE   *fp;
    char    filename[ICS_MAXPATHLEN];
    char    mode[3] = "wb";
    int     i;
    size_t  dim[ICS_MAXDIM];


    if (IcsStruct->Version == 1) {
        IcsGetIdsName(filename, IcsStruct->Filename);
    } else {
        ICSTR(IcsStruct->SrcFile[0] != '\0', IcsErr_Ok);
            /* Do nothing: the data is in another file somewhere */
        IcsStrCpy(filename, IcsStruct->Filename, ICS_MAXPATHLEN);
        mode[0] = 'a'; /* Open for append */
    }
    ICSTR((IcsStruct->Data == NULL) || (IcsStruct->DataLength == 0),
          IcsErr_MissingData);

    fp = IcsFOpen(filename, mode);
    ICSTR(fp == NULL, IcsErr_FOpenIds);

    for (i=0; i<IcsStruct->Dimensions; i++) {
        dim[i] = IcsStruct->Dim[i].Size;
    }
    switch (IcsStruct->Compression) {
        case IcsCompr_uncompressed:
            if (IcsStruct->DataStrides) {
                size_t size = IcsGetDataTypeSize(IcsStruct->Imel.DataType);
                error = IcsWritePlainWithStrides(IcsStruct->Data, dim,
                                                 IcsStruct->DataStrides,
                                                 IcsStruct->Dimensions,
                                                 size, fp);
            } else {
                    /* We do the writing in blocks if the data is very large,
                       this avoids a bug in some c library implementations on
                       windows. */
                size_t n = IcsStruct->DataLength;
                const size_t nwrite = 1024 * 1024 * 1024;
                char *p = (char*)IcsStruct->Data;
                while (error == IcsErr_Ok && n > 0) {
                    if (n >= nwrite) {
                        if (fwrite(p, 1, nwrite, fp) != nwrite) {
                            error = IcsErr_FWriteIds;
                        }
                        n -= nwrite;
                        p += nwrite;
                    } else {
                        if (fwrite(p, 1, n, fp) != n) {
                            error = IcsErr_FWriteIds;
                        }
                        n = 0;
                    }
                }
            }
            break;
#ifdef ICS_ZLIB
        case IcsCompr_gzip:
            if (IcsStruct->DataStrides) {
                size_t size = IcsGetDataTypeSize(IcsStruct->Imel.DataType);
                error = IcsWriteZipWithStrides(IcsStruct->Data, dim,
                                               IcsStruct->DataStrides,
                                               IcsStruct->Dimensions,
                                               size, fp, IcsStruct->CompLevel);
            } else {
                error = IcsWriteZip(IcsStruct->Data, IcsStruct->DataLength, fp,
                                    IcsStruct->CompLevel);
            }
            break;
#endif
        default:
            error = IcsErr_UnknownCompression;
    }

    if (fclose (fp) == EOF) {
        ICSCX(IcsErr_FCloseIds); /* Don't overwrite any previous error. */
    }
    return error;
}


/* Append image data from infilename at inoffset to outfilename. If outfilename
   is a .ics file it must end with the END keyword. */
Ics_Error IcsCopyIds(const char *infilename,
                     size_t      inoffset,
                     const char *outfilename)
{
    ICSINIT;
    FILE   *in     = 0;
    FILE   *out    = 0;
    char   *buffer = 0;
    int     done   = 0;
    size_t  n;


        /* Open files */
    in = IcsFOpen(infilename, "rb");
    if (in == NULL) {
        error = IcsErr_FCopyIds;
        goto exit;
    }
    if (fseek(in, inoffset, SEEK_SET) != 0) {
        error = IcsErr_FCopyIds;
        goto exit;
    }
    out = IcsFOpen(outfilename, "ab");
    if (out == NULL) {
        error = IcsErr_FCopyIds;
        goto exit;
    }
        /* Create an output buffer */
    buffer = (char*)malloc(ICS_BUF_SIZE);
    if (buffer == NULL) {
        error = IcsErr_Alloc;
        goto exit;
    }
    while (!done) {
        n = fread(buffer, 1, ICS_BUF_SIZE, in);
        if (feof (in)) {
            done = 1;
        } else if (n != ICS_BUF_SIZE) {
            error = IcsErr_FCopyIds;
            goto exit;
        }
        if (fwrite(buffer, 1, n, out) != n) {
            error = IcsErr_FCopyIds;
            goto exit;
        }
    }

  exit:
    if (buffer) free(buffer);
    if (in) fclose(in);
    if (out) fclose(out);
    return error;
}


/* Check if a file exist. */
static int IcsExistFile(const char *filename)
{
    FILE *fp;


    if ((fp = IcsFOpen(filename, "rb")) != NULL) {
        fclose (fp);
        return 1;
    } else {
        return 0;
    }
}


/* Find out if we are running on a little endian machine (Intel) or on a big
   endian machine. On Intel CPUs the least significant byte is stored first in
   memory. Returns: 1 if little endian; 0 big endian (e.g. MIPS). */
static int IcsIsLittleEndianMachine(void)
{
    int i = 1;
    char *cptr = (char*)&i;
    return (*cptr == 1);
}


/* Fill the byte order array with the machine's byte order. */
void IcsFillByteOrder(int bytes,
                      int machineByteOrder[ICS_MAX_IMEL_SIZE])
{
    int i;


    if (bytes > ICS_MAX_IMEL_SIZE) {
            /* This will cause problems if undetected, */
            /* but shouldn't happen anyway */
        bytes = ICS_MAX_IMEL_SIZE;
    }

    if (IcsIsLittleEndianMachine()) {
            /* Fill byte order for a little endian machine. */
        for (i = 0; i < bytes; i++) {
            machineByteOrder[i] = 1 + i;
        }
    } else {
            /* Fill byte order for a big endian machine. */
        for (i = 0; i < bytes; i++) {
            machineByteOrder[i] = bytes - i;
        }
    }
}


/* Reorder the bytes in the images as specified in the ByteOrder array. */
static Ics_Error IcsReorderIds(char   *buf,
                               size_t  length,
                               int     srcByteOrder[ICS_MAX_IMEL_SIZE],
                               int     bytes)
{
    ICSINIT;
    int  i, j, imels;
    int  dstByteOrder[ICS_MAX_IMEL_SIZE];
    char imel[ICS_MAX_IMEL_SIZE];
    int  different = 0, empty = 0;


    imels = length / bytes;
    ICSTR( length % bytes != 0, IcsErr_BitsVsSizeConfl );

        /* Create destination byte order: */
    IcsFillByteOrder(bytes, dstByteOrder);

        /* Localize byte order array: */
    for (i = 0; i < bytes; i++){
        different |= (srcByteOrder[i] != dstByteOrder[i]);
        empty |= !(srcByteOrder[i]);
    }
    ICSTR(!different || empty, IcsErr_Ok);

    for (j = 0; j < imels; j++){
        for (i = 0; i < bytes; i++){
            imel[i] = buf[srcByteOrder[i]-1];
        }
        for (i = 0; i < bytes; i++){
            buf[dstByteOrder[i]-1] = imel[i];
        }
        buf += bytes;
    }

    return error;
}


/* Open an IDS file for reading. */
Ics_Error IcsOpenIds(Ics_Header *IcsStruct)
{
    ICSINIT;
    Ics_BlockRead *br;
    char           filename[ICS_MAXPATHLEN];
    size_t         offset = 0;


    if (IcsStruct->BlockRead != NULL) {
        ICSXR( IcsCloseIds (IcsStruct) );
    }
    if (IcsStruct->Version == 1) {          /* Version 1.0 */
        IcsGetIdsName(filename, IcsStruct->Filename);
#ifdef ICS_DO_GZEXT
            /* If the .ids file does not exist then maybe the .ids.gz or .ids.Z
             * file exists. */
        if (!IcsExistFile(filename)) {
            if (strlen(filename) < ICS_MAXPATHLEN - 4) {
                strcat(filename, ".gz");
                if (IcsExistFile(filename)) {
                    IcsStruct->Compression = IcsCompr_gzip;
                } else {
                    strcpy(filename + strlen(filename) - 3, ".Z");
                    if (IcsExistFile(filename)) {
                        IcsStruct->Compression = IcsCompr_compress;
                    } else {
                        return IcsErr_FOpenIds;
                    }
                }
            }
        }
#endif
    } else {                                  /* Version 2.0 */
        ICSTR(IcsStruct->SrcFile[0] == '\0', IcsErr_MissingData);
        IcsStrCpy(filename, IcsStruct->SrcFile, ICS_MAXPATHLEN);
        offset = IcsStruct->SrcOffset;
    }

    br = (Ics_BlockRead*)malloc(sizeof (Ics_BlockRead));
    ICSTR(br == NULL, IcsErr_Alloc);

    br->DataFilePtr = IcsFOpen(filename, "rb");
    ICSTR(br->DataFilePtr == NULL, IcsErr_FOpenIds);
    if (fseek(br->DataFilePtr, offset, SEEK_SET) != 0) {
        fclose(br->DataFilePtr);
        free(br);
        return IcsErr_FReadIds;
    }
#ifdef ICS_ZLIB
    br->ZlibStream = NULL;
    br->ZlibInputBuffer = NULL;
#endif
    br->CompressRead = 0;
    IcsStruct->BlockRead = br;

#ifdef ICS_ZLIB
    if (IcsStruct->Compression == IcsCompr_gzip) {
        error = IcsOpenZip(IcsStruct);
        if (error) {
            fclose (br->DataFilePtr);
            free(IcsStruct->BlockRead);
            IcsStruct->BlockRead = NULL;
            return error;
        }
    }
#endif

    return error;
}


/* Close an IDS file for reading. */
Ics_Error IcsCloseIds(Ics_Header *IcsStruct)
{
    ICSINIT;
    Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;


    if (br->DataFilePtr && fclose(br->DataFilePtr) == EOF) {
        error = IcsErr_FCloseIds;
    }
#ifdef ICS_ZLIB
    if (br->ZlibStream != NULL) {
        ICSXA(IcsCloseZip(IcsStruct));
    }
#endif
    free(br);
    IcsStruct->BlockRead = NULL;

    return error;
}


/* Read a data block from an IDS file. */
Ics_Error IcsReadIdsBlock(Ics_Header *IcsStruct,
                          void       *dest,
                          size_t      n)
{
    ICSINIT;
    Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;


    switch (IcsStruct->Compression) {
        case IcsCompr_uncompressed:
            if ((fread(dest, 1, n, br->DataFilePtr)) != n) {
                if (ferror(br->DataFilePtr)) {
                    error = IcsErr_FReadIds;
                } else {
                    error = IcsErr_EndOfStream;
                }
            }
            break;
#ifdef ICS_ZLIB
        case IcsCompr_gzip:
            error = IcsReadZipBlock(IcsStruct, dest, n);
            break;
#endif
        case IcsCompr_compress:
            if (br->CompressRead) {
                error = IcsErr_BlockNotAllowed;
            } else {
                error = IcsReadCompress(IcsStruct, dest, n);
                br->CompressRead = 1;
            }
            break;
        default:
            error = IcsErr_UnknownCompression;
    }

    ICSCX(IcsReorderIds((char*)dest, n, IcsStruct->ByteOrder,
                        IcsGetBytesPerSample(IcsStruct)));

    return error;
}


/* Skip a data block from an IDS file. */
Ics_Error IcsSkipIdsBlock(Ics_Header *IcsStruct,
                          size_t      n)
{
    return IcsSetIdsBlock (IcsStruct, n, SEEK_CUR);
}


/* Sets the file pointer into the IDS file. */
Ics_Error IcsSetIdsBlock(Ics_Header *IcsStruct,
                         long        offset,
                         int         whence)
{
    ICSINIT;
    Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;


    switch (IcsStruct->Compression) {
        case IcsCompr_uncompressed:
            switch (whence) {
                case SEEK_SET:
                case SEEK_CUR:
                    if (fseek(br->DataFilePtr, (long)offset, whence) != 0) {
                        if (ferror(br->DataFilePtr)) {
                            error = IcsErr_FReadIds;
                        } else {
                            error = IcsErr_EndOfStream;
                        }
                    }
                    break;
                default:
                    error = IcsErr_IllParameter;
            }
            break;
#ifdef ICS_ZLIB
        case IcsCompr_gzip:
            switch (whence) {
                case SEEK_SET:
                case SEEK_CUR:
                    error = IcsSetZipBlock(IcsStruct, offset, whence);
                    break;
                default:
                    error = IcsErr_IllParameter;
            }
            break;
#endif
        case IcsCompr_compress:
            error = IcsErr_BlockNotAllowed;
            break;
        default:
            error = IcsErr_UnknownCompression;
    }

    return error;
}


/* Read the data from an IDS file. */
Ics_Error IcsReadIds(Ics_Header *IcsStruct,
                     void       *dest,
                     size_t      n)
{
    ICSDECL;

    ICSXR(IcsOpenIds(IcsStruct));
    error = IcsReadIdsBlock(IcsStruct, dest, n);
    ICSXA( IcsCloseIds(IcsStruct) );

    return error;
}
