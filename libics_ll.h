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
 * FILE : libics_ll.h
 *
 * This file defines the low-level interface functions. Include it in your
 * source code only if that is the way you want to go.
 */

#ifndef LIBICS_LL_H
#define LIBICS_LL_H

#ifndef LIBICS_H
#include "libics.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef ICS Ics_Header;

/*
 * These are the known data formats for imels.
 */
typedef enum {
   IcsForm_unknown = 0,
   IcsForm_integer,
   IcsForm_real,
   IcsForm_complex
} Ics_Format;


/* Definitions of separators in the .ics file for writing: */
#define ICS_FIELD_SEP '\t'
#define ICS_EOL '\n'

/*
 * Function declarations and short explanation:
 */

ICSEXPORT Ics_Error IcsReadIcs (Ics_Header* IcsStruct, char const* filename,
                                int forcename, int forcelocale);
/* Reads a .ics file into an Ics_Header structure. */

ICSEXPORT Ics_Error IcsWriteIcs (Ics_Header* IcsStruct, char const* filename);
/* Writes an Ics_Header structure into a .ics file. */

ICSEXPORT Ics_Error IcsOpenIds (Ics_Header* IcsStruct);
/* Initializes image data reading. */

ICSEXPORT Ics_Error IcsCloseIds (Ics_Header* IcsStruct);
/* Ends image data reading. */

ICSEXPORT Ics_Error IcsReadIdsBlock (Ics_Header* IcsStruct, void* outbuf, size_t len);
/* Reads image data block from disk. */

ICSEXPORT Ics_Error IcsSkipIdsBlock (Ics_Header* IcsStruct, size_t len);
/* Skips image data block from disk (fseek forward). */

ICSEXPORT Ics_Error IcsSetIdsBlock (Ics_Header* IcsStruct, long offset, int whence);
/* Sets the file pointer into the image data on disk (fseek anywhere). */

ICSEXPORT Ics_Error IcsReadIds (Ics_Header* IcsStruct, void* dest, size_t n);
/* Reads image data from disk. */

ICSEXPORT Ics_Error IcsWriteIds (Ics_Header const* IcsStruct);
/* Writes image data to disk. */

ICSEXPORT void IcsInit (Ics_Header* IcsStruct);
/* Initializes the Ics_Header structure to default values and zeros. */

ICSEXPORT char* IcsGetIcsName (char* dest, char const* src, int forcename);
/* Appends ".ics" to the filename (removing ".ids" if present)
 * If (forcename) we do change ".ids" into ".ics", but do not add anything. */

ICSEXPORT char* IcsGetIdsName (char* dest, char const* src);
/* Appends ".ids" to the filename (removing ".ics" if present). */

ICSEXPORT char* IcsExtensionFind (char const* str);
/* Return pointer to .ics or .ids extension or NULL if not found. */

ICSEXPORT size_t IcsGetDataTypeSize (Ics_DataType DataType);
/* Returns the size in bytes of the data type. */

ICSEXPORT void IcsGetPropsDataType (Ics_DataType DataType, Ics_Format* format,
                                    int* sign, size_t* bits);
/* Fills in format, sign and bits according to the data type. */

ICSEXPORT void IcsGetDataTypeProps (Ics_DataType* DataType, Ics_Format format,
                                    int sign, size_t bits);
/* Sets the data type according to format, sign and bits. */

ICSEXPORT void IcsFreeHistory (Ics_Header* ics);
/* Free the memory allocated for history. */

#ifdef __cplusplus
}
#endif

#endif /* LIBICS_LL_H */
