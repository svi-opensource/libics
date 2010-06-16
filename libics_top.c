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
 * FILE : libics_top.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsOpen()
 *   IcsClose()
 *   IcsGetLayout()
 *   IcsSetLayout()
 *   IcsGetDataSize()
 *   IcsGetImelSize()
 *   IcsGetImageSize()
 *   IcsGetData()
 *   IcsGetDataBlock ()
 *   IcsSkipDataBlock ()
 *   IcsGetROIData ()
 *   IcsGetDataWithStrides()
 *   IcsSetData()
 *   IcsSetDataWithStrides()
 *   IcsSetSource ()
 *   IcsSetCompression ()
 *   IcsGetPosition()
 *   IcsSetPosition()
 *   IcsGetOrder()
 *   IcsSetOrder()
 *   IcsGetCoordinateSystem()
 *   IcsSetCoordinateSystem()
 *   IcsGetSignificantBits()
 *   IcsSetSignificantBits()
 *   IcsGetImelUnits()
 *   IcsSetImelUnits()
 *   IcsGetScilType()
 *   IcsSetScilType()
 *   IcsGuessScilType()
 *   IcsGetErrorText()
 */

#include <stdlib.h>
#include <string.h>
#include "libics_intern.h"

/*
 * Default Order and Label strings:
 */
char const* ICSKEY_ORDER[] = {"x", "y", "z", "t", "probe"};
char const* ICSKEY_LABEL[] = {"x-position", "y-position", "z-position",
                              "time", "probe"};

/*
 * Create an ICS structure, and read the stuff from file if reading.
 */
Ics_Error IcsOpen (ICS* *ics, char const* filename, char const* mode)
{
   ICSINIT;
   int ii, version = 0, forcename = 0, forcelocale = 1, reading = 0, writing = 0;

   /* the mode string is one of: "r", "w", "rw", with "f" and/or "l" appended for
      reading and "1" or "2" appended for writing */
   for (ii = 0; ii<strlen(mode); ii++) {
      switch (mode[ii]) {
         case 'r':
            ICSTR( reading, IcsErr_IllParameter );
            reading = 1;
            break;
         case 'w':
            ICSTR( writing, IcsErr_IllParameter );
            writing = 1;
            break;
         case 'f':
            ICSTR( forcename, IcsErr_IllParameter );
            forcename = 1;
            break;
         case 'l':
            ICSTR( forcelocale, IcsErr_IllParameter );
            forcelocale = 0;
            break;
         case '1':
            ICSTR( version!=0, IcsErr_IllParameter );
            version = 1;
            break;
         case '2':
            ICSTR( version!=0, IcsErr_IllParameter );
            version = 2;
            break;
         default:
            return IcsErr_IllParameter;
      }
   }
   *ics = (ICS*)malloc (sizeof (ICS));
   ICSTR( *ics == NULL, IcsErr_Alloc );
   if (reading) {
      /* We're reading or updating */
      error = IcsReadIcs (*ics, filename, forcename, forcelocale);
      if (error) {
         free (*ics);
         *ics = NULL;
      }
      else {
         if (writing) {
            /* We're updating */
            (*ics)->FileMode = IcsFileMode_update;
         }
         else {
            /* We're just reading */
            (*ics)->FileMode = IcsFileMode_read;
         }
      }
   }
   else if (writing) {
      /* We're writing */
      IcsInit (*ics);
      (*ics)->FileMode = IcsFileMode_write;
      (*ics)->Version = version;
      IcsStrCpy ((*ics)->Filename, filename, ICS_MAXPATHLEN);
   }
   else {
      /* Missing an "r" or "w" mode character */
      return IcsErr_IllParameter;
   }

   return error;
}

/*
 * Free the ICS structure, and write the stuff to file if writing.
 */
Ics_Error IcsClose (ICS* ics)
{
   ICSINIT;
   char filename[ICS_MAXPATHLEN+4];

   ICSTR( ics == NULL, IcsErr_NotValidAction );
   if (ics->FileMode == IcsFileMode_read) {
      /* We're reading */
      if (ics->BlockRead != NULL) {
         error = IcsCloseIds (ics);
      }
   }
   else if (ics->FileMode == IcsFileMode_write) {
      /* We're writing */
      error = IcsWriteIcs (ics, NULL);
      ICSCX( IcsWriteIds (ics) );
   }
   else {
      /* We're updating */
      int needcopy = 0;
      if (ics->BlockRead != NULL) {
         error = IcsCloseIds (ics);
      }
      if (ics->Version == 2 && !strcmp(ics->SrcFile, ics->Filename)) {
         /* The ICS file contains the data */
         needcopy = 1;
         ics->SrcFile[0] = '\0'; /* needed to get the END keyword in the header */
         /* Rename the original file */
         strcpy (filename, ics->Filename);
         strcat (filename, ".tmp");
         if (rename (ics->Filename, filename)) {
            error = IcsErr_FTempMoveIcs;
         }
      }
      ICSCX( IcsWriteIcs (ics, NULL) );
      if (!error && needcopy) {
         /* Copy the data over from the original file */
         error = IcsCopyIds (filename, ics->SrcOffset, ics->Filename);
         /* Delete original file */
         if (!error) {
            remove (filename);
         }
      }
      if (error) {
         /* Let's try copying the old file back */
         remove (ics->Filename);
         rename (filename, ics->Filename);
      }
   }
   IcsFreeHistory (ics);
   free (ics);

   return error;
}

/*
 * Get the layout parameters from the ICS structure.
 */
Ics_Error IcsGetLayout (ICS const* ics, Ics_DataType* dt, int* ndims, size_t* dims)
{
   ICSINIT;
   int ii;

   ICS_FM_RD( ics );
   *ndims = ics->Dimensions;
   *dt = ics->Imel.DataType;
   /* Get the image sizes. Ignore the orders */
   for (ii = 0; ii < *ndims; ii++) {
      dims[ii] = ics->Dim[ii].Size;
   }

   return error;
}

/*
 * Put the layout parameters in the ICS structure.
 */
Ics_Error IcsSetLayout (ICS* ics, Ics_DataType dt, int ndims, size_t const* dims)
{
   ICSINIT;
   int ii;

   ICS_FM_WD( ics );
   ICSTR( ndims > ICS_MAXDIM, IcsErr_TooManyDims );
   /* Set the pixel parameters */
   ics->Imel.DataType = dt;
   /* Set the image sizes */
   for (ii=0; ii<ndims; ii++) {
      ics->Dim[ii].Size = dims[ii];
      strcpy (ics->Dim[ii].Order, ICSKEY_ORDER[ii]);
      strcpy (ics->Dim[ii].Label, ICSKEY_LABEL[ii]);
   }
   ics->Dimensions = ndims;

   return error;
}

/*
 * Get the image size in bytes.
 */
size_t IcsGetDataSize (ICS const* ics)
{
   ICSTR( ics == NULL, 0 );
   ICSTR( ics->Dimensions == 0, 0 );
   return IcsGetImageSize (ics) * IcsGetBytesPerSample (ics);
}

/*
 * Get the pixel size in bytes.
 */
size_t IcsGetImelSize (ICS const* ics)
{
   if (ics != NULL) {
      return IcsGetBytesPerSample (ics);
   }
   else {
      return 0;
   }
}

/*
 * Get the image size in pixels.
 */
size_t IcsGetImageSize (ICS const* ics)
{
   int ii;
   size_t size = 1;

   ICSTR( ics == NULL, 0 );
   ICSTR( ics->Dimensions == 0, 0 );
   for (ii = 0; ii < ics->Dimensions; ii++) {
      size *= ics->Dim[ii].Size;
   }

   return size;
}

/*
 * Get the image data. It is read from the file right here.
 */
Ics_Error IcsGetData (ICS* ics, void* dest, size_t n)
{
   ICSINIT;

   ICS_FM_RD( ics );
   if ((n != 0) && (dest != NULL)) {
      error = IcsReadIds (ics, dest, n);
   }

   return error;
}

/*
 * Read a portion of the image data from an ICS file.
 */
Ics_Error IcsGetDataBlock (ICS* ics, void* dest, size_t n)
{
   ICSINIT;

   ICS_FM_RD( ics );
   if ((n != 0) && (dest != NULL)) {
      if (ics->BlockRead == NULL) {
         error = IcsOpenIds (ics);
      }
      ICSCX( IcsReadIdsBlock (ics, dest, n) );
   }

   return error;
}

/*
 * Skip a portion of the image from an ICS file.
 */
Ics_Error IcsSkipDataBlock (ICS* ics, size_t n)
{
   ICSINIT;

   ICS_FM_RD( ics );
   if (n != 0) {
      if (ics->BlockRead == NULL) {
         error = IcsOpenIds (ics);
      }
      ICSCX( IcsSkipIdsBlock (ics, n) );
   }

   return error;
}

/*
 * Read a square region of the image from an ICS file.
 */
Ics_Error IcsGetROIData (ICS* ics, size_t const* p_offset, size_t const* p_size,
                         size_t const* p_sampling, void* p_dest, size_t n)
{
   ICSDECL;
   int ii, sizeconflict = 0, p;
   size_t imelsize, roisize, cur_loc, new_loc, bufsize;
   size_t curpos[ICS_MAXDIM];
   size_t stride[ICS_MAXDIM];
   size_t b_offset[ICS_MAXDIM];
   size_t b_size[ICS_MAXDIM];
   size_t b_sampling[ICS_MAXDIM];
   size_t const *offset, *size, *sampling;
   char* buf;
   char* dest = (char*)p_dest;

   ICS_FM_RD( ics );
   ICSTR( (n == 0) || (dest == NULL), IcsErr_Ok );
   p = ics->Dimensions;
   if (p_offset != NULL) {
      offset = p_offset;
   }
   else {
      for (ii = 0; ii < p; ii++) {
         b_offset[ii] = 0;
      }
      offset = b_offset;
   }
   if (p_size != NULL) {
      size = p_size;
   }
   else {
      for (ii = 0; ii < p; ii++) {
         b_size[ii] = ics->Dim[ii].Size - offset[ii];
      }
      size = b_size;
   }
   if (p_sampling != NULL) {
      sampling = p_sampling;
   }
   else {
      for (ii = 0; ii < p; ii++) {
         b_sampling[ii] = 1;
      }
      sampling = b_sampling;
   }
   for (ii = 0; ii < p; ii++) {
      if (sampling[ii] < 1 || offset[ii]+size[ii] > ics->Dim[ii].Size)
         return IcsErr_IllegalROI;
   }
   imelsize = IcsGetBytesPerSample (ics);
   roisize = imelsize;
   for (ii = 0; ii < p; ii++) {
      roisize *= (size[ii]+sampling[ii]-1) / sampling[ii]; /* my own ceil() */
   }
   if (n != roisize) {
      sizeconflict = 1;
      ICSTR( n < roisize, IcsErr_BufferTooSmall );
   }
   /* The stride array tells us how many imels to skip to go the next pixel in
      each dimension */
   stride[0] = 1;
   for (ii = 1; ii < p; ii++) {
      stride[ii] = stride[ii-1]*ics->Dim[ii-1].Size;
   }
   ICSXR( IcsOpenIds (ics) );
   bufsize = imelsize*size[0];
   if (sampling[0] > 1) {
      /* We read a line in a buffer, and then copy the needed imels to dest */
      buf = (char*)malloc (bufsize);
      ICSTR( buf == NULL, IcsErr_Alloc );
      cur_loc = 0;
      for (ii = 0; ii < p; ii++) {
         curpos[ii] = offset[ii];
      }
      while (1) {
         new_loc = 0;
         for (ii = 0; ii < p; ii++) {
            new_loc += curpos[ii]*stride[ii];
         }
         new_loc *= imelsize;
         if (cur_loc < new_loc) {
            error = IcsSkipIdsBlock (ics, new_loc - cur_loc);
            cur_loc = new_loc;
         }
         ICSCX( IcsReadIdsBlock (ics, buf, bufsize) );
         if (error != IcsErr_Ok) {
            break; /* stop reading on error */
         }
         cur_loc += bufsize;
         for (ii=0; ii<size[0]; ii+=sampling[0]) {
            memcpy (dest, buf+ii*imelsize, imelsize);
            dest += imelsize;
         }
         for (ii = 1; ii < p; ii++) {
            curpos[ii] += sampling[ii];
            if (curpos[ii] < offset[ii]+size[ii]) {
               break;
            }
            curpos[ii] = offset[ii];
         }
         if (ii==p) {
            break; /* we're done reading */
         }
      }
      free (buf);
   }
   else {
      /* No subsampling in dim[0] required: read directly into dest */
      cur_loc = 0;
      for (ii = 0; ii < p; ii++) {
         curpos[ii] = offset[ii];
      }
      while (1) {
         new_loc = 0;
         for (ii = 0; ii < p; ii++) {
            new_loc += curpos[ii]*stride[ii];
         }
         new_loc *= imelsize;
         if (cur_loc < new_loc) {
            error = IcsSkipIdsBlock (ics, new_loc - cur_loc);
            cur_loc = new_loc;
         }
         ICSCX( IcsReadIdsBlock (ics, dest, bufsize) );
         if (error != IcsErr_Ok) {
            break; /* stop reading on error */
         }
         cur_loc += bufsize;
         dest += bufsize;
         for (ii = 1; ii < p; ii++) {
            curpos[ii] += sampling[ii];
            if (curpos[ii] < offset[ii]+size[ii]) {
               break;
            }
            curpos[ii] = offset[ii];
         }
         if (ii==p) {
            break; /* we're done reading */
         }
      }
   }
   ICSXA( IcsCloseIds (ics) );

   if ((error == IcsErr_Ok) && sizeconflict) {
      error = IcsErr_OutputNotFilled;
   }
   return error;
}

/*
 * Read the image data into a region of your buffer.
 */
Ics_Error IcsGetDataWithStrides (ICS* ics, void* p_dest, size_t n, size_t const* p_stride, int ndims)
{
   ICSDECL;
   int ii, p;
   size_t imelsize, lastpixel, bufsize;
   size_t curpos[ICS_MAXDIM];
   size_t b_stride[ICS_MAXDIM];
   size_t const *stride;
   char* buf;
   char* dest = (char*)p_dest;
   char* out;

   ICS_FM_RD( ics );
   ICSTR( (n == 0) || (dest == NULL), IcsErr_Ok );
   p = ics->Dimensions;
   ICSTR( ndims != p, IcsErr_IllParameter );
   if (p_stride != NULL) {
      stride = p_stride;
   }
   else {
      b_stride[0] = 1;
      for (ii = 1; ii < p; ii++) {
         b_stride[ii] = b_stride[ii-1]*ics->Dim[ii-1].Size;
      }
      stride = b_stride;
   }
   imelsize = IcsGetBytesPerSample (ics);
   lastpixel = 0;
   for (ii = 0; ii < p; ii++) {
      lastpixel += (ics->Dim[ii].Size-1) * stride[ii];
   }
   ICSTR( lastpixel*imelsize > n, IcsErr_IllParameter );

   ICSXR( IcsOpenIds (ics) );
   bufsize = imelsize*ics->Dim[0].Size;
   if (stride[0] > 1) {
      /* We read a line in a buffer, and then copy the imels to dest */
      buf = (char*)malloc (bufsize);
      ICSTR( buf == NULL, IcsErr_Alloc );
      for (ii = 0; ii < p; ii++) {
         curpos[ii] = 0;
      }
      while (1) {
         out = dest;
         for (ii = 1; ii < p; ii++) {
            out += curpos[ii]*stride[ii]*imelsize;
         }
         ICSCX( IcsReadIdsBlock (ics, buf, bufsize) );
         if (error != IcsErr_Ok) {
            break; /* stop reading on error */
         }
         for (ii = 0; ii < ics->Dim[0].Size; ii++) {
            memcpy (out, buf+ii*imelsize, imelsize);
            out += stride[0]*imelsize;
         }
         for (ii = 1; ii < p; ii++) {
            curpos[ii]++;
            if (curpos[ii] < ics->Dim[ii].Size) {
               break;
            }
            curpos[ii] = 0;
         }
         if (ii==p) {
            break; /* we're done reading */
         }
      }
      free (buf);
   }
   else {
      /* No subsampling in dim[0] required: read directly into dest */
      for (ii = 0; ii < p; ii++) {
         curpos[ii] = 0;
      }
      while (1) {
         out = dest;
         for (ii = 1; ii < p; ii++) {
            out += curpos[ii]*stride[ii]*imelsize;
         }
         ICSCX( IcsReadIdsBlock (ics, out, bufsize) );
         if (error != IcsErr_Ok) {
            break; /* stop reading on error */
         }
         for (ii = 1; ii < p; ii++) {
            curpos[ii]++;
            if (curpos[ii] < ics->Dim[ii].Size) {
               break;
            }
            curpos[ii] = 0;
         }
         if (ii==p) {
            break; /* we're done reading */
         }
      }
   }
   ICSXA( IcsCloseIds (ics) );

   return error;
}

/*
 * Set the image data. The pointer must be valid until IcsClose() is called.
 */
Ics_Error IcsSetData (ICS* ics, void const* src, size_t n)
{
   ICSINIT;

   ICS_FM_WD( ics );
   ICSTR( ics->SrcFile[0] != '\0', IcsErr_DuplicateData );
   ICSTR( ics->Data != NULL, IcsErr_DuplicateData );
   ICSTR( ics->Dimensions == 0, IcsErr_NoLayout );
   if (n != IcsGetDataSize (ics)) {
      error = IcsErr_FSizeConflict;
   }
   ics->Data = src;
   ics->DataLength = n;
   ics->DataStrides = NULL;

   return error;
}

/*
 * Set the image data. The pointers must be valid until IcsClose() is called.
 * The strides indicate how to go to the next neighbor along each dimension. Use
 * this is your image data is not in one contiguous block or you want to swap
 * some dimensions in the file. ndims is the length of the strides array and
 * should match the dimensionality previously given.
 */
Ics_Error IcsSetDataWithStrides (ICS* ics, void const* src, size_t n,
                                 size_t const* strides, int ndims)
{
   ICSINIT;
   size_t lastpixel;
   int ii;

   ICS_FM_WD( ics );
   ICSTR( ics->SrcFile[0] != '\0', IcsErr_DuplicateData );
   ICSTR( ics->Data != NULL, IcsErr_DuplicateData );
   ICSTR( ics->Dimensions == 0, IcsErr_NoLayout );
   ICSTR( ndims != ics->Dimensions, IcsErr_IllParameter );
   lastpixel = 0;
   for (ii = 0; ii < ndims; ii++) {
      lastpixel += (ics->Dim[ii].Size-1) * strides[ii];
   }
   ICSTR( lastpixel*IcsGetDataTypeSize(ics->Imel.DataType) > n, IcsErr_IllParameter );
   if (n != IcsGetDataSize (ics)) {
      error = IcsErr_FSizeConflict;
   }
   ics->Data = src;
   ics->DataLength = n;
   ics->DataStrides = strides;

   return error;
}

/*
 * Set the image data source file.
 */
Ics_Error IcsSetSource (ICS* ics, char const* fname, size_t offset)
{
   ICSINIT;

   ICS_FM_WD( ics );
   ICSTR( ics->Version == 1, IcsErr_NotValidAction );
   ICSTR( ics->SrcFile[0] != '\0', IcsErr_DuplicateData );
   ICSTR( ics->Data != NULL, IcsErr_DuplicateData );
   IcsStrCpy (ics->SrcFile, fname, ICS_MAXPATHLEN);
   ics->SrcOffset = offset;

   return error;
}

/*
 * Set the compression method and compression parameter.
 */
Ics_Error IcsSetCompression (ICS* ics, Ics_Compression compression, int level)
{
   ICSINIT;

   ICS_FM_WD( ics );
   if (compression == IcsCompr_compress)
      compression = IcsCompr_gzip; /* don't try writing 'compress' compressed data. */
   ics->Compression = compression;
   ics->CompLevel = level;

   return error;
}

/*
 * Get the position of the image in the real world: the origin of the first
 * pixel, the distances between pixels and the units in which to measure.
 * If you are not interested in one of the parameters, set the pointer to NULL.
 * Dimensions start at 0.
 */
Ics_Error IcsGetPosition (ICS const* ics, int dimension, double* origin,
                          double* scale, char* units)
{
   ICSINIT;

   ICS_FM_RMD( ics );
   ICSTR( dimension >= ics->Dimensions, IcsErr_NotValidAction );
   if (origin) {
      *origin = ics->Dim[dimension].Origin;
   }
   if (scale) {
      *scale = ics->Dim[dimension].Scale;
   }
   if (units) {
      if (ics->Dim[dimension].Unit[0] != '\0') {
         strcpy (units, ics->Dim[dimension].Unit);
      }
      else {
         strcpy (units, ICS_UNITS_UNDEFINED);
      }
   }

   return error;
}

/*
 * Set the position of the image in the real world: the origin of the first
 * pixel, the distances between pixels and the units in which to measure.
 * If units is NULL or empty, it is set to the default value of "undefined".
 * Dimensions start at 0.
 */
Ics_Error IcsSetPosition (ICS* ics, int dimension, double origin,
                          double scale, char const* units)
{
   ICSINIT;

   ICS_FM_WMD( ics );
   ICSTR( dimension >= ics->Dimensions, IcsErr_NotValidAction );
   ics->Dim[dimension].Origin = origin;
   ics->Dim[dimension].Scale = scale;
   if (units && (units[0] != '\0')) {
      IcsStrCpy (ics->Dim[dimension].Unit, units, ICS_STRLEN_TOKEN);
   }
   else {
      strcpy (ics->Dim[dimension].Unit, ICS_UNITS_UNDEFINED);
   }

   return error;
}

/*
 * Get the ordering of the dimensions in the image. The ordering is defined
 * by names and labels for each dimension. The defaults are x, y, z, t (time)
 * and probe. Dimensions start at 0.
 */
Ics_Error IcsGetOrder (ICS const* ics, int dimension, char* order, char* label)
{
   ICSINIT;

   ICS_FM_RMD( ics );
   ICSTR( dimension >= ics->Dimensions, IcsErr_NotValidAction );
   if (order) {
      strcpy (order, ics->Dim[dimension].Order);
   }
   if (label) {
      strcpy (label, ics->Dim[dimension].Label);
   }

   return error;
}

/*
 * Set the ordering of the dimensions in the image. The ordering is defined
 * by providing names and labels for each dimension. The defaults are
 * x, y, z, t (time) and probe. Dimensions start at 0.
 */
Ics_Error IcsSetOrder (ICS* ics, int dimension, char const* order, char const* label)
{
   ICSINIT;

   ICS_FM_WMD( ics );
   ICSTR( dimension >= ics->Dimensions, IcsErr_NotValidAction );
   if (order && (order[0] != '\0')) {
      IcsStrCpy (ics->Dim[dimension].Order, order, ICS_STRLEN_TOKEN);
      if (label && (label[0] != '\0')) {
         IcsStrCpy (ics->Dim[dimension].Label, label, ICS_STRLEN_TOKEN);
      }
      else {
         IcsStrCpy (ics->Dim[dimension].Label, order, ICS_STRLEN_TOKEN);
      }
   }
   else {
      if (label && (label[0] != '\0')) {
         IcsStrCpy (ics->Dim[dimension].Label, label, ICS_STRLEN_TOKEN);
      }
      else {
         error = IcsErr_NotValidAction;
      }
   }

   return error;
}

/*
 * Get the coordinate system used in the positioning of the pixels.
 * Related to IcsGetPosition(). The default is "video".
 */
Ics_Error IcsGetCoordinateSystem (ICS const* ics, char* coord)
{
   ICSINIT;

   ICS_FM_RMD( ics );
   ICSTR( coord == NULL, IcsErr_NotValidAction );
   if (ics->Coord[0] != '\0') {
      strcpy (coord, ics->Coord);
   }
   else {
      strcpy (coord, ICS_COORD_VIDEO);
   }

   return error;
}

/*
 * Set the coordinate system used in the positioning of the pixels.
 * Related to IcsSetPosition(). The default is "video".
 */
Ics_Error IcsSetCoordinateSystem (ICS* ics, char const* coord)
{
   ICSINIT;

   ICS_FM_WMD( ics );
   if (coord && (coord[0] != '\0')) {
      IcsStrCpy (ics->Coord, coord, ICS_STRLEN_TOKEN);
   }
   else {
      strcpy (ics->Coord, ICS_COORD_VIDEO);
   }

   return error;
}

/*
 * Get the number of significant bits.
 */
Ics_Error IcsGetSignificantBits (ICS const* ics, size_t* nbits)
{
   ICSINIT;

   ICS_FM_RD( ics );
   ICSTR( nbits == NULL, IcsErr_NotValidAction );
   *nbits = ics->Imel.SigBits;

   return error;
}

/*
 * Set the number of significant bits.
 */
Ics_Error IcsSetSignificantBits (ICS* ics, size_t nbits)
{
   ICSINIT;
   size_t maxbits = IcsGetDataTypeSize (ics->Imel.DataType) * 8;

   ICS_FM_WD( ics );
   ICSTR( ics->Dimensions == 0, IcsErr_NoLayout );
   if (nbits > maxbits) {
      nbits = maxbits;
   }
   ics->Imel.SigBits = nbits;

   return error;
}

/*
 * Set the position of the pixel values: the offset and scaling, and the
 * units in which to measure. If you are not interested in one of the
 * parameters, set the pointer to NULL.
 */
Ics_Error IcsGetImelUnits (ICS const* ics, double* origin, double* scale, char* units)
{
   ICSINIT;

   ICS_FM_RMD( ics );
   if (origin) {
      *origin = ics->Imel.Origin;
   }
   if (scale) {
      *scale = ics->Imel.Scale;
   }
   if (units) {
      if (ics->Imel.Unit[0] != '\0') {
         strcpy (units, ics->Imel.Unit);
      }
      else {
         strcpy (units, ICS_UNITS_RELATIVE);
      }
   }

   return error;
}

/*
 * Set the position of the pixel values: the offset and scaling, and the
 * units in which to measure. If units is NULL or empty, it is set to the
 * default value of "relative".
 */
Ics_Error IcsSetImelUnits (ICS* ics, double origin, double scale, char const* units)
{
   ICSINIT;

   ICS_FM_WMD( ics );
   ics->Imel.Origin = origin;
   ics->Imel.Scale = scale;
   if (units && (units[0] != '\0')) {
      IcsStrCpy (ics->Imel.Unit, units, ICS_STRLEN_TOKEN);
   }
   else {
      strcpy (ics->Imel.Unit, ICS_UNITS_RELATIVE);
   }

   return error;
}

/*
 * Get the string for the SCIL_TYPE parameter. This string is used only
 * by SCIL_Image.
 */
Ics_Error IcsGetScilType (ICS const* ics, char* sciltype)
{
   ICSINIT;

   ICS_FM_RMD( ics );
   ICSTR( sciltype == NULL, IcsErr_NotValidAction );
   strcpy (sciltype, ics->ScilType);

   return error;
}

/*
 * Set the string for the SCIL_TYPE parameter. This string is used only
 * by SCIL_Image. It is required if you want to read the image using
 * SCIL_Image.
 */
Ics_Error IcsSetScilType (ICS* ics, char const* sciltype)
{
   ICSINIT;

   ICS_FM_WMD( ics );
   IcsStrCpy (ics->ScilType, sciltype, ICS_STRLEN_TOKEN);

   return error;
}

/*
 * As IcsSetScilType, but creates a string according to the DataType
 * in the ICS structure. It can create a string for g2d, g3d, f2d, f3d,
 * c2d and c3d.
 */
Ics_Error IcsGuessScilType (ICS* ics)
{
   ICSINIT;

   ICS_FM_WMD( ics );
   switch (ics->Imel.DataType) {
      case Ics_uint8:
      case Ics_sint8:
      case Ics_uint16:
      case Ics_sint16:
         ics->ScilType[0] = 'g';
         break;
      case Ics_real32:
         ics->ScilType[0] = 'f';
         break;
      case Ics_complex32:
         ics->ScilType[0] = 'c';
         break;
      case Ics_uint32:
      case Ics_sint32:
      case Ics_real64:
      case Ics_complex64:
         return IcsErr_NoScilType;
      case Ics_unknown:
      default:
         ics->ScilType[0] = '\0';
         return IcsErr_NotValidAction;
   }
   if (ics->Dimensions == 3) {
      ics->ScilType[1] = '3';
   }
   else if (ics->Dimensions > 3) {
      ics->ScilType[0] = '\0';
      error = IcsErr_NoScilType;
   }
   else {
      ics->ScilType[1] = '2';
   }
   ics->ScilType[2] = 'd';
   ics->ScilType[3] = '\0';

   return error;
}

/*
 * Returns a textual description of the error code.
 */
char const* IcsGetErrorText (Ics_Error error)
{
   char const* msg;
   switch (error) {
      case IcsErr_Ok:
         msg = "A-OK";
         break;
      case IcsErr_FSizeConflict:
         msg = "Non fatal error: unexpected data size";
         break;
      case IcsErr_OutputNotFilled:
         msg = "Non fatal error: the output buffer could not be completely filled";
         break;
      case IcsErr_Alloc:
         msg = "Memory allocation error";
         break;
      case IcsErr_BitsVsSizeConfl:
         msg = "Image size conflicts with bits per element";
         break;
      case IcsErr_BlockNotAllowed:
         msg = "It is not possible to read COMPRESS-compressed data in blocks";
         break;
      case IcsErr_BufferTooSmall:
         msg = "The buffer was too small to hold the given ROI";
         break;
      case IcsErr_CompressionProblem:
         msg = "Some error occurred during compression";
         break;
      case IcsErr_CorruptedStream:
         msg = "The compressed input stream is currupted";
         break;
      case IcsErr_DecompressionProblem:
         msg = "Some error occurred during decompression";
         break;
      case IcsErr_DuplicateData:
         msg = "The ICS data structure already contains incompatible stuff";
         break;
      case IcsErr_EmptyField:
         msg = "Empty field";
         break;
      case IcsErr_EndOfHistory:
         msg = "All history lines have already been returned";
         break;
      case IcsErr_EndOfStream:
         msg = "Unexpected end of stream";
         break;
      case IcsErr_FailWriteLine:
         msg = "Failed to write a line in .ics file";
         break;
      case IcsErr_FCloseIcs:
         msg = "File close error on .ics file";
         break;
      case IcsErr_FCloseIds:
         msg = "File close error on .ids file";
         break;
      case IcsErr_FCopyIds:
         msg = "Failed to copy image data from temporary file on .ics file opened for updating";
         break;
      case IcsErr_FOpenIcs:
         msg = "File open error on .ics file";
         break;
      case IcsErr_FOpenIds:
         msg = "File open error on .ids file";
         break;
      case IcsErr_FReadIcs:
         msg = "File read error on .ics file";
         break;
      case IcsErr_FReadIds:
         msg = "File read error on .ids file";
         break;
      case IcsErr_FTempMoveIcs:
         msg = "Failed to remane .ics file opened for updating";
         break;
      case IcsErr_FWriteIcs:
         msg = "File write error on .ics file";
         break;
      case IcsErr_FWriteIds:
         msg = "File write error on .ids file";
         break;
      case IcsErr_IllegalROI:
         msg = "The given ROI extends outside the image";
         break;
      case IcsErr_IllIcsToken:
         msg = "Illegal ICS token detected";
         break;
      case IcsErr_IllParameter:
         msg = "A function parameter has a value that is not legal or does not match with a value previously given";
         break;
      case IcsErr_LineOverflow:
         msg = "Line overflow in .ics file";
         break;
      case IcsErr_MissBits:
         msg = "Missing \"bits\" element in .ics file";
         break;
      case IcsErr_MissCat:
         msg = "Missing main category";
         break;
      case IcsErr_MissingData:
         msg = "There is no Data defined";
         break;
      case IcsErr_MissLayoutSubCat:
         msg = "Missing layout subcategory";
         break;
      case IcsErr_MissParamSubCat:
         msg = "Missing parameter subcategory";
         break;
      case IcsErr_MissRepresSubCat:
         msg = "Missing representation subcategory";
         break;
      case IcsErr_MissSensorSubCat:
         msg = "Missing sensor subcategory";
         break;
      case IcsErr_MissSensorSubSubCat:
         msg = "Missing sensor subsubcategory";
         break;
      case IcsErr_MissSubCat:
         msg = "Missing sub category";
         break;
      case IcsErr_NoLayout:
         msg = "Layout parameters missing or not defined";
         break;
      case IcsErr_NoScilType:
         msg = "There doesn't exist a SCIL_TYPE value for this image";
         break;
      case IcsErr_NotIcsFile:
         msg = "Not an ICS file";
         break;
      case IcsErr_NotValidAction:
         msg = "The function won't work on the ICS given";
         break;
      case IcsErr_TooManyChans:
         msg = "Too many channels specified";
         break;
      case IcsErr_TooManyDims:
         msg = "Data has too many dimensions";
         break;
      case IcsErr_UnknownCompression:
         msg = "Unknown compression type";
         break;
      case IcsErr_UnknownDataType:
         msg = "The datatype is not recognized";
         break;
      case IcsErr_WrongZlibVersion:
         msg = "libics is linking to a different version of zlib than used during compilation";
         break;
      default:
         msg = "Some error occurred I know nothing about.";
   }
   return msg;
}
