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

static Ics_Error IcsToken2Str (Ics_Token T, char* CPtr)
{
   ICSINIT;
   int notFound = 1, ii;

   /* Search the globally defined categories for a token match: */
   ii = 0;
   while (notFound && ii < G_Categories.Entries) {
      notFound = T != G_Categories.List[ii].Token;
      if (!notFound) {
         strcpy (CPtr, G_Categories.List[ii].Name);
      }
      ii++;
   }
   ii = 0;
   while (notFound && ii < G_SubCategories.Entries) {
      notFound = T != G_SubCategories.List[ii].Token;
      if (!notFound) {
         strcpy (CPtr, G_SubCategories.List[ii].Name);
      }
      ii++;
   }
   ii = 0;
   while (notFound && ii < G_SubSubCategories.Entries) {
      notFound = T != G_SubSubCategories.List[ii].Token;
      if (!notFound) {
         strcpy (CPtr, G_SubSubCategories.List[ii].Name);
      }
      ii++;
   }
   ii = 0;
   while (notFound && ii < G_Values.Entries) {
      notFound = T != G_Values.List[ii].Token;
      if (!notFound) {
         strcpy (CPtr, G_Values.List[ii].Name);
      }
      ii++;
   }
   ICSTR( notFound, IcsErr_IllIcsToken );

   return error;
}

static Ics_Error IcsFirstToken (char* Line, Ics_Token T)
{
   ICSDECL;
   char tokenName[ICS_STRLEN_TOKEN];

   ICSXR( IcsToken2Str (T, tokenName) );
   strcpy (Line, tokenName);
   IcsAppendChar (Line, ICS_FIELD_SEP);

   return error;
}

static Ics_Error IcsAddToken (char* Line, Ics_Token T)
{
   ICSDECL;
   char tokenName[ICS_STRLEN_TOKEN];

   ICSXR( IcsToken2Str (T, tokenName) );
   ICSTR( strlen (Line) + strlen (tokenName) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, tokenName);
   IcsAppendChar (Line, ICS_FIELD_SEP);

   return error;
}

static Ics_Error IcsAddLastToken (char* Line, Ics_Token T)
{
   ICSDECL;
   char tokenName[ICS_STRLEN_TOKEN];

   ICSXR( IcsToken2Str (T, tokenName) );
   ICSTR( strlen (Line) + strlen (tokenName) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, tokenName);
   IcsAppendChar (Line, ICS_EOL);

   return error;
}

static Ics_Error IcsFirstText (char* Line, char* Text)
{
   ICSINIT;

   ICSTR( Text[0] == '\0', IcsErr_EmptyField );
   ICSTR( strlen (Text) + 2 > ICS_LINE_LENGTH, IcsErr_LineOverflow );
   strcpy (Line, Text);
   IcsAppendChar (Line, ICS_FIELD_SEP);

   return error;
}

static Ics_Error IcsAddText (char* Line, char* Text)
{
   ICSINIT;

   ICSTR( Text[0] == '\0', IcsErr_EmptyField );
   ICSTR( strlen (Line) + strlen (Text) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, Text);
   IcsAppendChar (Line, ICS_FIELD_SEP);

   return error;
}

static Ics_Error IcsAddLastText (char* Line, char* Text)
{
   ICSINIT;

   ICSTR( Text[0] == '\0', IcsErr_EmptyField );
   ICSTR( strlen (Line) + strlen (Text) + 2 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, Text);
   IcsAppendChar (Line, ICS_EOL);

   return error;
}

static Ics_Error IcsAddInt (char* Line, long int I)
{
   ICSINIT;
   char intStr[ICS_STRLEN_OTHER];

   sprintf (intStr, "%ld%c", I, ICS_FIELD_SEP);
   ICSTR( strlen (Line) + strlen (intStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, intStr);

   return error;
}

static Ics_Error IcsAddLastInt (char* Line, long int I)
{
   ICSINIT;
   char intStr[ICS_STRLEN_OTHER];

   sprintf (intStr, "%ld%c", I, ICS_EOL);
   ICSTR( strlen (Line) + strlen (intStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, intStr);

   return error;
}

static Ics_Error IcsAddDouble (char* Line, double D)
{
   ICSINIT;
   char dStr[ICS_STRLEN_OTHER];

   if (D==0 || (fabs(D) < ICS_MAX_DOUBLE && fabs(D) >= ICS_MIN_DOUBLE)) {
      sprintf (dStr, "%f%c", D, ICS_FIELD_SEP);
   }
   else {
      sprintf (dStr, "%e%c", D, ICS_FIELD_SEP);
   }
   ICSTR( strlen (Line) + strlen (dStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, dStr);

   return error;
}

static Ics_Error IcsAddLastDouble (char* Line, double D)
{
   ICSINIT;
   char dStr[ICS_STRLEN_OTHER];

   if (D==0 || (fabs(D) < ICS_MAX_DOUBLE && fabs(D) >= ICS_MIN_DOUBLE)) {
      sprintf (dStr, "%f%c", D, ICS_EOL);
   }
   else {
      sprintf (dStr, "%e%c", D, ICS_EOL);
   }
   ICSTR( strlen (Line) + strlen (dStr) + 1 > ICS_LINE_LENGTH,
          IcsErr_LineOverflow );
   strcat (Line, dStr);

   return error;
}

static Ics_Error IcsAddLine (char* Line, FILE* fp)
{
   ICSINIT;

   ICSTR( fputs (Line, fp) == EOF, IcsErr_FWriteIcs );

   return error;
}

static Ics_Error WriteIcsSource (Ics_Header* IcsStruct, FILE* fp)
{
   ICSINIT;
   int problem;
   char line[ICS_LINE_LENGTH];

   if ((IcsStruct->Version >= 2) && (IcsStruct->SrcFile[0] != '\0')) {
      /* Write the source filename to the file */
      problem = IcsFirstToken (line, ICSTOK_SOURCE);
      problem |= IcsAddToken (line, ICSTOK_FILE);
      problem |= IcsAddLastText (line, IcsStruct->SrcFile);
      ICSTR( problem, IcsErr_FailWriteLine );
      ICSXR( IcsAddLine (line, fp) );

      /* Now write the source file offset to the file */
      problem = IcsFirstToken (line, ICSTOK_SOURCE);
      problem |= IcsAddToken (line, ICSTOK_OFFSET);
      problem |= IcsAddLastInt (line, (long int)IcsStruct->SrcOffset);
      ICSTR( problem, IcsErr_FailWriteLine );
      ICSXR( IcsAddLine (line, fp) );
   }

   return error;
}

static Ics_Error WriteIcsLayout (Ics_Header* IcsStruct, FILE* fp)
{
   ICSDECL;
   int problem, ii;
   char line[ICS_LINE_LENGTH];

   /* Write the number of parameters to the buffer: */
   ICSTR( IcsStruct->Dimensions < 1, IcsErr_NoLayout );
   ICSTR( IcsStruct->Dimensions > ICS_MAXDIM, IcsErr_TooManyDims );
   problem = IcsFirstToken (line, ICSTOK_LAYOUT);
   problem |= IcsAddToken (line, ICSTOK_PARAMS);
   problem |= IcsAddLastInt (line, IcsStruct->Dimensions + 1);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Now write the order identifiers to the buffer: */
   problem = IcsFirstToken (line, ICSTOK_LAYOUT);
   problem |= IcsAddToken (line, ICSTOK_ORDER);
   problem |= IcsAddText (line, ICS_ORDER_BITS);
   for (ii = 0; ii < IcsStruct->Dimensions-1; ii++) {
      ICSTR( *(IcsStruct->Dim[ii].Order) == '\0', IcsErr_NoLayout );
      problem |= IcsAddText (line, IcsStruct->Dim[ii].Order);
   }
   ICSTR( *(IcsStruct->Dim[ii].Order) == '\0', IcsErr_NoLayout );
   problem |= IcsAddLastText (line, IcsStruct->Dim[ii].Order);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Write the sizes: */
   problem = IcsFirstToken (line, ICSTOK_LAYOUT);
   problem |= IcsAddToken (line, ICSTOK_SIZES);
   problem |= IcsAddInt (line, (long int)IcsGetDataTypeSize (IcsStruct->Imel.DataType)*8);
   for (ii = 0; ii < IcsStruct->Dimensions-1; ii++) {
      ICSTR( IcsStruct->Dim[ii].Size == 0, IcsErr_NoLayout );
      problem |= IcsAddInt (line, (long int) IcsStruct->Dim[ii].Size);
   }
   ICSTR( IcsStruct->Dim[ii].Size == 0, IcsErr_NoLayout );
   problem |= IcsAddLastInt (line, (long int) IcsStruct->Dim[ii].Size);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Coordinates class. Video (default) means 0,0 corresponds with top-left. */
   if (*(IcsStruct->Coord) == '\0') {
      strcpy (IcsStruct->Coord, ICS_COORD_VIDEO);
   }
   problem = IcsFirstToken (line, ICSTOK_LAYOUT);
   problem |= IcsAddToken (line, ICSTOK_COORD);
   problem |= IcsAddLastText (line, IcsStruct->Coord);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Number of significant bits, default is the number of bits/sample: */
   if (IcsStruct->Imel.SigBits == 0) {
      IcsStruct->Imel.SigBits = IcsGetDataTypeSize (IcsStruct->Imel.DataType)*8;
   }
   problem = IcsFirstToken (line, ICSTOK_LAYOUT);
   problem |= IcsAddToken (line, ICSTOK_SIGBIT);
   problem |= IcsAddLastInt (line, (long int)IcsStruct->Imel.SigBits);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   return error;
}

static Ics_Error WriteIcsRep (Ics_Header* IcsStruct, FILE* fp)
{
   ICSDECL;
   int problem, ii;
   char line[ICS_LINE_LENGTH];
   Ics_Format Format;
   int Sign;
   size_t Bits;

   IcsGetPropsDataType (IcsStruct->Imel.DataType, &Format, &Sign, &Bits);

   /* Write basic format, i.e. integer, float or complex, default is integer: */
   problem = IcsFirstToken (line, ICSTOK_REPRES);
   problem |= IcsAddToken (line, ICSTOK_FORMAT);
   switch (Format) {
      case IcsForm_integer:
         problem |= IcsAddLastToken (line, ICSTOK_FORMAT_INTEGER);
         break;
      case IcsForm_real:
         problem |= IcsAddLastToken (line, ICSTOK_FORMAT_REAL);
         break;
      case IcsForm_complex:
         problem |= IcsAddLastToken (line, ICSTOK_FORMAT_COMPLEX);
         break;
      default:
         return IcsErr_UnknownDataType;
   }
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Signal whether the 'basic format' is signed or unsigned. Rubbish for
    * float or complex, but this seems to be the definition. */
   problem = IcsFirstToken (line, ICSTOK_REPRES);
   problem |= IcsAddToken (line, ICSTOK_SIGN);
   if (Sign == 1) {
      problem |= IcsAddLastToken (line, ICSTOK_SIGN_SIGNED);
   }
   else {
      problem |= IcsAddLastToken (line, ICSTOK_SIGN_UNSIGNED);
   }
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Signal whether the entire data array is compressed and if so by what
    * compression technique: */
   problem = IcsFirstToken (line, ICSTOK_REPRES);
   problem |= IcsAddToken (line, ICSTOK_COMPR);
   switch (IcsStruct->Compression) {
      case IcsCompr_uncompressed:
         problem |= IcsAddLastToken (line, ICSTOK_COMPR_UNCOMPRESSED);
         break;
      case IcsCompr_compress:
         problem |= IcsAddLastToken (line, ICSTOK_COMPR_COMPRESS);
         break;
      case IcsCompr_gzip:
         problem |= IcsAddLastToken (line, ICSTOK_COMPR_GZIP);
         break;
      default:
         return IcsErr_UnknownCompression;
   }
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Define the byteorder. This is supposed to resolve little/big
    * endian problems. We will overwrite anything the calling function
    * put in here. This must be the machine's byte order. */
   IcsFillByteOrder (IcsGetDataTypeSize (IcsStruct->Imel.DataType), IcsStruct->ByteOrder);
   problem = IcsFirstToken (line, ICSTOK_REPRES);
   problem |= IcsAddToken (line, ICSTOK_BYTEO);
   for (ii = 0; ii < (int)IcsGetDataTypeSize (IcsStruct->Imel.DataType) - 1; ii++) {
      problem |= IcsAddInt (line, IcsStruct->ByteOrder[ii]);
   }
   problem |= IcsAddLastInt (line, IcsStruct->ByteOrder[ii]);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* SCIL_Image compatability stuff: SCIL_TYPE */
   if (IcsStruct->ScilType[0] != '\0') {
      problem = IcsFirstToken (line, ICSTOK_REPRES);
      problem |= IcsAddToken (line, ICSTOK_SCILT);
      problem |= IcsAddLastText (line, IcsStruct->ScilType);
      ICSTR( problem, IcsErr_FailWriteLine );
      ICSXR( IcsAddLine (line, fp) );
   }

   return error;
}

static Ics_Error WriteIcsParam (Ics_Header* IcsStruct, FILE* fp)
{
   ICSINIT;
   int problem, ii;
   char line[ICS_LINE_LENGTH];

   /* Define the origin, scaling factors and the units */
   problem = IcsFirstToken (line, ICSTOK_PARAM);
   problem |= IcsAddToken (line, ICSTOK_ORIGIN);
   problem |= IcsAddDouble (line, IcsStruct->Imel.Origin);
   for (ii = 0; ii < IcsStruct->Dimensions-1; ii++) {
      problem |= IcsAddDouble (line, IcsStruct->Dim[ii].Origin);
   }
   problem |= IcsAddLastDouble (line, IcsStruct->Dim[ii].Origin);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   problem = IcsFirstToken (line, ICSTOK_PARAM);
   problem |= IcsAddToken (line, ICSTOK_SCALE);
   problem |= IcsAddDouble (line, IcsStruct->Imel.Scale);
   for (ii = 0; ii < IcsStruct->Dimensions-1; ii++) {
      problem |= IcsAddDouble (line, IcsStruct->Dim[ii].Scale);
   }
   problem |= IcsAddLastDouble (line, IcsStruct->Dim[ii].Scale);
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   problem = IcsFirstToken (line, ICSTOK_PARAM);
   problem |= IcsAddToken (line, ICSTOK_UNITS);
   if (IcsStruct->Imel.Unit[0] == '\0') {
      problem |= IcsAddText (line, ICS_UNITS_RELATIVE);
   }
   else {
      problem |= IcsAddText (line, IcsStruct->Imel.Unit);
   }
   for (ii = 0; ii < IcsStruct->Dimensions-1; ii++) {
      if (IcsStruct->Dim[ii].Unit[0] == '\0') {
         problem |= IcsAddText (line, ICS_UNITS_UNDEFINED);
      }
      else {
         problem |= IcsAddText (line, IcsStruct->Dim[ii].Unit);
      }
   }
   if (IcsStruct->Dim[ii].Unit[0] == '\0') {
      problem |= IcsAddLastText (line, ICS_UNITS_UNDEFINED);
   }
   else {
      problem |= IcsAddLastText (line, IcsStruct->Dim[ii].Unit);
   }
   ICSTR( problem, IcsErr_FailWriteLine );
   ICSXR( IcsAddLine (line, fp) );

   /* Write labels associated with the dimensions to the .ics file, if any: */
   problem = 0;
   for (ii = 0; ii < (int)IcsStruct->Dimensions; ii++) {
      problem |= *(IcsStruct->Dim[ii].Label) == '\0';
   }
   if (!problem) {
      problem = IcsFirstToken (line, ICSTOK_PARAM);
      problem |= IcsAddToken (line, ICSTOK_LABELS);
      problem |= IcsAddText (line, ICS_LABEL_BITS);
      for (ii = 0; ii < IcsStruct->Dimensions-1; ii++) {
         problem |= IcsAddText (line, IcsStruct->Dim[ii].Label);
      }
      problem |= IcsAddLastText (line, IcsStruct->Dim[ii].Label);
      ICSTR( problem, IcsErr_FailWriteLine );
      ICSXR( IcsAddLine (line, fp) );
   }

   return error;
}

static Ics_Error WriteIcsSensorData (Ics_Header* IcsStruct, FILE* fp)
{
   ICSINIT;
   int problem, ii, chans;
   char line[ICS_LINE_LENGTH];

   if (IcsStruct->WriteSensor) {

      chans = IcsStruct->SensorChannels;
      ICSTR( chans > ICS_MAX_LAMBDA, IcsErr_TooManyChans );

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_TYPE);
      problem |= IcsAddLastText (line, IcsStruct->Type);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_MODEL);
      problem |= IcsAddLastText (line, IcsStruct->Model);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_CHANS);
      problem |= IcsAddLastInt (line, chans);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_PINHRAD);
      for (ii = 0; ii < chans-1; ii++) {
         problem |= IcsAddDouble (line, IcsStruct->PinholeRadius[ii]);
      }
      problem |= IcsAddLastDouble (line, IcsStruct->PinholeRadius[chans-1]);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_LAMBDEX);
      for (ii = 0; ii < chans-1; ii++) {
         problem |= IcsAddDouble (line, IcsStruct->LambdaEx[ii]);
      }
      problem |= IcsAddLastDouble (line, IcsStruct->LambdaEx[chans-1]);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_LAMBDEM);
      for (ii = 0; ii < chans-1; ii++) {
         problem |= IcsAddDouble (line, IcsStruct->LambdaEm[ii]);
      }
      problem |= IcsAddLastDouble (line, IcsStruct->LambdaEm[chans-1]);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_REFRIME);
      problem |= IcsAddLastDouble (line, IcsStruct->RefrInxMedium);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_NUMAPER);
      problem |= IcsAddLastDouble (line, IcsStruct->NumAperture);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_REFRILM);
      problem |= IcsAddLastDouble (line, IcsStruct->RefrInxLensMedium);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

      problem = IcsFirstToken (line, ICSTOK_SENSOR);
      problem |= IcsAddToken (line, ICSTOK_SPARAMS);
      problem |= IcsAddToken (line, ICSTOK_PINHSPA);
      problem |= IcsAddLastDouble (line, IcsStruct->PinholeSpacing);
      if (!problem) {
         ICSXR( IcsAddLine (line, fp) );
      }

   }

   return error;
}

static Ics_Error WriteIcsHistory (Ics_Header* IcsStruct, FILE* fp)
{
   ICSINIT;
   int problem, ii;
   char line[ICS_LINE_LENGTH];
   Ics_History* hist = (Ics_History*)IcsStruct->History;

   if (hist != NULL) {
      for (ii = 0; ii < hist->NStr; ii++) {
         if (hist->Strings[ii] != NULL) {
            problem = IcsFirstToken (line, ICSTOK_HISTORY);
            problem |= IcsAddLastText (line, hist->Strings[ii]);
            if (!problem) {
               ICSXR( IcsAddLine (line, fp) );
            }
         }
      }
   }

   return error;
}

static Ics_Error MarkEndOfFile (Ics_Header* IcsStruct, FILE* fp)
{
   ICSINIT;
   char line[ICS_LINE_LENGTH];

   if ((IcsStruct->Version >= 2) && (IcsStruct->SrcFile[0] == '\0')) {
      error = IcsFirstToken (line, ICSTOK_END);
      ICSTR( error, IcsErr_FailWriteLine );
      IcsAppendChar (line, ICS_EOL);
      ICSXR( IcsAddLine (line, fp) );
   }

   return error;
}

Ics_Error IcsWriteIcs (Ics_Header* IcsStruct, char const* filename)
{
   ICSDECL;
   ICS_INIT_LOCALE;
   char line[ICS_LINE_LENGTH];
   char buf[ICS_MAXPATHLEN];
   FILE* fp;

   if ((filename != NULL) && (filename[0] != '\0')) {
      IcsGetIcsName (IcsStruct->Filename, filename, 0);
   }
   else if (IcsStruct->Filename[0] != '\0') {
      IcsStrCpy (buf, IcsStruct->Filename, ICS_MAXPATHLEN);
      IcsGetIcsName (IcsStruct->Filename, buf, 0);
   }
   else {
      return IcsErr_FOpenIcs;
   }

   fp = fopen (IcsStruct->Filename, "wb");
   ICSTR( fp == NULL, IcsErr_FOpenIcs );

   ICS_SET_LOCALE;

   line[0] = ICS_FIELD_SEP;
   line[1] = ICS_EOL;
   line[2] = '\0';
   error = IcsAddLine (line, fp);

   /* Which ICS version is this file? */
   if (!error) {
      IcsFirstText (line, ICS_VERSION);
      if (IcsStruct->Version == 1) {
         IcsAddLastText (line, "1.0");
      }
      else {
         IcsAddLastText (line, "2.0");
      }
      ICSCX( IcsAddLine (line, fp) );
   }

   /* Write the root of the filename: */
   if (!error) {
      IcsGetFileName (buf, IcsStruct->Filename);
      IcsFirstText (line, ICS_FILENAME);
      IcsAddLastText (line, buf);
      ICSCX( IcsAddLine (line, fp) );
   }

   /* Write all image descriptors: */
   ICSCX( WriteIcsSource (IcsStruct, fp) );
   ICSCX( WriteIcsLayout (IcsStruct, fp) );
   ICSCX( WriteIcsRep (IcsStruct, fp) );
   ICSCX( WriteIcsParam (IcsStruct, fp) );
   ICSCX( WriteIcsSensorData (IcsStruct, fp) );
   ICSCX( WriteIcsHistory (IcsStruct, fp) );
   ICSCX( MarkEndOfFile (IcsStruct, fp) );

   ICS_REVERT_LOCALE;

   if (fclose (fp) == EOF) {
      ICSCX (IcsErr_FCloseIcs); /* Don't overwrite any previous error. */
   }
   return error;
}
