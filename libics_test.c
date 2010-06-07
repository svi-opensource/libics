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
 * FILE : libics_test.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsPrintIcs()
 *   IcsPrintError()
 */

#include <stdlib.h>
#include <stdio.h>
#include "libics_intern.h"
#include "libics_test.h"

void IcsPrintIcs (ICS const* ics)
{
   int p, ii;
   Ics_Format Format;
   int Sign;
   size_t Bits;
   char* s;

   IcsGetPropsDataType (ics->Imel.DataType, &Format, &Sign, &Bits);
   p = ics->Dimensions;
   printf ("Version: %d\n", ics->Version);
   printf ("FileMode: %d\n", ics->FileMode);
   printf ("Filename: %s\n", ics->Filename);
   printf ("SrcFile: %s\n", ics->SrcFile);
   printf ("SrcOffset: %ld\n", (long int)ics->SrcOffset);
   printf ("Data: %p\n", ics->Data);
   printf ("DataLength: %ld\n", (long int)ics->DataLength);
   printf ("Parameters: %d\n", ics->Dimensions+1);
   printf ("Order: bits ");
   for (ii=0; ii<p; ii++)
      printf ("%s ", ics->Dim[ii].Order);
   printf ("\n");
   printf ("Sizes: %d ", (int)Bits);
   for (ii=0; ii<p; ii++)
      printf ("%lu ", (unsigned long) ics->Dim[ii].Size);
   printf ("\n");
   printf ("Sigbits: %d\n", (int)ics->Imel.SigBits);
   printf ("Origin: %f ", ics->Imel.Origin);
   for (ii=0; ii<p; ii++)
      printf ("%f ", ics->Dim[ii].Origin);
   printf ("\n");
   printf ("Scale: %f ", ics->Imel.Scale);
   for (ii=0; ii<p; ii++)
      printf ("%f ", ics->Dim[ii].Scale);
   printf ("\n");
   printf ("Labels: intensity ");
   for (ii=0; ii<p; ii++)
      printf ("%s ", ics->Dim[ii].Label);
   printf ("\n");
   printf ("Units: %s ", ics->Imel.Unit);
   for (ii=0; ii<p; ii++)
      printf ("%s ", ics->Dim[ii].Unit);
   printf ("\n");
   switch (Format) {
      case IcsForm_real:
         s = "real";
         break;
      case IcsForm_complex:
         s = "complex";
         break;
      default:
         s = "integer";
   }
   printf ("Format: %s\n", s);
   printf ("Sign: %s\n", Sign?"signed":"unsigned");
   printf ("SCIL_TYPE: %s\n", ics->ScilType);
   printf ("Coordinates: %s\n", ics->Coord);
   switch (ics->Compression) {
      case IcsCompr_uncompressed:
         s = "uncompressed";
         break;
      case IcsCompr_compress:
         s = "compress";
         break;
      case IcsCompr_gzip:
         s = "gzip";
         break;
      default:
         s = "unknown";
   }
   printf ("Compression: %s (level %d)\n", s, ics->CompLevel);
   printf ("Byteorder: ");
   for (ii=0; ii<ICS_MAX_IMEL_SIZE; ii++)
      if (ics->ByteOrder[ii] != 0)
         printf ("%d ", ics->ByteOrder[ii]);
      else
         break;
   printf ("\n");
   printf ("BlockRead: %p\n", ics->BlockRead);
   if (ics->BlockRead != NULL) {
      Ics_BlockRead* br = (Ics_BlockRead*)ics->BlockRead;
      printf ("   DataFilePtr: %p\n", (void*)br->DataFilePtr);
#ifdef ICS_ZLIB
      printf ("   ZlibStream: %p\n", br->ZlibStream);
      printf ("   ZlibInputBuffer: %p\n", br->ZlibInputBuffer);
#endif
   }
   printf ("Sensor data: \n");
   printf ("   Sensor type: %s\n", ics->Type);
   printf ("   Sensor model: %s\n", ics->Model);
   printf ("   SensorChannels: %d\n", ics->SensorChannels);
   printf ("   RefrInxMedium: %f\n", ics->RefrInxMedium);
   printf ("   NumAperture: %f\n", ics->NumAperture);
   printf ("   RefrInxLensMedium: %f\n", ics->RefrInxLensMedium);
   printf ("   PinholeSpacing: %f\n", ics->PinholeSpacing);
   printf ("   PinholeRadius: ");
   for (ii = 0; ii < ICS_MAX_LAMBDA && ii < ics->SensorChannels; ++ii) {
      printf ("%f ", ics->PinholeRadius[ii]);
   }
   printf ("\n");
   printf ("   LambdaEx: ");
   for (ii = 0; ii < ICS_MAX_LAMBDA && ii < ics->SensorChannels; ++ii) {
      printf ("%f ", ics->LambdaEx[ii]);
   }
   printf ("\n");
   printf ("   LambdaEm: ");
   for (ii = 0; ii < ICS_MAX_LAMBDA && ii < ics->SensorChannels; ++ii) {
      printf ("%f ", ics->LambdaEm[ii]);
   }
   printf ("\n");
   printf ("   ExPhotonCnt: ");
   for (ii = 0; ii < ICS_MAX_LAMBDA && ii < ics->SensorChannels; ++ii) {
      printf ("%d ", ics->ExPhotonCnt[ii]);
   }
   printf ("\n");
   printf ("History Lines:\n");
   if (ics->History != NULL) {
      Ics_History* hist = (Ics_History*)ics->History;
      for (ii = 0; ii < hist->NStr; ii++) {
         if (hist->Strings[ii] != NULL) {
            printf ("   %s\n", hist->Strings[ii]);
         }
      }
   }
}

void IcsPrintError (Ics_Error error)
{
    char const* msg;

    msg = IcsGetErrorText (error);
    printf ("libics error: %s.\n", msg);
}

