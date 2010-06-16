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
 * FILE : libics_read.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsReadIcs()
 *   IcsVersion()
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libics_intern.h"

/*
 * Find the index for "bits", which should be the first parameter...
 */
static int IcsGetBitsParam (char Order[ICS_MAXDIM+1][ICS_STRLEN_TOKEN], int Parameters)
{
   int ii;

   for (ii = 0; ii < Parameters; ii++) {
      if (strcmp (Order[ii], ICS_ORDER_BITS) == 0) {
         return ii;
      }
   }

   return -1;
}

/*
 * Like fgets(), gets a string from a stream. However, does not stop at
 * newline character, but at 'sep'. It retains the 'sep' character at the end
 * of the string; a null byte is appended.
 * Also, it implements the solution to the CR/LF pair problem caused by
 * some windows applications. If 'sep' is LF, it might be prepended by a CR.
 */
static char* IcsFGetStr (char* line, int n, FILE* fi, char sep)
{
   /* n == ICS_LINE_LENGTH */
   int ii = 0;
   int ch;

   /* Imitate fgets() */
   while (ii < n-1) {
      ch = getc (fi);
      if (ch == EOF)
         break;

      /* Skip CR if next char is LF and sep is LF. */
      if (ch == '\r' && sep == '\n') {
         ch = getc (fi);
         if (ch != sep && ch != EOF) {
            ungetc (ch, fi);
            ch = '\r';
         }
      }

      line[ii] = (char)ch;
      ii++;
      if ((char)ch == sep)
         break;
   }
   line[ii] = '\0';
   if (ii != 0) {
      /* Read at least a 'sep' character */
      return line;
   }
   else
      /* EOF at first getc() call */
      return NULL;
}

/*
 * Read the two ICS separators from file. There is a special case for ICS
 * headers which are erroneously written under Windows in text mode causing
 * a newline separator to be prepended by a carriage return.  Therefore when
 * the second separator is a carriage return and the first separator is not
 * a newline then peek at the third character to see if it is a newline.  If
 * so then use newline as the second separator. Return IcsErr_FReadIcs on
 * read errors and IcsErr_NotIcsFile on premature end-of-file.
 */
static Ics_Error GetIcsSeparators (FILE* fi, char* seps)
{
   int sep1;
   int sep2;
   int sep3;

   sep1 = fgetc (fi);
   if (sep1 == EOF) {
      return (ferror (fi)) ? IcsErr_FReadIcs : IcsErr_NotIcsFile;
   }
   sep2 = fgetc (fi);
   if (sep2 == EOF) {
      return (ferror (fi)) ? IcsErr_FReadIcs : IcsErr_NotIcsFile;
   }
   if (sep1 == sep2) {
      return IcsErr_NotIcsFile;
   }
   if (sep2 == '\r' && sep1 != '\n') {
      sep3 = fgetc (fi);
      if (sep3 == EOF) {
         return (ferror (fi)) ? IcsErr_FReadIcs : IcsErr_NotIcsFile;
      } else {
         if (sep3 == '\n') {
            sep2 = '\n';
         } else {
            ungetc (sep3, fi);
         }
      }
   }
   seps[0] = sep1;
   seps[1] = sep2;
   seps[2] = '\0';

   return IcsErr_Ok;
}

static Ics_Error GetIcsVersion (FILE* fi, char const* seps, int* ver)
{
   ICSINIT;
   char* word;
   char line[ICS_LINE_LENGTH];

   ICSTR( IcsFGetStr (line, ICS_LINE_LENGTH, fi, seps[1]) == NULL, IcsErr_FReadIcs );
   word = strtok (line, seps);
   ICSTR( word == NULL, IcsErr_NotIcsFile );
   ICSTR( strcmp (word, ICS_VERSION) != 0, IcsErr_NotIcsFile );
   word = strtok (NULL , seps);
   ICSTR( word == NULL, IcsErr_NotIcsFile );
   if (strcmp (word, "1.0") == 0) {
      *ver = 1;
   }
   else if (strcmp (word, "2.0") == 0) {
      *ver = 2;
   }
   else {
      error = IcsErr_NotIcsFile;
   }

   return error;
}

static Ics_Error GetIcsFileName (FILE* fi, char const* seps)
{
   ICSINIT;
   char* word;
   char line[ICS_LINE_LENGTH];

   ICSTR( IcsFGetStr (line, ICS_LINE_LENGTH, fi, seps[1]) == NULL, IcsErr_FReadIcs );
   word = strtok (line, seps);
   ICSTR( word == NULL, IcsErr_NotIcsFile );
   ICSTR( strcmp (word, ICS_FILENAME) != 0, IcsErr_NotIcsFile );
   /* The rest of the line is discarded */

   return error;
}

static Ics_Token GetIcsToken (char* str, Ics_SymbolList* ListSpec)
{
   int ii;
   Ics_Token token = ICSTOK_NONE;

   if (str != NULL) {
      for (ii = 0; ii < ListSpec->Entries; ii++) {
         if (strcmp (ListSpec->List[ii].Name, str) == 0) {
            token = ListSpec->List[ii].Token;
         }
      }
   }

   return token;
}

static Ics_Error GetIcsCat (char* str, char const* seps, Ics_Token* Cat,
                            Ics_Token* SubCat, Ics_Token* SubSubCat)
{
   ICSINIT;
   char* token, buffer[ICS_LINE_LENGTH];
   *SubCat = *SubSubCat = ICSTOK_NONE;

   IcsStrCpy (buffer, str, ICS_LINE_LENGTH);
   token = strtok (buffer, seps);
   *Cat = GetIcsToken (token, &G_Categories);
   ICSTR( *Cat == ICSTOK_NONE, IcsErr_MissCat );
   if ((*Cat != ICSTOK_HISTORY) && (*Cat != ICSTOK_END)) {
      token = strtok (NULL, seps);
      *SubCat = GetIcsToken (token, &G_SubCategories);
      ICSTR( *SubCat == ICSTOK_NONE, IcsErr_MissSubCat );
      if (*SubCat == ICSTOK_SPARAMS) {
         token = strtok (NULL, seps);
         *SubSubCat = GetIcsToken (token, &G_SubSubCategories);
         ICSTR( *SubSubCat == ICSTOK_NONE , IcsErr_MissSensorSubSubCat );
      }
   }

   /* Copy the remaining stuff into 'str' */
   if ((token = strtok (NULL, seps)) != NULL) {
      strcpy (str, token);
   }
   while ((token = strtok (NULL, seps)) != NULL) {
      IcsAppendChar (str, seps[0]);
      strcat (str, token);
   }

   return error;
}

Ics_Error IcsReadIcs (Ics_Header* IcsStruct, char const* filename, int forcename, int forcelocale)
{
   ICSDECL;
   ICS_INIT_LOCALE;
   FILE* fp;
   int end = 0, ii, jj, bits;
   char seps[3], *ptr, *data;
   char line[ICS_LINE_LENGTH];
   Ics_Token cat, subCat, subSubCat;
   /* These are temporary buffers to hold the data read until it is
    * copied to the Ics_Header structure. This is needed because the
    * Ics_Header structure is made to look more like we like to see
    * images, compared to the way the data is written in the ICS file.
    */
   Ics_Format Format = IcsForm_unknown;
   int Sign = 1;
   int Parameters = 0;
   char Order[ICS_MAXDIM+1][ICS_STRLEN_TOKEN];
   size_t Sizes[ICS_MAXDIM+1];
   double Origin[ICS_MAXDIM+1];
   double Scale[ICS_MAXDIM+1];
   char Label[ICS_MAXDIM+1][ICS_STRLEN_TOKEN];
   char Unit[ICS_MAXDIM+1][ICS_STRLEN_TOKEN];
   for (ii = 0; ii < ICS_MAXDIM+1; ii++)
   {
      Sizes[ii] = 1;
      Origin[ii] = 0.0;
      Scale[ii] = 1.0;
      Order[ii][0] = '\0';
      Label[ii][0] = '\0';
      Unit[ii][0] = '\0';
   }

   IcsInit (IcsStruct);
   IcsStruct->FileMode = IcsFileMode_read;

   IcsStrCpy (IcsStruct->Filename, filename, ICS_MAXPATHLEN);
   ICSXR( IcsOpenIcs (&fp, IcsStruct->Filename, forcename) );

   if (forcelocale) {
      ICS_SET_LOCALE;
   }

   ICSCX( GetIcsSeparators (fp, seps) );

   ICSCX( GetIcsVersion (fp, seps, &(IcsStruct->Version)) );
   ICSCX( GetIcsFileName (fp, seps) );

   while (!end && !error && (IcsFGetStr (line, ICS_LINE_LENGTH, fp, seps[1]) != NULL)) {
      if (GetIcsCat (line, seps, &cat, &subCat, &subSubCat) != IcsErr_Ok)
         continue;
      ptr = strtok (line, seps);
      ii = 0;
      switch (cat) {
         case ICSTOK_END:
            end = 1;
            if (IcsStruct->SrcFile[0] == '\0') {
               IcsStruct->SrcOffset = (size_t) ftell (fp);
               IcsStrCpy (IcsStruct->SrcFile, IcsStruct->Filename, ICS_MAXPATHLEN);
            }
            break;
         case ICSTOK_SOURCE:
            switch (subCat) {
               case ICSTOK_FILE:
                  if (ptr != NULL) {
                     IcsStrCpy (IcsStruct->SrcFile, ptr, ICS_MAXPATHLEN);
                  }
                  break;
               case ICSTOK_OFFSET:
                  if (ptr != NULL) {
                     IcsStruct->SrcOffset = IcsStrToSize (ptr);
                  }
                  break;
               default:
                  break;
            }
            break;
         case ICSTOK_LAYOUT:
            switch (subCat) {
               case ICSTOK_PARAMS:
                  if (ptr != NULL) {
                     Parameters = atoi (ptr);
                     if (Parameters > ICS_MAXDIM+1) {
                        error = IcsErr_TooManyDims;
                     }
                  }
                  break;
               case ICSTOK_ORDER:
                  while (ptr!= NULL && ii < ICS_MAXDIM+1) {
                     IcsStrCpy (Order[ii++], ptr, ICS_STRLEN_TOKEN);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               case ICSTOK_SIZES:
                  while (ptr!= NULL && ii < ICS_MAXDIM+1) {
                     Sizes[ii++] = IcsStrToSize (ptr);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               case ICSTOK_COORD:
                  if (ptr != NULL) {
                     IcsStrCpy (IcsStruct->Coord, ptr, ICS_STRLEN_TOKEN);
                  }
                  break;
               case ICSTOK_SIGBIT:
                  if (ptr != NULL) {
                     IcsStruct->Imel.SigBits = IcsStrToSize (ptr);
                  }
                  break;
               default:
                  error = IcsErr_MissLayoutSubCat;
            }
            break;
         case ICSTOK_REPRES:
            switch (subCat) {
               case ICSTOK_FORMAT:
                  switch (GetIcsToken (ptr, &G_Values)) {
                     case ICSTOK_FORMAT_INTEGER:
                        Format = IcsForm_integer;
                        break;
                     case ICSTOK_FORMAT_REAL:
                        Format = IcsForm_real;
                        break;
                     case ICSTOK_FORMAT_COMPLEX:
                        Format = IcsForm_complex;
                        break;
                     default:
                        Format = IcsForm_unknown;
                  }
                  break;
               case ICSTOK_SIGN:
                  if (GetIcsToken (ptr, &G_Values) == ICSTOK_SIGN_UNSIGNED) {
                     Sign = 0;
                  }
                  else {
                     Sign = 1;
                  }
                  break;
               case ICSTOK_SCILT:
                  if (ptr!= NULL) {
                     IcsStrCpy (IcsStruct->ScilType, ptr, ICS_STRLEN_TOKEN);
                  }
                  break;
               case ICSTOK_COMPR:
                  switch (GetIcsToken (ptr, &G_Values)) {
                     case ICSTOK_COMPR_UNCOMPRESSED:
                        IcsStruct->Compression = IcsCompr_uncompressed;
                        break;
                     case ICSTOK_COMPR_COMPRESS:
                        if (IcsStruct->Version==1) {
                           IcsStruct->Compression = IcsCompr_compress;
                        }
                        else { /* A version 2.0 file never uses COMPRESS, maybe it means GZIP? */
                           IcsStruct->Compression = IcsCompr_gzip;
                        }
                        break;
                     case ICSTOK_COMPR_GZIP:
                        IcsStruct->Compression = IcsCompr_gzip;
                        break;
                     default:
                        error = IcsErr_UnknownCompression;
                  }
                  break;
               case ICSTOK_BYTEO:
                  while (ptr!= NULL && ii < ICS_MAX_IMEL_SIZE) {
                     IcsStruct->ByteOrder[ii++] = atoi (ptr);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               default:
                  error = IcsErr_MissRepresSubCat;
               break;
            }
            break;
         case ICSTOK_PARAM:
            switch (subCat) {
               case ICSTOK_ORIGIN:
                  while (ptr!= NULL && ii < ICS_MAXDIM+1) {
                     Origin[ii++] = atof (ptr);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               case ICSTOK_SCALE:
                  while (ptr!= NULL && ii < ICS_MAXDIM+1) {
                     Scale[ii++] = atof (ptr);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               case ICSTOK_UNITS:
                  while (ptr!= NULL && ii < ICS_MAXDIM+1) {
                     IcsStrCpy (Unit[ii++], ptr, ICS_STRLEN_TOKEN);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               case ICSTOK_LABELS:
                  while (ptr!= NULL && ii < ICS_MAXDIM+1) {
                     IcsStrCpy (Label[ii++], ptr, ICS_STRLEN_TOKEN);
                     ptr = strtok (NULL, seps);
                  }
                  break;
               default:
                  error = IcsErr_MissParamSubCat;
            }
            break;
         case ICSTOK_HISTORY:
            if (ptr != NULL) {
               data = strtok (NULL, seps+1); /* This will get the rest of the line */
               if (data == NULL) { /* data is not allowed to be "", but ptr is */
                  data = ptr;
                  ptr = "";
               }
               /* The next portion is to avoid having IcsInternAddHistory
                * return IcsErr_LineOverflow. */
               ii = strlen (ptr);
               if (ii+1 > ICS_STRLEN_TOKEN) {
                  ptr[ICS_STRLEN_TOKEN-1] = '\0';
                  ii = ICS_STRLEN_TOKEN-1;
               }
               jj = strlen (ICS_HISTORY);
               if ((strlen (data) + ii + jj + 4) > ICS_LINE_LENGTH) {
                  data[ICS_LINE_LENGTH - ii - jj - 4] = '\0';
               }
               error = IcsInternAddHistory (IcsStruct, ptr, data, seps);
            }
            break;
         case ICSTOK_SENSOR:
            switch (subCat) {
               case ICSTOK_TYPE:
                  if (ptr != NULL) {
                     IcsStrCpy (IcsStruct->Type, ptr, ICS_STRLEN_TOKEN);
                  }
                  break;
               case ICSTOK_MODEL:
                  if (ptr != NULL) {
                     IcsStrCpy (IcsStruct->Model, ptr, ICS_STRLEN_OTHER);
                  }
                  break;
               case ICSTOK_SPARAMS:
                  switch (subSubCat) {
                     case ICSTOK_CHANS:
                        if (ptr != NULL) {
                           IcsStruct->SensorChannels = atoi (ptr);
                           if (IcsStruct->SensorChannels > ICS_MAX_LAMBDA) {
                              error = IcsErr_TooManyChans;
                           }
                        }
                        break;
                     case ICSTOK_PINHRAD:
                        while (ptr != NULL && ii < ICS_MAX_LAMBDA) {
                           IcsStruct->PinholeRadius[ii++] = atof (ptr);
                           ptr = strtok (NULL, seps);
                        }
                        break;
                     case ICSTOK_LAMBDEX:
                        while (ptr != NULL && ii < ICS_MAX_LAMBDA) {
                           IcsStruct->LambdaEx[ii++] = atof (ptr);
                           ptr = strtok (NULL, seps);
                        }
                        break;
                     case ICSTOK_LAMBDEM:
                        while (ptr != NULL && ii < ICS_MAX_LAMBDA) {
                           IcsStruct->LambdaEm[ii++] = atof (ptr);
                           ptr = strtok (NULL, seps);
                        }
                        break;
                     case ICSTOK_PHOTCNT:
                        while (ptr != NULL && ii < ICS_MAX_LAMBDA) {
                           IcsStruct->ExPhotonCnt[ii++] = atoi (ptr);
                           ptr = strtok(NULL, seps);
                        }
                        break;
                     case ICSTOK_REFRIME:
                        if (ptr != NULL) {
                           IcsStruct->RefrInxMedium = atof (ptr);
                        }
                        break;
                     case ICSTOK_NUMAPER:
                        if (ptr != NULL) {
                           IcsStruct->NumAperture = atof (ptr);
                        }
                        break;
                     case ICSTOK_REFRILM:
                        if (ptr != NULL) {
                           IcsStruct->RefrInxLensMedium = atof (ptr);
                        }
                        break;
                     case ICSTOK_PINHSPA:
                        if (ptr != NULL) {
                           IcsStruct->PinholeSpacing = atof (ptr);
                        }
                        break;
                     default:
                        error = IcsErr_MissSensorSubSubCat;
                  }
                  break;
               default:
                  error = IcsErr_MissSensorSubCat;
            }
            break;
         default:
            error = IcsErr_MissCat;
      }
   }

   if (!error) {
      bits = IcsGetBitsParam (Order, Parameters);
      if (bits < 0) {
         error = IcsErr_MissBits;
      }
      else {
         IcsGetDataTypeProps (&(IcsStruct->Imel.DataType), Format, Sign, Sizes[bits]);
         for (jj = 0, ii = 0; ii < Parameters; ii++) {
            if (ii == bits) {
               IcsStruct->Imel.Origin = Origin[ii];
               IcsStruct->Imel.Scale = Scale[ii];
               strcpy (IcsStruct->Imel.Unit, Unit[ii]);
            }
            else {
               IcsStruct->Dim[jj].Size = Sizes[ii];
               IcsStruct->Dim[jj].Origin = Origin[ii];
               IcsStruct->Dim[jj].Scale = Scale[ii];
               strcpy (IcsStruct->Dim[jj].Order, Order[ii]);
               strcpy (IcsStruct->Dim[jj].Label, Label[ii]);
               strcpy (IcsStruct->Dim[jj].Unit, Unit[ii]);
               jj++;
            }
         }
         IcsStruct->Dimensions = Parameters - 1;
      }
   }

   if (forcelocale) {
      ICS_REVERT_LOCALE;
   }

   if (fclose (fp) == EOF) {
      ICSCX (IcsErr_FCloseIcs); /* Don't overwrite any previous error. */
   }
   return error;
}

/*
 * Read the first 3 lines of an ICS file to see which version it is.
 * It returns 0 if it is not an ICS file, or the version number if it is.
 */
int IcsVersion (char const* filename, int forcename)
{
   ICSDECL;
   ICS_INIT_LOCALE;
   int version;
   FILE* fp;
   char FileName[ICS_MAXPATHLEN];
   char seps[3];

   IcsStrCpy (FileName, filename, ICS_MAXPATHLEN);
   error = IcsOpenIcs (&fp, FileName, forcename);
   ICSTR( error, 0 );
   version = 0;
   ICS_SET_LOCALE;
   if (!error) error = GetIcsSeparators (fp, seps);
   if (!error) error = GetIcsVersion (fp, seps, &version);
   if (!error) error = GetIcsFileName (fp, seps);
   ICS_REVERT_LOCALE;
   if (fclose (fp) == EOF) {
      return 0;
   }
   if (error)
      return 0;
   else
      return version;
}
