/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright (C) 2000-2010 Cris Luengo and others
 * email: clluengo@users.sourceforge.net
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

/*
 * Write uncompressed data, with strides.
 */
Ics_Error IcsWritePlainWithStrides (void const* src, size_t const* dim,
                                    size_t const* stride, int ndims, int nbytes,
                                    FILE* file)
{
   ICSINIT;
   size_t curpos[ICS_MAXDIM];
   char const* data;
   int ii;

   for (ii = 0; ii < ndims; ii++) {
      curpos[ii] = 0;
   }
   while (1) {
      data = (char const*)src;
      for (ii = 1; ii < ndims; ii++) { /* curpos[0]==0 here */
         data += curpos[ii]*stride[ii]*nbytes;
      }
      if (stride[0] == 1) {
         if (fwrite (data, nbytes, dim[0], file) != dim[0]) {
            return IcsErr_FWriteIds;
         }
      }
      else {
         for (ii = 0; ii < dim[0]; ii++) {
            if (fwrite (data, nbytes, 1, file) != 1) {
               return IcsErr_FWriteIds;
            }
            data += stride[0]*nbytes;
         }
      }
      for (ii = 1; ii < ndims; ii++) {
         curpos[ii]++;
         if (curpos[ii] < dim[ii]) {
            break;
         }
         curpos[ii] = 0;
      }
      if (ii == ndims) {
         break; /* we're done writing */
      }
   }

   return error;
}

/*
 * Write the data to an IDS file.
 */
Ics_Error IcsWriteIds (Ics_Header const* IcsStruct)
{
   ICSINIT;
   FILE* fp;
   char filename[ICS_MAXPATHLEN];
   char mode[3] = "wb";
   int ii;
   size_t dim[ICS_MAXDIM];

   if (IcsStruct->Version == 1) {
      IcsGetIdsName (filename, IcsStruct->Filename);
   }
   else {
      ICSTR( IcsStruct->SrcFile[0] != '\0', IcsErr_Ok );
         /* Do nothing: the data is in another file somewhere */
      IcsStrCpy (filename, IcsStruct->Filename, ICS_MAXPATHLEN);
      mode[0] = 'a'; /* Open for append */
   }
   ICSTR( (IcsStruct->Data == NULL) || (IcsStruct->DataLength == 0), IcsErr_MissingData );

   fp = fopen (filename, mode);
   ICSTR( fp == NULL, IcsErr_FOpenIds );

   for (ii=0; ii<IcsStruct->Dimensions; ii++) {
      dim[ii] = IcsStruct->Dim[ii].Size;
   }
   switch (IcsStruct->Compression) {
      case IcsCompr_uncompressed:
         if (IcsStruct->DataStrides) {
            error = IcsWritePlainWithStrides (IcsStruct->Data, dim, IcsStruct->DataStrides,
                       IcsStruct->Dimensions, IcsGetDataTypeSize (IcsStruct->Imel.DataType), fp);
         }
         else {
            if (fwrite (IcsStruct->Data, 1, IcsStruct->DataLength, fp) != IcsStruct->DataLength) {
               error = IcsErr_FWriteIds;
            }
         }
         break;
#ifdef ICS_ZLIB
      case IcsCompr_gzip:
         if (IcsStruct->DataStrides) {
            error = IcsWriteZipWithStrides (IcsStruct->Data, dim, IcsStruct->DataStrides,
                       IcsStruct->Dimensions, IcsGetDataTypeSize (IcsStruct->Imel.DataType), fp,
                       IcsStruct->CompLevel);
         }
         else {
            error = IcsWriteZip (IcsStruct->Data, IcsStruct->DataLength, fp, IcsStruct->CompLevel);
         }
         break;
#endif
      default:
         error = IcsErr_UnknownCompression;
   }

   if (fclose (fp) == EOF) {
      ICSCX (IcsErr_FCloseIds); /* Don't overwrite any previous error. */
   }
   return error;
}

/*
 * Append image data from infilename at inoffset to outfilename.
 * If outfilename is a .ics file it must end with the END keyword.
 */
Ics_Error IcsCopyIds (char const* infilename, size_t inoffset, char const* outfilename)
{
   ICSINIT;
   FILE* in = 0;
   FILE* out = 0;
   char* buffer = 0;
   int done = 0, n;

   /* Open files */
   in = fopen (infilename, "rb");
   if (in == NULL) {
      error = IcsErr_FCopyIds;
      goto quitcopy;
   }
   if (fseek (in, inoffset, SEEK_SET) != 0) {
      error = IcsErr_FCopyIds;
      goto quitcopy;
   }
   out = fopen (outfilename, "ab");
   if (out == NULL) {
      error = IcsErr_FCopyIds;
      goto quitcopy;
   }
   /* Create an output buffer */
   buffer = (char*)malloc (ICS_BUF_SIZE);
   if (buffer == NULL) {
      error = IcsErr_Alloc;
      goto quitcopy;
   }
   while (!done) {
      n = fread (buffer, 1, ICS_BUF_SIZE, in);
      if (feof (in)) {
         done = 1;
      }
      else if (n != ICS_BUF_SIZE) {
         error = IcsErr_FCopyIds;
         goto quitcopy;
      }
      if (fwrite (buffer, 1, n, out) != n) {
         error = IcsErr_FCopyIds;
         goto quitcopy;
      }
   }

quitcopy:
   if (buffer) free (buffer);
   if (in)     fclose (in);
   if (out)    fclose (out);
   return error;
}

/*
 * Does the file exist?
 */
static int IcsExistFile (char const* filename)
{
   FILE* fp;

   if ((fp = fopen (filename, "rb")) != NULL) {
      fclose (fp);
      return 1;
   }
   else {
      return 0;
   }
}

/*
 * Find out if we are running on a little endian machine (Intel)
 * or on a big endian machine. On Intel CPUs the least significant
 * byte is stored first in memory.
 * Returns: 1 if little endian; 0 big endian (e.g. MIPS).
 */
static int IcsIsLittleEndianMachine (void)
{
   int i = 1;
   char* cptr = (char*)&i;
   return (*cptr == 1);
}

/*
 * Fill the byte order array with the machine's byte order.
 */
void IcsFillByteOrder (int bytes, int machineByteOrder[ICS_MAX_IMEL_SIZE])
{
   int ii;

   if (bytes > ICS_MAX_IMEL_SIZE) {
      /* This will cause problems if undetected, */
      /* but shouldn't happen anyway */
      bytes = ICS_MAX_IMEL_SIZE;
   }

   if (IcsIsLittleEndianMachine ()) {
      /* Fill byte order for a little endian machine. */
      for (ii = 0; ii < bytes; ii++) {
         machineByteOrder[ii] = 1 + ii;
      }
   }
   else {
      /* Fill byte order for a big endian machine. */
      for (ii = 0; ii < bytes; ii++) {
         machineByteOrder[ii] = bytes - ii;
      }
   }
}

/*
 * Reorder the bytes in the images as specified in the ByteOrder array.
 */
static Ics_Error IcsReorderIds (char* buf, size_t length,
                                int srcByteOrder[ICS_MAX_IMEL_SIZE], int bytes)
{
   ICSINIT;
   int ii, jj, imels;
   /*int srcByteOrder[ICS_MAX_IMEL_SIZE];*/
   int dstByteOrder[ICS_MAX_IMEL_SIZE];
   char imel[ICS_MAX_IMEL_SIZE];
   int different = 0, empty = 0;

   imels = length / bytes;
   ICSTR( length % bytes != 0, IcsErr_BitsVsSizeConfl );

   /* Create destination byte order: */
   IcsFillByteOrder (bytes, dstByteOrder);

   /* Localize byte order array: */
   for (ii = 0; ii < bytes; ii++){
      /*srcByteOrder[ii] = ByteOrder[ii];*/
      different |= (srcByteOrder[ii] != dstByteOrder[ii]);
      empty |= !(srcByteOrder[ii]);
   }
   ICSTR( !different || empty, IcsErr_Ok );

   for (jj = 0; jj < imels; jj++){
      for (ii = 0; ii < bytes; ii++){
         imel[ii] = buf[srcByteOrder[ii]-1];
      }
      for (ii = 0; ii < bytes; ii++){
         buf[dstByteOrder[ii]-1] = imel[ii];
      }
      buf += bytes;
   }

   return error;
}

/*
 * Open an IDS file for reading.
 */
Ics_Error IcsOpenIds (Ics_Header* IcsStruct)
{
   ICSINIT;
   Ics_BlockRead* br;
   char filename[ICS_MAXPATHLEN];
   size_t offset = 0;

   if (IcsStruct->BlockRead != NULL) {
      ICSXR( IcsCloseIds (IcsStruct) );
   }
   if (IcsStruct->Version == 1) {          /* Version 1.0 */
      IcsGetIdsName (filename, IcsStruct->Filename);
#ifdef ICS_DO_GZEXT
      /* If the .ids file does not exist then maybe
       * the .ids.gz or .ids.Z file exists.
       */
      if (!IcsExistFile (filename)) {
         if (strlen(filename) < ICS_MAXPATHLEN - 4) {
            strcat(filename, ".gz");
            if (IcsExistFile (filename)) {
               IcsStruct->Compression = IcsCompr_gzip;
            }
            else {
               strcpy(filename + strlen(filename) - 3, ".Z");
               if (IcsExistFile (filename)) {
                  IcsStruct->Compression = IcsCompr_compress;
               }
               else {
                  return IcsErr_FOpenIds;
               }
            }
         }
      }
#endif
   }
   else {                                  /* Version 2.0 */
      ICSTR( IcsStruct->SrcFile[0] == '\0', IcsErr_MissingData );
      IcsStrCpy (filename, IcsStruct->SrcFile, ICS_MAXPATHLEN);
      offset = IcsStruct->SrcOffset;
   }

   br = (Ics_BlockRead*)malloc (sizeof (Ics_BlockRead));
   ICSTR( br == NULL, IcsErr_Alloc );

   br->DataFilePtr = fopen (filename, "rb");
   ICSTR( br->DataFilePtr == NULL, IcsErr_FOpenIds );
   if (fseek (br->DataFilePtr, offset, SEEK_SET) != 0) {
      fclose (br->DataFilePtr);
      free (br);
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
      error = IcsOpenZip (IcsStruct);
      if (error) {
         fclose (br->DataFilePtr);
         free (IcsStruct->BlockRead);
         IcsStruct->BlockRead = NULL;
         return error;
      }
   }
#endif

   return error;
}

/*
 * Close an IDS file for reading.
 */
Ics_Error IcsCloseIds (Ics_Header* IcsStruct)
{
   ICSINIT;
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;

   if (br->DataFilePtr && fclose (br->DataFilePtr) == EOF) {
      error = IcsErr_FCloseIds;
   }
#ifdef ICS_ZLIB
   if (br->ZlibStream != NULL) {
      ICSXA( IcsCloseZip (IcsStruct) );
   }
#endif
   free (br);
   IcsStruct->BlockRead = NULL;

   return error;
}

/*
 * Read a data block from an IDS file.
 */
Ics_Error IcsReadIdsBlock (Ics_Header* IcsStruct, void* dest, size_t n)
{
   ICSINIT;
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;

   switch (IcsStruct->Compression) {
      case IcsCompr_uncompressed:
         if ((fread (dest, 1, n, br->DataFilePtr)) != n) {
            error = ferror(br->DataFilePtr) ? IcsErr_FReadIds : IcsErr_EndOfStream;
         }
         break;
#ifdef ICS_ZLIB
      case IcsCompr_gzip:
         error = IcsReadZipBlock (IcsStruct, dest, n);
         break;
#endif
      case IcsCompr_compress:
         if (br->CompressRead) {
            error = IcsErr_BlockNotAllowed;
         } else {
            error = IcsReadCompress (IcsStruct, dest, n);
            br->CompressRead = 1;
         }
         break;
      default:
         error = IcsErr_UnknownCompression;
   }

   ICSCX( IcsReorderIds ((char*)dest, n, IcsStruct->ByteOrder, IcsGetBytesPerSample (IcsStruct)) );

   return error;
}

/*
 * Skip a data block from an IDS file.
 */
Ics_Error IcsSkipIdsBlock (Ics_Header* IcsStruct, size_t n)
{
   return IcsSetIdsBlock (IcsStruct, n, SEEK_CUR);
}

/*
 * Sets the file pointer into the IDS file.
 */
Ics_Error IcsSetIdsBlock (Ics_Header* IcsStruct, long offset, int whence)
{
   ICSINIT;
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;

   switch (IcsStruct->Compression) {
      case IcsCompr_uncompressed:
         switch (whence) {
            case SEEK_SET:
            case SEEK_CUR:
               if (fseek(br->DataFilePtr, (long)offset, whence) != 0) {
                  error = ferror(br->DataFilePtr) ? IcsErr_FReadIds : IcsErr_EndOfStream;
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
               error = IcsSetZipBlock (IcsStruct, offset, whence);
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

/*
 * Read the data from an IDS file.
 */
Ics_Error IcsReadIds (Ics_Header* IcsStruct, void* dest, size_t n)
{
   ICSDECL;

   ICSXR( IcsOpenIds (IcsStruct) );
   error = IcsReadIdsBlock (IcsStruct, dest, n);
   ICSXA( IcsCloseIds (IcsStruct) );

   return error;
}
