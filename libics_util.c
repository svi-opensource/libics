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
 * FILE : libics_util.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsGetLibVersion()
 *   IcsGetIcsName()
 *   IcsGetIdsName()
 *   IcsInit()
 *   IcsGetDataTypeSize()
 *   IcsGetPropsDataType()
 *   IcsGetDataTypeProps()
 *
 * The following internal functions are contained in this file:
 *
 *   IcsStrCpy()
 *   IcsAppendChar()
 *   IcsGetFileName()
 *   IcsExtensionFind()
 *   IcsGetBytesPerSample()
 *   IcsOpenIcs()
 */

#include <stdlib.h>
#include <string.h>
#include "libics_intern.h"

#ifdef HAVE_STRINGS_STRCASECMP
   #include <strings.h>   /* For strcasecmp() */
#endif
#ifndef HAVE_STRING_STRCASECMP
#ifndef HAVE_STRINGS_STRCASECMP
   #define strcasecmp stricmp
#endif
#endif

char const ICSEXT[] = ".ics";
char const IDSEXT[] = ".ids";
char const IDSEXT_Z[] = ".ids.Z";
char const IDSEXT_GZ[] = ".ids.gz";

/*
 * This function can be used to check for the correct library version:
 * if (strcmp (ICSLIB_VERSION, IcsGetLibVersion ()) != 0) return ERRORCODE;
 */
char const* IcsGetLibVersion (void)
{
   return ICSLIB_VERSION;
}

/*
 * Parse a number string and return the value in a size_t.
 */
size_t IcsStrToSize (char const* str)
{
   unsigned long ulsize;
   size_t size;

   ulsize = strtoul (str, NULL, 10);
   size = (size_t) ulsize;

   return size;
}

/*
 * A safe strcpy. len is the number of characters in dest. Up to len-1 characters copied.
 */
void IcsStrCpy (char* dest, char const* src, int len)
{
   if (dest != src) {
      int nchar = strlen(src);
      nchar = (nchar > len-1) ? len-1 : nchar;
      memcpy (dest, src, nchar);
      dest[nchar] = 0;
   }
}

/*
 * Append a character to a string
 */
void IcsAppendChar (char* Line, char ch)
{
   int len = strlen (Line);
   Line[len] = ch;
   Line[len+1] = '\0';
}

/*
 * Find the start of the filename
 */
static char* IcsFileNameFind (char const* str) {
   char const* begin;

#ifdef WIN32
   /* For Windows we check both kinds of path separators */
   begin = strrchr (str, '\\');
   if (begin == NULL) {
      begin = strrchr (str, '/');
   }
#else
   begin = strrchr (str, '/');
#endif
   if (begin == NULL) {
      begin = str;
   }
   else {
      begin++;
   }

   return (char*)begin;
}

/*
 * Find the start of the '.ics' or '.ids' extension.
 * Also handle filenames ending in '.ids.Z' or '.ids.gz'.
 * All character comparisons must be case insensitive.
 * Return a pointer to the start of the extension
 * or NULL if no extension could be found.
 */
char* IcsExtensionFind (char const* str)
{
   int len;
   char const* ext;

   len = strlen (str);

   ext = str + len - (sizeof(ICSEXT) - 1);
   if (ext >= str && strcasecmp (ext, ICSEXT) == 0) {
      return (char*)ext;
   }

   ext = str + len - (sizeof(IDSEXT) - 1);
   if (ext >= str && strcasecmp (ext, IDSEXT) == 0) {
      return (char*)ext;
   }

   ext = str + len - (sizeof(IDSEXT_Z) - 1);
   if (ext >= str && strcasecmp (ext, IDSEXT_Z) == 0) {
      return (char*)ext;
   }

   ext = str + len - (sizeof(IDSEXT_GZ) - 1);
   if (ext >= str && strcasecmp (ext, IDSEXT_GZ) == 0) {
      return (char*)ext;
   }

   return NULL;
}

/*
 * Strip the path from everything but the file name
 * (including extension).
 */
void IcsGetFileName (char* dest, char const* src)
{
   char const* begin;
   char* end;

   begin = IcsFileNameFind (src);
   IcsStrCpy (dest, begin, ICS_MAXPATHLEN);
   end = IcsExtensionFind (dest);
   if (end != NULL) {
      *end = '\0';
   }
}

/*
 * Make a filename ending in '.ics' from the given filename.
 * If the filename ends in '.IDS' then make this '.ICS'.
 * Also accept filenames ending in '.ids.Z' and '.ids.gz',
 * but strip the compression extension.
 */
char* IcsGetIcsName (char* dest, char const* src, int forcename)
{
   char* end;

   IcsStrCpy (dest, src, ICS_MAXPATHLEN);
   end = IcsExtensionFind (dest);
   if (end != NULL) {
      if (strcasecmp (end, ICSEXT) == 0) {
         return dest;
      }
      else {
         /* Keep same case. */
         if (end[2] == 'd') {
            end[2] = 'c';
            end[4] = '\0';
            return dest;
         }
         else if (end[2] == 'D') {
            end[2] = 'C';
            end[4] = '\0';
            return dest;
         }
         else { /* does not happen! */
            if (!forcename) {
               *end = '\0';
            }
         }
      }
   }
   if (!forcename && strlen(dest) + strlen(ICSEXT) + 1 < ICS_MAXPATHLEN) {
      strcat (dest, ICSEXT);
   }

   return dest;
}

/*
 * Make a filename ending in '.ids' from the given filename.
 * If the filename ends in '.ICS' then make this '.IDS'.
 * Also accept filenames ending in '.ids.Z' and '.ids.gz',
 * but strip the compression extension.
 */
char* IcsGetIdsName (char* dest, char const* src)
{
   char* end;

   IcsStrCpy (dest, src, ICS_MAXPATHLEN);
   end = IcsExtensionFind (dest);
   if (end != NULL) {
      if (strcasecmp (end, ICSEXT) == 0) {
         /* Keep same case. */
         if (end[2] == 'c') {
            end[2] = 'd';
            return dest;
         }
         else if (end[2] == 'C') {
            end[2] = 'D';
            return dest;
         }
         else {
            *end = '\0';
         }
      }
      else {
         end[4] = '\0';
         return dest;
      }
   }
   if (strlen(dest) + strlen(IDSEXT) + 1 < ICS_MAXPATHLEN) {
      strcat (dest, IDSEXT);
   }

   return dest;
}

/*
 * Open an .ics file, even if the name given end in .ids.
 */
Ics_Error IcsOpenIcs (FILE** fpp, char* filename, int forcename)
{
   ICSINIT;
   FILE* fp;
   char FileName[ICS_MAXPATHLEN];

   IcsGetIcsName (FileName, filename, forcename);
   fp = fopen (FileName, "rb");
   ICSTR( fp == NULL, IcsErr_FOpenIcs );

   *fpp = fp;
   strcpy (filename, FileName);

   return error;
}

/*
 * Initialize the Ics_Header structure.
 */
void IcsInit (Ics_Header* IcsStruct)
{
   int ii;

   IcsStruct->Version = 1; /* We write an ICS v.1.0 as default */
   IcsStruct->FileMode = IcsFileMode_write;
   IcsStruct->Data = NULL;
   IcsStruct->DataLength = 0;
   IcsStruct->DataStrides = NULL;
   IcsStruct->Filename[0] = '\0';
   IcsStruct->Dimensions = 0;
   for (ii = 0; ii < ICS_MAXDIM; ii++)
   {
      IcsStruct->Dim[ii].Size = 0;
      IcsStruct->Dim[ii].Origin = 0.0;
      IcsStruct->Dim[ii].Scale = 1.0;
      IcsStruct->Dim[ii].Order[0] = '\0';
      IcsStruct->Dim[ii].Label[0] = '\0';
      IcsStruct->Dim[ii].Unit[0] = '\0';
   }
   IcsStruct->Imel.DataType = Ics_unknown;
   IcsStruct->Imel.SigBits = 0;
   IcsStruct->Imel.Origin = 0.0;
   IcsStruct->Imel.Scale = 1.0;
   IcsStruct->Imel.Unit[0] = '\0';
   IcsStruct->Coord[0] = '\0';
   IcsStruct->Compression = IcsCompr_uncompressed;
   IcsStruct->CompLevel = 0;
   IcsStruct->History = NULL;
   IcsStruct->BlockRead = NULL;
   IcsStruct->SrcFile[0] = '\0';
   IcsStruct->SrcOffset = 0;
   for (ii = 0; ii < ICS_MAX_IMEL_SIZE; ii++) {
      IcsStruct->ByteOrder[ii] = 0;
   }
   IcsStruct->WriteSensor = 0;
   IcsStruct->Type[0] = '\0';
   IcsStruct->Model[0]= '\0';
   IcsStruct->RefrInxMedium = 0.0;
   IcsStruct->NumAperture = 0.0;
   IcsStruct->RefrInxLensMedium = 0.0;
   IcsStruct->PinholeSpacing = 0.0;
   IcsStruct->SensorChannels = 0;
   for (ii = 0; ii < ICS_MAX_LAMBDA; ii++) {
      IcsStruct->PinholeRadius[ii] = 0.0;
      IcsStruct->LambdaEx[ii] = 0.0;
      IcsStruct->LambdaEm[ii] = 0.0;
      IcsStruct->ExPhotonCnt[ii] = 1;
   }
   IcsStruct->ScilType[0] = '\0';
}

/*
 * Find the number of bytes per sample.
 */
int IcsGetBytesPerSample (Ics_Header const* IcsStruct)
{
   return IcsGetDataTypeSize (IcsStruct->Imel.DataType);
}

/*
 * Get the size of the Ics_DataType in bytes.
 */
size_t IcsGetDataTypeSize (Ics_DataType DataType)
{
   size_t bytes;

   switch (DataType) {
      case Ics_uint8:
      case Ics_sint8:
         bytes = 1;
         break;
      case Ics_uint16:
      case Ics_sint16:
         bytes = 2;
         break;
      case Ics_uint32:
      case Ics_sint32:
      case Ics_real32:
         bytes = 4;
         break;
      case Ics_real64:
      case Ics_complex32:
         bytes = 8;
         break;
      case Ics_complex64:
         bytes = 16;
         break;
      default:
         bytes = 0;
   }

   return bytes;
}

/*
 * Get the properties of the Ics_DataType
 */
void IcsGetPropsDataType (Ics_DataType DataType, Ics_Format* format, int* sign, size_t* bits)
{
   *bits = IcsGetDataTypeSize (DataType) * 8;
   *sign = 1;
   switch (DataType) {
      case Ics_uint8:
      case Ics_uint16:
      case Ics_uint32:
         *sign = 0;
      case Ics_sint8:
      case Ics_sint16:
      case Ics_sint32:
         *format = IcsForm_integer;
         break;
      case Ics_real32:
      case Ics_real64:
         *format = IcsForm_real;
         break;
      case Ics_complex32:
      case Ics_complex64:
         *format = IcsForm_complex;
         break;
      default:
         *format = IcsForm_unknown;
   }
}

/*
 * Get the Ics_DataType belonging to the given properties
 */
void IcsGetDataTypeProps (Ics_DataType* DataType, Ics_Format format, int sign, size_t bits)
{
   switch (format) {
      case IcsForm_integer:
         switch (bits) {
            case 8:
               *DataType = sign ? Ics_sint8 : Ics_uint8;
               break;
            case 16:
               *DataType = sign ? Ics_sint16 : Ics_uint16;
               break;
            case 32:
               *DataType = sign ? Ics_sint32 : Ics_uint32;
               break;
            default:
               *DataType = Ics_unknown;
         }
         break;
      case IcsForm_real:
         switch (bits) {
            case 32:
               *DataType = Ics_real32;
               break;
            case 64:
               *DataType = Ics_real64;
               break;
            default:
               *DataType = Ics_unknown;
         }
         break;
      case IcsForm_complex:
         switch (bits) {
            case 2*32:
               *DataType = Ics_complex32;
               break;
            case 2*64:
               *DataType = Ics_complex64;
               break;
            default:
               *DataType = Ics_unknown;
         }
         break;
      default:
         *DataType = Ics_unknown;
   }
}
