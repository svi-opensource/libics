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
 * FILE : libics_history.c
 *
 * The following library functions are contained in this file:
 *
 *   IcsAddHistoryString()
 *   IcsGetNumHistoryStrings()
 *   IcsNewHistoryIterator()
 *   IcsGetHistoryString()
 *   IcsGetHistoryKeyValue()
 *   IcsGetHistoryStringI()
 *   IcsGetHistoryKeyValueI()
 *   IcsDeleteHistory()
 *   IcsDeleteHistoryStringI()
 *   IcsReplaceHistoryStringI()
 *   IcsFreeHistory()
 *
 * The following internal functions are contained in this file:
 *
 *   IcsInternAddHistory()
 */
/*
 * The void* History in the ICS struct is a pointer to a struct defined in
 * libics_intern.h. Only the functions in this file and two others elsewhere
 * meddle with this structure [IcsPrintIcs() in libics_test.c and WriteIcsHistory()
 * in libics_write.c]. (I guess these two others should start using the functions
 * defined here.)
 *
 * This struct contains an array of strings. The struct and the array are allocated
 * when first adding a string, and the array is reallocated when it becomes too small.
 * The array grows in increments of ICS_HISTARRAY_INCREMENT, and it's length is given
 * by the struct element Lenth. Each array element up to NStr is either NULL or a
 * pointer to a char[]. These are allocated and deallocated when adding or deleting
 * the strings. When deleting a string, the array element is set to NULL. It is not
 * possible to move the other array elements down because that could invalidate
 * iterators. IcsFreeHistory() frees all of these strings, the array and the struct,
 * leaving the History pointer in the ICS struct as NULL.
 */

#include <stdlib.h>
#include <string.h>
#include "libics_intern.h"

/*
 * Add HISTORY line to the ICS file. key can be NULL.
 */
Ics_Error IcsAddHistoryString (ICS* ics, char const* key, char const* value)
{
   ICSDECL;
   static char const seps[3] = {ICS_FIELD_SEP,ICS_EOL,'\0'};

   ICS_FM_WMD( ics );

   if (key == NULL) {
      key = "";
   }
   error = IcsInternAddHistory (ics, key, value, seps);

   return error;
}

/*
 * Add HISTORY lines to the ICS file (key can be "", value shouldn't).
 */
Ics_Error IcsInternAddHistory (Ics_Header* ics, char const* key, char const* value,
                               char const* seps)
{
   ICSINIT;
   size_t len;
   char* line;
   Ics_History* hist;

   /* Checks */
   len = strlen (key) + strlen (value) + 2;
      /* Length of { key + '\t' + value + '\0' } */
   ICSTR( strlen (ICS_HISTORY) + len + 2 > ICS_LINE_LENGTH, IcsErr_LineOverflow );
      /* Length of { "history" + '\t' + key + '\t' + value + '\n' + '\0' } */
   ICSTR( strchr (key, ICS_FIELD_SEP) != NULL, IcsErr_IllParameter ); /* defualt keyword separator */
   ICSTR( strchr (key,   seps[0])     != NULL, IcsErr_IllParameter ); /* chosen keyword separator */
   ICSTR( strchr (key,   seps[1]) != NULL, IcsErr_IllParameter ); /* chosen line separator */
   ICSTR( strchr (key,   ICS_EOL) != NULL, IcsErr_IllParameter ); /* default line separator */
   ICSTR( strchr (key,   '\n')    != NULL, IcsErr_IllParameter ); /* possible line separator */
   ICSTR( strchr (key,   '\r')    != NULL, IcsErr_IllParameter ); /* possible line separator */
   ICSTR( strchr (value, seps[1]) != NULL, IcsErr_IllParameter ); /* chosen line separator */
   ICSTR( strchr (value, ICS_EOL) != NULL, IcsErr_IllParameter ); /* default line separator */
   ICSTR( strchr (value, '\n')    != NULL, IcsErr_IllParameter ); /* possible line separator */
   ICSTR( strchr (value, '\r')    != NULL, IcsErr_IllParameter ); /* possible line separator */

   /* Allocate array if necessary */
   if (ics->History == NULL) {
      ics->History = malloc (sizeof (Ics_History));
      ICSTR( ics->History == NULL, IcsErr_Alloc );
      hist = (Ics_History*)ics->History;
      hist->Strings = (char**)malloc (ICS_HISTARRAY_INCREMENT*sizeof (char*));
      if (hist->Strings == NULL) {
         free (ics->History);
         ics->History = NULL;
         return IcsErr_Alloc;
      }
      hist->Length = ICS_HISTARRAY_INCREMENT;
      hist->NStr = 0;
   }
   else {
      hist = (Ics_History*)ics->History;
   }
   /* Reallocate if array is not large enough */
   if (hist->NStr >= hist->Length) {
      char** tmp = (char**)realloc (hist->Strings, (hist->Length+ICS_HISTARRAY_INCREMENT)*sizeof (char*));
      ICSTR( tmp == NULL, IcsErr_Alloc );
      hist->Strings = tmp;
      hist->Length += ICS_HISTARRAY_INCREMENT;
   }

   /* Create line */
   line = (char*)malloc (len * sizeof (char));
   ICSTR( line == NULL, IcsErr_Alloc );
   if (key[0] != '\0') {
      strcpy (line, key); /* already tested length */
      IcsAppendChar (line, ICS_FIELD_SEP);
   }
   else {
      line[0] = '\0';
   }
   strcat (line, value);
   /* Convert seps[0] into ICS_FIELD_SEP */
   if (seps[0] != ICS_FIELD_SEP) {
      char* s;
      while ((s = strchr (line, seps[0])) != NULL) {
         s[0] = ICS_FIELD_SEP;
      }
   }

   /* Put line into array */
   hist->Strings[hist->NStr] = line;
   hist->NStr++;

   return error;
}

/*
 * Get the number of HISTORY lines from the ICS file.
 */
Ics_Error IcsGetNumHistoryStrings (ICS* ics, int* num)
{
   ICSINIT;
   int ii, count = 0;
   Ics_History* hist = (Ics_History*)ics->History;

   ICS_FM_RMD( ics );

   *num = 0;
   ICSTR( hist == NULL, IcsErr_Ok );
   for (ii = 0; ii < hist->NStr; ii++) {
      if (hist->Strings[ii] != NULL) {
         count++;
      }
   }
   *num = count;

   return error;
}

/*
 * Finds next matching string in history.
 */
static void IcsIteratorNext (Ics_History* hist, Ics_HistoryIterator* it)
{
   int nchar = strlen (it->key);
   it->previous = it->next;
   it->next++;
   if (nchar > 0) {
      for ( ; it->next < hist->NStr; it->next++) {
         if ((hist->Strings[it->next] != NULL) &&
             (strncmp (it->key, hist->Strings[it->next], nchar) == 0)) {
            break;
         }
      }
   }
   if (it->next >= hist->NStr) {
      it->next = -1;
   }
}

/*
 * Initializes history iterator. key can be NULL.
 */
Ics_Error IcsNewHistoryIterator (ICS* ics, Ics_HistoryIterator* it, char const* key)
{
   ICSINIT;
   Ics_History* hist = (Ics_History*)ics->History;

   ICS_FM_RMD( ics );

   it->next = -1;
   it->previous = -1;
   if ((key == NULL) || (key[0] == '\0')) {
      it->key[0] = '\0';
   }
   else {
      int n;
      IcsStrCpy (it->key, key, ICS_STRLEN_TOKEN);
      /* Append a \t, so that the search for the key only finds whole words */
      n = strlen(it->key);
      it->key[n] = ICS_FIELD_SEP;
      it->key[n+1] = '\0';
   }

   ICSTR( hist == NULL, IcsErr_EndOfHistory );

   IcsIteratorNext (hist, it);
   ICSTR( it->next < 0, IcsErr_EndOfHistory );

   return error;
}

/*
 * Get HISTORY lines from the ICS file. history must have at least ICS_LINE_LENGTH
 * characters allocated.
 */
static Ics_HistoryIterator intern_iter = {-1,-1,{'\0'}};  /* used in this and the next function */
Ics_Error IcsGetHistoryString (ICS* ics, char* string, Ics_HistoryWhich which)
{
   ICSINIT;

   ICS_FM_RMD( ics );

   if (which == IcsWhich_First) {
      ICSXR( IcsNewHistoryIterator (ics, &intern_iter, NULL) );
   }
   ICSXR( IcsGetHistoryStringI (ics, &intern_iter, string) );

   return error;
}

/*
 * Get history line from the ICS file as key/value pair. key must have
 * ICS_STRLEN_TOKEN characters allocated, and value ICS_LINE_LENGTH. key can be
 * null, token will be discarded.
 */
Ics_Error IcsGetHistoryKeyValue (ICS* ics, char* key, char* value, Ics_HistoryWhich which)
{
   ICSINIT;

   ICS_FM_RMD( ics );

   if (which == IcsWhich_First) {
      ICSXR( IcsNewHistoryIterator (ics, &intern_iter, NULL) );
   }
   ICSXR( IcsGetHistoryKeyValueI (ics, &intern_iter, key, value) );

   return error;
}


/*
 * Get history line from the ICS file using iterator. string must have at
 * least ICS_LINE_LENGTH characters allocated.
 */
Ics_Error IcsGetHistoryStringI (ICS* ics, Ics_HistoryIterator* it, char* string)
{
   ICSINIT;
   Ics_History* hist = (Ics_History*)ics->History;

   ICS_FM_RMD( ics );

   ICSTR( hist == NULL, IcsErr_EndOfHistory );
   if ((it->next >= 0) && (hist->Strings[it->next] == NULL)) {
      /* The string pointed to has been deleted.
       * Find the next string, but don't change prev! */
      int prev = it->previous;
      IcsIteratorNext (hist, it);
      it->previous = prev;
   }
   ICSTR( it->next < 0, IcsErr_EndOfHistory );
   IcsStrCpy (string, hist->Strings[it->next], ICS_LINE_LENGTH);
   IcsIteratorNext (hist, it);

   return error;
}

/*
 * Get history line from the ICS file as key/value pair using iterator.
 * key must have ICS_STRLEN_TOKEN characters allocated, and value
 * ICS_LINE_LENGTH. key can be null, token will be discarded.
 */
Ics_Error IcsGetHistoryKeyValueI (ICS* ics, Ics_HistoryIterator* it, char* key, char* value)
{
   ICSINIT;
   size_t length;
   char buf[ICS_LINE_LENGTH];
   char *ptr;

   ICSXR( IcsGetHistoryStringI (ics, it, buf) );

   ptr = strchr (buf, ICS_FIELD_SEP);
   length = ptr-buf;
   if ((ptr != NULL) && (length>0) && (length<ICS_STRLEN_TOKEN)) {
      if (key != NULL) {
         memcpy (key, buf, length);
         key[length] = '\0';
      }
      IcsStrCpy (value, ptr+1, ICS_LINE_LENGTH);
   }
   else {
      if (key != NULL) {
         key[0] = '\0';
      }
      IcsStrCpy (value, buf, ICS_LINE_LENGTH);
   }

   return error;
}

/*
 * Delete all history lines with key from ICS file. key can be NULL,
 * deletes all.
 */
Ics_Error IcsDeleteHistory (ICS* ics, char const* key)
{
   ICSINIT;
   Ics_History* hist = (Ics_History*)ics->History;
   ICSTR( hist == NULL, IcsErr_Ok );
   ICSTR( hist->NStr == 0, IcsErr_Ok );

   ICS_FM_RMD( ics );

   if ((key == NULL) || (key[0] == '\0')) {
      int ii;
      for (ii = 0; ii < hist->NStr; ii++) {
         if (hist->Strings[ii] != NULL) {
            free (hist->Strings[ii]);
            hist->Strings[ii] = NULL;
         }
      }
      hist->NStr = 0;
   }
   else {
      Ics_HistoryIterator it;
      IcsNewHistoryIterator (ics, &it, key);
      if (it.next >= 0) {
         IcsIteratorNext (hist, &it);
      }
      while (it.previous >= 0) {
         free (hist->Strings[it.previous]);
         hist->Strings[it.previous] = NULL;
         IcsIteratorNext (hist, &it);
      }
      /* If we just deleted strings at the end of the array, recover those spots... */
      hist->NStr--;
      while ((hist->NStr >= 0) && (hist->Strings[hist->NStr] == NULL)) {
         hist->NStr--;
      }
      hist->NStr++;
   }

   return error;
}

/*
 * Delete last retrieved history line (iterator still points to the same string).
 */
Ics_Error IcsDeleteHistoryStringI (ICS* ics, Ics_HistoryIterator* it)
{
   ICSINIT;
   Ics_History* hist = (Ics_History*)ics->History;

   ICS_FM_RMD( ics );

   ICSTR( hist == NULL, IcsErr_Ok );      /* give error message? */
   ICSTR( it->previous < 0, IcsErr_Ok);
   ICSTR( hist->Strings[it->previous] == NULL,  IcsErr_Ok);

   free (hist->Strings[it->previous]);
   hist->Strings[it->previous] = NULL;
   if (it->previous == hist->NStr-1) {
      /* We just deleted the last string. Let's recover that spot... */
      hist->NStr--;
   }
   it->previous = -1;

   return error;
}

/*
 * Delete last retrieved history line (iterator still points to the same string).
 * Contains code duplicated from IcsInternAddHistory().
 */
Ics_Error IcsReplaceHistoryStringI (ICS* ics, Ics_HistoryIterator* it,
                                    char const* key, char const* value)
{
   ICSINIT;
   size_t len;
   char* line;
   Ics_History* hist = (Ics_History*)ics->History;

   ICS_FM_RMD( ics );

   ICSTR( hist == NULL, IcsErr_Ok );      /* give error message? */
   ICSTR( it->previous < 0, IcsErr_Ok);
   ICSTR( hist->Strings[it->previous] == NULL,  IcsErr_Ok);

   /* Checks */
   len = strlen (key) + strlen (value) + 2;
      /* Length of { key + '\t' + value + '\0' } */
   ICSTR( strlen (ICS_HISTORY) + len + 2 > ICS_LINE_LENGTH, IcsErr_LineOverflow );
      /* Length of { "history" + '\t' + key + '\t' + value + '\n' + '\0' } */
   ICSTR( strchr (key, ICS_FIELD_SEP) != NULL, IcsErr_IllParameter ); /* defualt keyword separator */
   ICSTR( strchr (key,   ICS_EOL) != NULL, IcsErr_IllParameter ); /* default line separator */
   ICSTR( strchr (key,   '\n')    != NULL, IcsErr_IllParameter ); /* possible line separator */
   ICSTR( strchr (key,   '\r')    != NULL, IcsErr_IllParameter ); /* possible line separator */
   ICSTR( strchr (value, ICS_EOL) != NULL, IcsErr_IllParameter ); /* default line separator */
   ICSTR( strchr (value, '\n')    != NULL, IcsErr_IllParameter ); /* possible line separator */
   ICSTR( strchr (value, '\r')    != NULL, IcsErr_IllParameter ); /* possible line separator */

   /* Create line */
   line = (char*)realloc (hist->Strings[it->previous], len * sizeof (char));
   ICSTR( line == NULL, IcsErr_Alloc );
   hist->Strings[it->previous] = line;
   if (key[0] != '\0') {
      strcpy (line, key); /* already tested length */
      IcsAppendChar (line, ICS_FIELD_SEP);
   }
   strcat (line, value);

   return error;
}

/*
 * Free the memory allocated for history.
 */
void IcsFreeHistory (Ics_Header* ics)
{
   int ii;
   Ics_History* hist = (Ics_History*)ics->History;
   if (hist != NULL) {
      for (ii = 0; ii < hist->NStr; ii++) {
         if (hist->Strings[ii] != NULL) {
            free (hist->Strings[ii]);
         }
      }
      free (hist->Strings);
      free (ics->History);
      ics->History = NULL;
   }
}
