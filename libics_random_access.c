/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright 2015-2018:
 *   Scientific Volume Imaging Holding B.V.
 *   Laapersveld 63, 1213 VB Hilversum, The Netherlands
 *   https://www.svi.nl
 *
 * Copyright (C) 2018 Michael van Ginkel
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
 * Library General Public License for more details.`
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * FILE : libics_random_access.c
 *
 * The following library functions are contained in this file:
 *
 *   Ics_RaOpen()
 *   Ics_RaClose()
 *   Ics_RaReadOrWrite()
 *   Ics_RaCreate()
 */


#include <stdlib.h>
#include <string.h>
#include "libics_random_access.h"
#include "libics_intern.h"

#define ICSXJ( call ) if (( error = call ) != IcsErr_Ok ) goto ics_error;

typedef struct {
  /* FILE specs */
  FILE          *fp;
  size_t         fend;
  /* buffer specs */
  unsigned char *buffer;
  size_t         length;    /* in bytes */
  size_t         position;  /* position of buffer[0] in file */
  /* state */
  Ics_FileMode   mode;
  size_t         ndata;     /* amount of valid bytes are in the buffer */
} ReadwriteBuffer;


static void setByteOrderMap(ICSRA *icsra, int *isNop)
{
    ICS *ics;
    int nbytes, ii, machOrder[ICS_MAX_IMEL_SIZE], map[ICS_MAX_IMEL_SIZE];

    ics = &(icsra->ics);
    nbytes = IcsGetBytesPerSample( ics );
    IcsFillByteOrder(ics->imel.dataType, nbytes, machOrder);

    for ( ii=0; ii<nbytes; ii++ ) {
        map[ics->byteOrder[ii]-1] = ii;
    }
    *isNop = 1;
    for ( ii=0; ii<nbytes; ii++ ) {
        icsra->bomap[ii] = map[machOrder[ii]-1];
        *isNop = *isNop && (icsra->bomap[ii] == ii);
    }
}

/* Create an ICSRA structure. Read the ICS header and keep the file
 * descriptor open for random access */
Ics_Error Ics_RaOpen(ICSRA      **p_icsra,
                     const char  *filename,
                     const char  *mode)
{
    ICSINIT;
    ICSRA       *icsra;
    int          forceName = 0, forceLocale = 1, isNop;
    size_t       ii;
    Ics_OpenMode openMode = IcsOpenMode_raReadWrite;

    ii = 0;
    if ( strlen(mode)>=2 && mode[0] == 'r' ) {
      if ( mode[1] == 'o' ) {
        openMode = IcsOpenMode_raRead;
        ii = 2;
      }
      if ( mode[1] == 'w' ) {
        ii = 2;
      }
    }
    /* the remaining mode string can containing "f" and/or "l" */
    for ( ; ii<strlen(mode); ii++) {
        switch (mode[ii]) {
            case 'f':
                if (forceName) return IcsErr_IllParameter;
                forceName = 1;
                break;
            case 'l':
                if (forceLocale) return IcsErr_IllParameter;
                forceLocale = 0;
                break;
            default:
                return IcsErr_IllParameter;
        }
    }
    *p_icsra =(ICSRA*)malloc(sizeof(ICSRA));
    if (*p_icsra == NULL) return IcsErr_Alloc;
    icsra = *p_icsra;
    icsra->readOnly = (openMode == IcsOpenMode_raRead ) ? 1 : 0;
    /* also tests for version>1 and single .ics containing header+data */
    error = IcsReadIcsLow(&(icsra->ics), filename, forceName, forceLocale,
                          openMode, &(icsra->fp));
    if (!error && icsra->ics.compression != IcsCompr_uncompressed) {
        error = IcsErr_RaOnlyUncompressedDataSupported;
    }
    setByteOrderMap(icsra, &isNop);

    if (error) {
        free(*p_icsra);
        *p_icsra = NULL;
    }

    return error;
}

Ics_Error Ics_RaClose(ICSRA *icsra)
{
    ICSINIT;
    if ( icsra == NULL ) return error;
    if ( icsra->fp != NULL && fclose(icsra->fp) == EOF ) {
        error = IcsErr_FCloseIcs;
    }
    free(icsra);
    return error;
}

static Ics_Error initialiseBuffer(ReadwriteBuffer   *buffer,
                                  const int          dm,
                                  const size_t      *dims,
                                  const int          elsize,
                                  FILE              *fp,
                                  const size_t       fileOffset,
                                  const Ics_FileMode mode)
{
    size_t maxBufferSize = ICS_MAX_IMEL_SIZE*(4*1024*1024/ICS_MAX_IMEL_SIZE);
    size_t bufferSize;
    int ii;

    if ( (mode != IcsFileMode_write) && (mode != IcsFileMode_read) ) {
        return IcsErr_IllParameter;
    }

    bufferSize = dims[0] * elsize;
    for ( ii=1; ii<dm; ii++ ) bufferSize *= dims[ii];
    buffer->fp = fp;
    buffer->fend = fileOffset + bufferSize;
    bufferSize = (bufferSize < maxBufferSize) ? bufferSize : maxBufferSize;
    buffer->buffer = (unsigned char *)malloc(bufferSize);
    if ( !buffer->buffer ) return IcsErr_Alloc;
    buffer->length = bufferSize;
    buffer->position = fileOffset;
    buffer->ndata = 0;
    buffer->mode = mode;
    return IcsErr_Ok;
}

static Ics_Error writeBuffer(ReadwriteBuffer *buffer)
{
    size_t written;

    if ( buffer->ndata == 0 ) return IcsErr_Ok;

    if (fseek(buffer->fp,buffer->position,SEEK_SET)) {
        return IcsErr_FWriteIds;
    }
    written = fwrite( buffer->buffer,1,buffer->ndata,buffer->fp);
    if ( written != buffer->ndata ) {
        return IcsErr_FWriteIds;
    }
    return IcsErr_Ok;
}

static Ics_Error updateBuffer(ReadwriteBuffer *buffer,
                              size_t *bof,
                              size_t *boe)
{
    ICSINIT;
    size_t bytesToRead;
    size_t read;

    /* write buffer to disk if needed */
    if ( buffer->mode == IcsFileMode_write ) {
        error = writeBuffer(buffer);
        if ( error != IcsErr_Ok ) return error;
    }

    buffer->position += *bof;
    if (fseek(buffer->fp,buffer->position,SEEK_SET)) {
        return IcsErr_FReadIds;
    }
    bytesToRead = buffer->fend - buffer->position;
    if ( bytesToRead > buffer->length ) bytesToRead = buffer->length;
    read = fread( buffer->buffer,1,bytesToRead,buffer->fp);
    if ( read != bytesToRead) {
        return IcsErr_FReadIds;
    }
    *bof = 0;
    *boe = bytesToRead;
    buffer->ndata = bytesToRead;

    return IcsErr_Ok;
}


static void reorder(const int     dm,
                    ptrdiff_t    *array,
                    const size_t *order)
{
    size_t sorted[ICS_MAXDIM];
    int ii;
    for ( ii=0; ii<dm; ii++ ) {
        sorted[ii] = array[order[ii]];
    }
    for ( ii=0; ii<dm; ii++ ) {
        array[ii] = sorted[ii];
    }
}

static void optimiseProcessingOrder(const int  dm,
                                    size_t    *dims,
                                    ptrdiff_t *f_stride,
                                    ptrdiff_t *i_stride,
                                    size_t    *f_offset,
                                    size_t    *i_offset)
{
    size_t order[ICS_MAXDIM];
    int ii, jj, in_order = 1;

    /* first ensure all f_stride>0 */
    for ( ii=0; ii<dm; ii++ ) {
        if ( f_stride[ii]>=0 ) continue;
        f_stride[ii] = -f_stride[ii];
        i_stride[ii] = -i_stride[ii];
        *f_offset -= f_stride[ii]*(ptrdiff_t)(dims[ii]-1);
        *i_offset -= i_stride[ii]*(ptrdiff_t)(dims[ii]-1);
        if ( ii>0 && f_stride[ii]<f_stride[ii-1] ) in_order = 0;
    }
    if ( in_order ) return;
    /* find the order in which the strides are sorted */
    order[0] = 0;
    for ( ii=1; ii<dm; ii++ ) {
        order[ii] = ii;
        for ( jj=ii-1; jj>=0; jj-- ) {
            if ( f_stride[order[jj]] < f_stride[ii] )
                break;
            order[jj+1] = order[jj];
        }
        order[jj+1] = ii;
    }
    /* re-order the file strides from small to large */
    reorder( dm, f_stride, order );
    reorder( dm, i_stride, order );
    reorder( dm, (ptrdiff_t *)dims, order );
}

static void computeStrides(const int     dm,
                           const size_t *dims,
                           size_t       *stride)
{
    int ii;

    stride[0] = 1;
    for ( ii=1; ii<dm; ii++ ) {
      stride[ii] = stride[ii-1]*dims[ii-1];
    }
}

static size_t computeOffset(const int     dm,
                            const size_t *origin,
                            const size_t *stride)
{
    int ii;
    size_t offset = 0;

    if ( origin == NULL ) return 0;
    for ( ii=0; ii<dm; ii++ ) {
      offset += origin[ii]*stride[ii];
    }
    return offset;
}

static void computeFinalStrides(const int     dm,
                                const int     elsize,
                                const size_t *stride,
                                const size_t *interval,
                                ptrdiff_t    *step)
{
    int ii;
    ptrdiff_t d_step;

    for ( ii=0; ii<dm; ii++ ) {
        d_step = (ptrdiff_t) stride[ii] * elsize;
        step[ii] = interval ? d_step * interval[ii] : d_step;
    }
}


/* the integer arithmetic below can overflow and is not bullet-proof */
static Ics_Error checkLimits(const int        dm, 
                             const size_t    *dims,
                             const size_t    *fl_origin,
                             const size_t    *fl_dims,
                             const ptrdiff_t *fl_interval )
{
    int ii;
    ptrdiff_t limL, limH;

    for ( ii=0; ii<dm; ii++ ) {
        limL = fl_origin ? fl_origin[ii] : 0;
        limH = limL + ( fl_interval ? fl_interval[ii] : 1 ) *
                      (ptrdiff_t)( fl_dims ? fl_dims[ii]-1 : dims[ii]-1 );
        if ((size_t)limL >= dims[ii]) return IcsErr_IllegalROI;
        if (limH<0)                   return IcsErr_IllegalROI;
        if ((size_t)limH >= dims[ii]) return IcsErr_IllegalROI;
    }
    return IcsErr_Ok;
}


/* notes:
 * - dm is only passed as an extra verification step. It should have been
 *   obtained through Ics_GetLayout() by the caller. All fl_ and im_ parameters
 *   can be NULL.
 * - im_dims  is NOT a required parameter. If given, it is used to
 *   check im_origin and im_interval (these are otherwise not checked)
 */
Ics_Error Ics_RaReadOrWrite(const ICSRA        *icsra,
                            const int           pdm,
                            const size_t       *rg_dims,
                            const size_t       *fl_origin,
                            const ptrdiff_t    *fl_interval,
                            const void         *image,
                            const size_t       *im_dims,
                            const size_t       *im_stride,
                            const size_t       *im_origin,
                            const ptrdiff_t    *im_interval,
                            const Ics_FileMode  mode)
{
    ICSINIT;
    const ICS *ics;
    Ics_DataType dt;
    int dm, ii, jj, ci;
    size_t elsize, fl_dims[ICS_MAXDIM], r_dims[ICS_MAXDIM], fl_stride[ICS_MAXDIM];
    size_t f_offset, i_offset, cors[ICS_MAXDIM], boe;
    ptrdiff_t f_stride[ICS_MAXDIM], i_stride[ICS_MAXDIM];
    ReadwriteBuffer buffer;
    unsigned char *ip = NULL, *line;

    buffer.buffer = NULL;   /* BEFORE ANYTHING ELSE */

    if (icsra == NULL) return IcsErr_NotValidAction;
    if ( icsra->readOnly>0 && mode == IcsFileMode_write ) return IcsErr_FReadOnly;
    if (im_stride == NULL) return IcsErr_IllParameter;
    ics = &(icsra->ics);
    ICSXJ( IcsGetLayout(ics, &dt, &dm, fl_dims) );
    if ( dm != pdm ) {
      error = IcsErr_DimensionalityMismatch;
      goto ics_error;
    }
    ICSXJ( checkLimits( dm, fl_dims, fl_origin, rg_dims, fl_interval ));
    if ( im_dims != NULL ) {
        ICSXJ( checkLimits( dm, im_dims, im_origin, rg_dims, im_interval ));
    }
    elsize = IcsGetDataTypeSize( dt );
    ICSXJ( initialiseBuffer( &buffer, dm, fl_dims, elsize,
                             icsra->fp, ics->srcOffset, mode ));

    computeStrides( dm, fl_dims, fl_stride );
    f_offset = computeOffset( dm, fl_origin, fl_stride );
    i_offset = computeOffset( dm, im_origin, im_stride );
    /* switch to byte offsets and final strides (in bytes) */
    f_offset *= elsize;
    i_offset *= elsize;
    computeFinalStrides( dm, elsize, fl_stride, fl_interval, f_stride );
    computeFinalStrides( dm, elsize, im_stride, im_interval, i_stride );

    for ( ii=0; ii<dm; ii++ ) {
      r_dims[ii] = rg_dims[ii];
      cors[ii] = 0;
    }
    optimiseProcessingOrder(dm,r_dims,f_stride,i_stride,&f_offset,&i_offset);

    ip = (unsigned char *) image;
    ip += i_offset;

    boe = 0;
    ci = 0;
    line = buffer.buffer;
    while(ci!=dm) {
        for ( ii=0; ii<r_dims[0]; ii++ ) {
            if ( f_offset >= boe ) {
                ICSXJ(updateBuffer( &buffer, &f_offset, &boe));
            }
            if ( mode == IcsFileMode_write ) {
                for ( jj=0; jj<elsize; jj++ ) {
                    line[f_offset+icsra->bomap[jj]] = ip[jj];
                }
            } else {
                for ( jj=0; jj<elsize; jj++ ) {
                    ip[jj] = line[f_offset+icsra->bomap[jj]];
                }
            }
            f_offset += f_stride[0];
            ip += i_stride[0];
        }
        f_offset -= f_stride[0]*r_dims[0];
        ip -= i_stride[0]*r_dims[0];
        for ( ci=1; ci<dm; ci++ ) {
            f_offset += f_stride[ci];
            ip += i_stride[ci];
            cors[ci]++;
            if ( cors[ci] != r_dims[ci] ) break;
            f_offset -= f_stride[ci]*r_dims[ci];
            ip -= i_stride[ci]*r_dims[ci];
            cors[ci]=0;
        }
    }
    if ( mode == IcsFileMode_write ) {
        ICSXJ( writeBuffer(&buffer));
    }

ics_error:
    free(buffer.buffer);
    return error;
}

Ics_Error Ics_RaCreate(const char         *filename,
                       const char         *mode,
                       const Ics_DataType  dt,
                       const int           nbits,
                       const int           dm,
                       const size_t       *dims)
{
  ICSINIT;
  unsigned char dummyByte = 0;
  ICS ics;
  size_t size;
  FILE *fp = NULL;
  int forceName = 0, forceLocale = 1;
  int ii;

  /* the mode string is "f" and/or "l" */
  for (ii = 0; ii<strlen(mode); ii++) {
      switch (mode[ii]) {
          case 'f':
              if (forceName) return IcsErr_IllParameter;
              forceName = 1;
              break;
          case 'l':
              if (forceLocale) return IcsErr_IllParameter;
              forceLocale = 0;
              break;
          default:
              return IcsErr_IllParameter;
      }
  }
  IcsInit(&ics);
  ics.fileMode = IcsFileMode_write;
  ics.version = 2;
  ICSXJ( IcsSetLayout( &ics, dt, dm, dims ));
  ICSXJ( IcsSetSignificantBits ( &ics,  nbits));
  ICSXJ( IcsSetCompression ( &ics, IcsCompr_uncompressed, 0 ));
  ICSXJ( IcsWriteIcsLow( &ics, filename, &fp ));
  size = IcsGetDataTypeSize( dt );
  for ( ii=0; ii<dm; ii++ ) size *= dims[ii];

  if (fseek( fp, size-1, SEEK_CUR )!=0 && !error) {
     error = IcsErr_FWriteIds;
     goto ics_error;
  }
  if (fwrite( &dummyByte, 1, 1, fp )!=1 && !error) {
     error = IcsErr_FWriteIds;
  }

ics_error:
  if (fp) {
    fclose(fp);
  }
  return error;
}
