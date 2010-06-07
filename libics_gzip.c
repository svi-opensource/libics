/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright (C) 2000-2006 Cris Luengo and others
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
 * FILE : libics_gzip.c
 *
 * The following internal functions are contained in this file:
 *
 *   IcsWriteZip()
 *   IcsWriteZipWithStrides()
 *   IcsOpenZip ()
 *   IcsCloseZip ()
 *   IcsReadZipBlock ()
 *   IcsSetZipBlock ()
 *
 * This is the only file that contains any zlib dependancies.
 *
 * Because of a defect in the zlib interface, the only way of using gzread
 * and gzwrite on streams that are already open is through file handles (which
 * are not ANSI C). The weird thing is that zlib creates a stream from this
 * handle to do its stuff. To avoid using non-ANSI C functions, I copied a
 * large part of gzio.c (simplifying it to do just what I needed it to do).
 * Therefore, most of the code in this file was written by Jean-loup Gailly.
 *    (Copyright (C) 1995-1998 Jean-loup Gailly)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libics_intern.h"

#ifdef ICS_ZLIB
#include "zlib.h"
#endif

#define DEF_MEM_LEVEL 8 /* Default value defined in zutil.h */

/* GZIP stuff */
#ifdef WIN32
#define OS_CODE 0x0b
#else
#define OS_CODE 0x03  /* assume Unix */
#endif
static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

/*
 * Outputs a long in LSB order to the given stream
 */
static void _IcsPutLong (FILE* file, unsigned long int x)
{
   int ii;
   for (ii = 0; ii < 4; ii++) {
      fputc((int)(x & 0xff), file);
      x >>= 8;
   }
}

/*
 * Reads a long in LSB order from the given stream.
 */
static unsigned long int _IcsGetLong (FILE* file)
{
    unsigned long int x = (unsigned long int)getc (file);
    x += ((unsigned long int)getc (file))<<8;
    x += ((unsigned long int)getc (file))<<16;
    x += ((unsigned long int)getc (file))<<24;
    return x;
}

/*
 * Write ZIP compressed data.
 *
 * This function mostly does
 *    gzFile out;
 *    char mode[4]; strcpy(mode, "wb0"); mode[2] += level;
 *    out = gzdopen (dup(fileno(file)), mode);
 *    ICSTR( out == NULL, IcsErr_FWriteIds );
 *    if (gzwrite (out, (const voidp)inbuf, n) != (int)n)
 *       error = IcsErr_CompressionProblem;
 *    gzclose (out);
 */
Ics_Error IcsWriteZip (void const* inbuf, size_t len, FILE* file, int level)
{
#ifdef ICS_ZLIB
   z_stream stream;
   Byte* outbuf;    /* output buffer */
   int err, done;
   size_t count;

   /* Create an output buffer */
   outbuf = (Byte*)malloc (ICS_BUF_SIZE);
   ICSTR( outbuf == Z_NULL, IcsErr_Alloc );

   /* Initialize the stream for output */
   stream.zalloc = (alloc_func)0;
   stream.zfree = (free_func)0;
   stream.opaque = (voidpf)0;
   stream.next_in = (Bytef*)inbuf;
   stream.avail_in = len;
   stream.next_out = Z_NULL;
   stream.avail_out = 0;
   err = deflateInit2 (&stream, level, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
         /* windowBits is passed < 0 to suppress zlib header */
   if (err != Z_OK) {
      free (outbuf);
      return err == Z_VERSION_ERROR ? IcsErr_WrongZlibVersion : IcsErr_CompressionProblem;
   }

   /* Write a very simple GZIP header: */
   fprintf (file, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1], Z_DEFLATED,
            0,0,0,0,0,0, OS_CODE);

   /* Write the compressed data */
   stream.next_out = outbuf;
   stream.avail_out = ICS_BUF_SIZE;
   while (stream.avail_in != 0) {
      if (stream.avail_out == 0) {
         if (fwrite (outbuf, 1, ICS_BUF_SIZE, file) != ICS_BUF_SIZE) {
            deflateEnd (&stream);
            free (outbuf);
            return IcsErr_FWriteIds;
         }
         stream.next_out = outbuf;
         stream.avail_out = ICS_BUF_SIZE;
      }
      err = deflate (&stream, Z_NO_FLUSH);
      if (err != Z_OK) {
         break;
      }
   }
   /* Was all the input processed? */
   if (stream.avail_in != 0) {
      deflateEnd (&stream);
      free (outbuf);
      return IcsErr_CompressionProblem;
   }

   /* Flush the stream */
   done = 0;
   for (;;) {
      count = ICS_BUF_SIZE - stream.avail_out;
      if (count != 0) {
         if ((size_t)fwrite (outbuf, 1, count, file) != count) {
            deflateEnd (&stream);
            free (outbuf);
            return IcsErr_FWriteIds;
         }
         stream.next_out = outbuf;
         stream.avail_out = ICS_BUF_SIZE;
      }
      if (done) {
         break;
      }
      err = deflate (&stream, Z_FINISH);
      if ((err != Z_OK) && (err != Z_STREAM_END)) {
         deflateEnd (&stream);
         free (outbuf);
         return IcsErr_CompressionProblem;
      }
      done = (stream.avail_out != 0 || err == Z_STREAM_END);
   }
   /* Write the CRC and original data length */
   _IcsPutLong (file, crc32 (0L, (Bytef *)inbuf, len));
   _IcsPutLong (file, stream.total_in);
   /* Deallocate stuff */
   err = deflateEnd (&stream);
   free (outbuf);

   return (err == Z_OK) ? IcsErr_Ok : IcsErr_CompressionProblem;
#else
   return IcsErr_UnknownCompression;
#endif
}

/*
 * Write ZIP compressed data, with strides.
 */
Ics_Error IcsWriteZipWithStrides (void const* src,  size_t const* dim,
                                  size_t const* stride, int ndims, int nbytes,
                                  FILE* file, int level)
{
#ifdef ICS_ZLIB
   ICSINIT;
   z_stream stream;
   Byte* inbuf = 0;  /* input buffer */
   Byte* inbuf_ptr;
   Byte* outbuf = 0; /* output buffer */
   size_t curpos[ICS_MAXDIM];
   char const* data;
   int ii, err, done;
   size_t count;
   uLong crc;
   int const contiguous_line = stride[0]==1;

   /* Create an output buffer */
   outbuf = (Byte*)malloc (ICS_BUF_SIZE);
   ICSTR( outbuf == Z_NULL, IcsErr_Alloc );
   /* Create an input buffer */
   if (!contiguous_line) {
      inbuf = (Byte*)malloc (dim[0]*nbytes);
      if (inbuf == Z_NULL) {
         free (outbuf);
         return IcsErr_Alloc;
      }
   }

   /* Initialize the stream for output */
   stream.zalloc = (alloc_func)0;
   stream.zfree = (free_func)0;
   stream.opaque = (voidpf)0;
   stream.next_in = (Bytef*)0;
   stream.avail_in = 0;
   stream.next_out = Z_NULL;
   stream.avail_out = 0;
   err = deflateInit2 (&stream, level, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
         /* windowBits is passed < 0 to suppress zlib header */
   if (err != Z_OK) {
      free (outbuf);
      if (!contiguous_line) free (inbuf);
      return err == Z_VERSION_ERROR ? IcsErr_WrongZlibVersion : IcsErr_CompressionProblem;
   }
   stream.next_out = outbuf;
   stream.avail_out = ICS_BUF_SIZE;
   crc = crc32 (0L, Z_NULL, 0);

   /* Write a very simple GZIP header: */
   fprintf (file, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1], Z_DEFLATED,
            0,0,0,0,0,0, OS_CODE);

   /* Walk over each line in the 1st dimension */
   for (ii = 0; ii < ndims; ii++) {
      curpos[ii] = 0;
   }
   while (1) {
      data = (char const*)src;
      for (ii = 1; ii < ndims; ii++) { /* curpos[0]==0 here */
         data += curpos[ii]*stride[ii]*nbytes;
      }
      /* Get data line */
      if (contiguous_line) {
         inbuf = (Byte*)data;
      }
      else {
         inbuf_ptr = inbuf;
         for (ii = 0; ii < dim[0]; ii++) {
            memcpy (inbuf_ptr, data, nbytes);
            data += stride[0]*nbytes;
            inbuf_ptr += nbytes;
         }
      }
      /* Write the compressed data */
      stream.next_in = (Bytef*)inbuf;
      stream.avail_in = dim[0]*nbytes;
      while (stream.avail_in != 0) {
         if (stream.avail_out == 0) {
            if (fwrite (outbuf, 1, ICS_BUF_SIZE, file) != ICS_BUF_SIZE) {
               error = IcsErr_FWriteIds;
               goto error_exit;
            }
            stream.next_out = outbuf;
            stream.avail_out = ICS_BUF_SIZE;
         }
         err = deflate (&stream, Z_NO_FLUSH);
         if (err != Z_OK) {
            break;
         }
      }
      /* Was all the input processed? */
      if (stream.avail_in != 0) {
         error = IcsErr_CompressionProblem;
         goto error_exit;
      }
      crc = crc32 (crc, (Bytef*)inbuf, dim[0]*nbytes);
      /* This is part of the N-D loop */
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

   /* Flush the stream */
   done = 0;
   for (;;) {
      count = ICS_BUF_SIZE - stream.avail_out;
      if (count != 0) {
         if ((size_t)fwrite (outbuf, 1, count, file) != count) {
            error = IcsErr_FWriteIds;
            goto error_exit;
         }
         stream.next_out = outbuf;
         stream.avail_out = ICS_BUF_SIZE;
      }
      if (done) {
         break;
      }
      err = deflate (&stream, Z_FINISH);
      if ((err != Z_OK) && (err != Z_STREAM_END)) {
         error = IcsErr_CompressionProblem;
         goto error_exit;
      }
      done = (stream.avail_out != 0 || err == Z_STREAM_END);
   }
   /* Write the CRC and original data length */
   _IcsPutLong (file, crc);
   _IcsPutLong (file, stream.total_in);

error_exit:
   /* Deallocate stuff */
   err = deflateEnd (&stream);
   free (outbuf);
   if (!contiguous_line) free (inbuf);

   return error ? error : ((err == Z_OK) ? IcsErr_Ok : IcsErr_CompressionProblem);
#else
   return IcsErr_UnknownCompression;
#endif
}

/*
 * Start reading ZIP compressed data.
 *
 * This function mostly does
 *    br->ZlibStream = gzdopen (dup(fileno(br->DataFilePtr)), "rb");
 */
Ics_Error IcsOpenZip (Ics_Header* IcsStruct)
{
#ifdef ICS_ZLIB
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;
   FILE* file = br->DataFilePtr;
   z_stream* stream;
   void* inbuf;
   int err;
   int method, flags;  /* hold data from the GZIP header */

   /*printf("\nZLIB_VERSION = \"%s\" - zlibVersion() = \"%s\"\n\n", ZLIB_VERSION, zlibVersion());*/

   /* check the GZIP header */
   ICSTR( (getc(file) != gz_magic[0]) || (getc(file) != gz_magic[1]),
          IcsErr_CorruptedStream );
   method = getc(file);
   flags = getc(file);
   ICSTR( (method != Z_DEFLATED) || ((flags & RESERVED) != 0),
          IcsErr_CorruptedStream );
   fseek (file, 6, SEEK_CUR);         /* Discard time, xflags and OS code: */
   if ((flags & EXTRA_FIELD) != 0) {  /* skip the extra field */
      size_t len;
      len  =  (uInt)getc(file);
      len += ((uInt)getc(file))<<8;
      ICSTR( feof (file), IcsErr_CorruptedStream );
      fseek (file, len, SEEK_CUR);
   }
   if ((flags & ORIG_NAME) != 0) {   /* skip the original file name */
      int c;
      while (((c = getc(file)) != 0) && (c != EOF)) ;
   }
   if ((flags & COMMENT) != 0) {     /* skip the .gz file comment */
      int c;
      while (((c = getc(file)) != 0) && (c != EOF)) ;
   }
   if ((flags & HEAD_CRC) != 0) {    /* skip the header crc */
      fseek (file, 2, SEEK_CUR);
   }
   ICSTR( feof (file) || ferror (file), IcsErr_CorruptedStream );

   /* Create an input buffer */
   inbuf = malloc (ICS_BUF_SIZE);
   ICSTR( inbuf == NULL, IcsErr_Alloc );

   /* Initialize the stream for input */
   stream = (z_stream*)malloc (sizeof (z_stream));
   ICSTR( stream == NULL, IcsErr_Alloc );
   stream->zalloc = NULL;
   stream->zfree = NULL;
   stream->opaque = NULL;
   stream->next_in = NULL;
   stream->avail_in = 0;
   stream->next_out = NULL;
   stream->avail_out = 0;
   stream->next_in = (Byte*)inbuf;
   err = inflateInit2 (stream, -MAX_WBITS);
         /* windowBits is passed < 0 to tell that there is no zlib header.
          * Note that in this case inflate *requires* an extra "dummy" byte
          * after the compressed stream in order to complete decompression and
          * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
          * present after the compressed stream. */
   if (err != Z_OK) {
      if (err != Z_VERSION_ERROR) {
         inflateEnd (stream);
      }
      free (inbuf);
      return err == Z_VERSION_ERROR ? IcsErr_WrongZlibVersion : IcsErr_DecompressionProblem;
   }

   br->ZlibStream = stream;
   br->ZlibInputBuffer = inbuf;
   br->ZlibCRC = crc32 (0L, Z_NULL, 0);
   return IcsErr_Ok;
#else
   return IcsErr_UnknownCompression;
#endif
}

/*
 * Close ZIP compressed data stream.
 *
 * This function mostly does
 *    gzclose((gzFile)br->ZlibStream);
 */
Ics_Error IcsCloseZip (Ics_Header* IcsStruct)
{
#ifdef ICS_ZLIB
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;
   z_stream* stream = (z_stream*)br->ZlibStream;
   int err;

   err = inflateEnd (stream);
   free (stream);
   br->ZlibStream = NULL;
   free (br->ZlibInputBuffer);
   br->ZlibInputBuffer = NULL;

   if (err != Z_OK) {
      return IcsErr_DecompressionProblem;
   }
   return IcsErr_Ok;
#else
   return IcsErr_UnknownCompression;
#endif
}

/*
 * Read ZIP compressed data block.
 *
 * This function mostly does
 *    gzread ((gzFile)br->ZlibStream, outbuf, len);
 */
Ics_Error IcsReadZipBlock (Ics_Header* IcsStruct, void* outbuf, size_t len)
{
#ifdef ICS_ZLIB
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;
   FILE* file = br->DataFilePtr;
   z_stream* stream = (z_stream*)br->ZlibStream;
   void* inbuf = br->ZlibInputBuffer;
   int err = Z_STREAM_ERROR;
   size_t prevout = stream->total_out;

   /* Read the compressed data */
   stream->next_out = (Bytef*)outbuf;
   stream->avail_out = len;
   while (stream->avail_out != 0) {
      if (stream->avail_in == 0) {
         stream->next_in = (Byte*)inbuf;
         stream->avail_in = fread (inbuf, 1, ICS_BUF_SIZE, file);
         if (stream->avail_in == 0) {
            if (ferror (file)) {
               return IcsErr_FReadIds;
            } /* else eof! */
            break;
         }
      }
      err = inflate (stream, Z_NO_FLUSH);
      if (err == Z_STREAM_END) {
         /* All the data has been decompressed: Check CRC and original data size */
         br->ZlibCRC = crc32 (br->ZlibCRC, (Bytef*)outbuf, len);
         /* Set the file pointer back so that _IcsGetLong can read the
            numbers just behind the compressed data */
         fseek (file, -(int)stream->avail_in, SEEK_CUR);
         if (_IcsGetLong (file) != br->ZlibCRC) {
            err = Z_STREAM_ERROR;
         }
         else {
            if (_IcsGetLong(file) != stream->total_out) {
               err = Z_STREAM_ERROR;
            }
         }
      }
      if (err != Z_OK) {
         break;
      }
   }
   /* Update CRC */
   if (err == Z_OK) {
      br->ZlibCRC = crc32 (br->ZlibCRC, (Bytef*)outbuf, len);
   }

   /* Report errors */
   ICSTR( err == Z_STREAM_ERROR, IcsErr_CorruptedStream );
   if (err == Z_STREAM_END) {
      ICSTR( len != stream->total_out - prevout, IcsErr_EndOfStream );
      return IcsErr_Ok;
   }
   ICSTR( err == Z_OK, IcsErr_Ok );
   return IcsErr_DecompressionProblem;
#else
   return IcsErr_UnknownCompression;
#endif
}

/*
 * Skip ZIP compressed data block.
 *
 * This function mostly does
 *    gzseek ((gzFile)br->ZlibStream, (z_off_t)offset, whence);
 */
Ics_Error IcsSetZipBlock (Ics_Header* IcsStruct, long offset, int whence)
{
#ifdef ICS_ZLIB
   ICSINIT;
   size_t n, bufsize;
   void* buf;
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;
   z_stream* stream = (z_stream*)br->ZlibStream;

   if ((whence == SEEK_CUR) && (offset<0)) {
      offset += stream->total_out;
      whence = SEEK_SET;
   }
   if (whence == SEEK_SET) {
      ICSTR( offset < 0, IcsErr_IllParameter );
      /* Rewind the file -- maybe the convoluted way is quicker, but who cares? */
      ICSXR( IcsCloseIds (IcsStruct) );
      ICSXR( IcsOpenIds (IcsStruct) );
      ICSTR( offset==0, IcsErr_Ok );
   }

   bufsize = offset < ICS_BUF_SIZE ? offset : ICS_BUF_SIZE;
   buf = malloc (bufsize);
   ICSTR( buf == NULL, IcsErr_Alloc );

   n = offset;
   while (n > 0) {
      if (n > bufsize) {
         error = IcsReadZipBlock (IcsStruct, buf, bufsize);
         n -= bufsize;
      }
      else {
         error = IcsReadZipBlock (IcsStruct, buf, n);
         break;
      }
      if (error) {
         break;
      }
   }

   free (buf);

   return error;
#else
   return IcsErr_UnknownCompression;
#endif
}

