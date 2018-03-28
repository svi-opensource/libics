/*
 * libics: Image Cytometry Standard file reading and writing.
 *
 * Copyright 2018:
 *   Scientific Volume Imaging Holding B.V.
 *   Laapersveld 63, 1213 VB Hilversum, The Netherlands
 *   https://www.svi.nl
 *
 * Contact: libics@svi.nl
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * FILE : libics_random_access.h
 *
 * Definitions for the random access (read/write ROIs) functions
 *
 * The random access functions can only be used on ICS files satisfying
 * the following constraints:
 * - single file (implies ICS v2)
 * - uncompressed
 *
 * ROI = region of interest
 * The general procedure is:
 *   ICSRA *icsra;
 *   # open existing ics file for reading and writing
 *   Ics_RaOpen( &icsra, filename, mode );
 *   # one or more calls to Ics_RaReadOrWrite reading/writing
 *   # ROIs in the on-disk image from/into a ROI of an in memory image
 *   Ics_RaClose( icsra );   # closes the file
 */

#ifndef LIBICS_RANDOM_ACCESS_H
#define LIBICS_RANDOM_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "libics.h"



/* Structures that define the image representation. They are only used inside
   the ICS data structure. */
typedef struct {
  ICS   ics;
  FILE *fp;
  int   bomap[ICS_MAX_IMEL_SIZE];   /* byte order mapping */
  int   readOnly;
} ICSRA;

/* Open an existing ics file (must be uncompressed v2) for random read/write
 * access. The ICSRA structure that is returned is a handle that allows the
 * other random access functions to keep track of the file. Note that this
 * is different from the ICS structure that is used throughout the remainder
 * of libics. The <mode> parameter is similar to that of IcsOpen() however
 * using different modes:
 *   "ro" for read-only access
 *   "rw" for read-write access (the default, i.e. "" is interpreted as "rw")
 * this can be affixed with "f" and/or "l" with the same meaning as they
 * have in IcsOpen. Note that read-write access applies to the image data
 * in the ICS file; the header remains untouched.
 */
ICSEXPORT Ics_Error Ics_RaOpen(ICSRA      **icsra,
                               const char  *filename,
                               const char  *mode);
/* Closes the ics file */
ICSEXPORT Ics_Error Ics_RaClose(ICSRA      *icsra);
/* Read or write into an ics file opened with Ics_RaOpen(). A region of
 * interest can be specified for either of both the ics file and the in-memory
 * image. The parameters are:
 *
 *   ICSRA *icsra
 *   int        dm           included as a sanity check. The dimensionality of
 *                           the in-memory image. Must equal the dimensionality
 *                           of the ICS image data
 * 0 size_t    *rg_dims      dimensions of the regions to be copied.
 *                           NULL: use dimensions of ics file
 * 0 size_t    *fl_origin    origin of the ROI within the ics
 * 0 ptrdiff_t *fl_interval  step size to be made between pixels in the ics file
 *   void      *image        image data
 * 0 size_t    *im_dims      sanity check. If provided is used to check
 *                           im_origin and im_interval
 *   size_t    *im_stride    step size to be made in memory to move to the
 *                           next pixel (describes the image layout)
 * 0 size_t    *im_origin    origin of the ROI within the image
 * 0 ptrdiff_t *im_interval  step size to be made between pixels
 *   Ics_FileMode mode       allowed are:
 *                              IcsFileMode_read  : move data ICS -> image
 *                              IcsFileMode_write : move data image -> ICS
 *
 * Any parameter with a "0" in front can be set to NULL
 *
 * Note on stride and interval parameters: N-D image data is stored in a
 * 1D array. Usually, but not necessarily, I(x,y) and I(x+1,y) are stored
 * next to each other in this 1D array. As a consequence I(x,y) and I(x,y+1)
 * can lie next to each other. They are e.g. stored Sy elements apart. This
 * is the <stride> for dimension y. The stride array describes the layout
 * of an image in memory (or in the file). The code knows the layout of
 * the ics file, hence no fl_stride parameter is present. The im_stride
 * parameter is obligatory. The strides are given in pixels, not bytes.
 *   The interval parameters allows you to skip pixels (downsample) while
 * copying (both at the source and at the destination). If we consider
 * e.g. fl_interval=[3,1] then every third pixel is read along the x-axis:
 * the step size in the underlying 1D array becomes stride*interval. The
 * interval values are allowed to be negative.
 * 
 * For clarity: the rg_dims parameter gives the amount of pixels to be copied
 * along each dimension. When using intervals, this means the source or
 * destination region covers rg_dims*abs(interval) pixels
 */
ICSEXPORT Ics_Error Ics_RaReadOrWrite(const ICSRA        *icsra,
                                      const int           dm,
                                      const size_t       *rg_dims,
                                      const size_t       *fl_origin,
                                      const ptrdiff_t    *fl_interval,
                                      const void         *image,
                                      const size_t       *im_dims,
                                      const size_t       *im_stride,
                                      const size_t       *im_origin,
                                      const ptrdiff_t    *im_interval,
                                      const Ics_FileMode  mode);
/* Create an ics file with the given name, data type and dimensions. It can
 * then be opened with Ics_RaOpen and accessed with Ics_RaReadOrWrite.
 * The file is created by writing a single byte at the last position. I cannot
 * find a definite statement on whether the intervening data is zeroed by
 * the operating system/POSIX standard. Best not to depend on that! The mode
 * parameter is identical to Ics_RaOpen's mode parameter
 */
ICSEXPORT Ics_Error Ics_RaCreate(const char         *filename,
                                 const char         *mode,
                                 const Ics_DataType  dt,
                                 const int           significantBits,
                                 const int           dm,
                                 const size_t       *dims);

#ifdef __cplusplus
}
#endif

#endif
