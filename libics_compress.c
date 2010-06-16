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
 * FILE : libics_compress.c
 *
 * The following internal functions are contained in this file:
 *
 *   IcsReadCompress ()
 *
 * This file is based on code from (N)compress 4.2.4.3, written by
 * Spencer W. Thomas, Jim McKie, Steve Davies, Ken Turkowski, James
 * A. Woods, Joe Orost, Dave Mack and Peter Jannesen between 1984 and
 * 1992. The original code is public domain and obtainable from
 * http://ncompress.sourceforge.net/ .
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libics_intern.h"

#define IBUFSIZ ICS_BUF_SIZE  /* Input buffer size */
#define IBUFXTRA 64

/* Defines for third byte of header */
#define MAGIC_1 (unsigned char)'\037'   /* First byte of compressed file */
#define MAGIC_2 (unsigned char)'\235'   /* Second byte of compressed file */
#define BIT_MASK 0x1f   /* Mask for 'number of compresssion bits' */
                        /* Masks 0x20 and 0x40 are free. I think 0x20 should mean 
                           that there is a fourth header byte (for expansion). */
#define BLOCK_MODE 0x80 /* Block compresssion if table is full and compression
                           rate is dropping flush tables */

/* The next two codes should not be changed lightly, as they must not
   lie within the contiguous general code space. */
#define FIRST 257    /* first free entry */
#define CLEAR 256    /* table clear output code */

#define INIT_BITS 9  /* initial number of bits/code */
#define HBITS 17     /* 50% occupancy */
#define HSIZE (1<<HBITS)
#define BITS  16

#define MAXCODE(n)   (1L << (n))

#define input(b,o,c,n,m){ unsigned char *p = &(b)[(o)>>3]; \
   (c) = ((((long)(p[0]))|((long)(p[1])<<8)|((long)(p[2])<<16))>>((o)&0x7))&(m); \
   (o) += (n); \
}

#define tab_prefixof(i)       codetab[i]
#define tab_suffixof(i)       htab[i]
#define de_stack              (&(htab[HSIZE-1]))
#define clear_tab_prefixof()  memset(codetab, 0, 256)

/*
 * Read the full COMPRESS-compressed data stream.
 */
Ics_Error IcsReadCompress (Ics_Header* IcsStruct, void* outbuf, size_t len)
{
   ICSINIT;
   Ics_BlockRead* br = (Ics_BlockRead*)IcsStruct->BlockRead;
   unsigned char *stackp;
   long int code;
   int finchar;
   long int oldcode;
   long int incode;
   int inbits;
   int posbits;
   size_t outpos = 0;
   size_t insize;
   int bitmask;
   long int free_ent;
   long int maxcode;
   long int maxmaxcode;
   int n_bits;
   size_t rsize;
   int block_mode;
   int maxbits;
   int ii;
   int offset;
   unsigned char *inbuf = NULL;
   unsigned char *htab = NULL;
   unsigned short *codetab = NULL;

   /* Dynamically allocate memory that's static in (N)compress. */
   inbuf = (unsigned char*)malloc (IBUFSIZ+IBUFXTRA);
   if (inbuf == NULL) {
      error = IcsErr_Alloc;
      goto error_exit;
   }
   htab = (unsigned char*)malloc (HSIZE*4); /* Not sure about the size of this thing, original code uses a long int array that's cast to char */
   if (htab == NULL) {
      error = IcsErr_Alloc;
      goto error_exit;
   }
   codetab = (unsigned short*)malloc (HSIZE*sizeof(unsigned short));
   if (codetab == NULL) {
      error = IcsErr_Alloc;
      goto error_exit;
   }

   if ((rsize = fread(inbuf, 1, IBUFSIZ, br->DataFilePtr)) <= 0) {
      error = IcsErr_FReadIds;
      goto error_exit;
   }
   insize = rsize;
   if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2) {
      printf("point 1!\n");
      error = IcsErr_CorruptedStream;
      goto error_exit;
   }

   maxbits = inbuf[2] & BIT_MASK;
   block_mode = inbuf[2] & BLOCK_MODE;
   maxmaxcode = MAXCODE(maxbits);
   if (maxbits > BITS) {
      error = IcsErr_DecompressionProblem;
      goto error_exit;
   }

   maxcode = MAXCODE(n_bits = INIT_BITS)-1;
   bitmask = (1<<n_bits)-1;
   oldcode = -1;
   finchar = 0;
   posbits = 3<<3;

   free_ent = ((block_mode) ? FIRST : 256);

   clear_tab_prefixof();   /* As above, initialize the first 256 entries in the table. */
   for (code = 255 ; code >= 0 ; --code) {
      tab_suffixof(code) = (unsigned char)code;
   }

   do {
resetbuf:

      offset = posbits >> 3;
      insize = offset <= insize ? insize - offset : 0;
      for (ii = 0 ; ii < insize ; ++ii) {
         inbuf[ii] = inbuf[ii+offset];
      }
      posbits = 0;

      if (insize < IBUFXTRA) {
         rsize = fread(inbuf+insize, 1, IBUFSIZ, br->DataFilePtr);
         if (rsize <= 0 && !feof(br->DataFilePtr)) {
            error = IcsErr_FReadIds;
            goto error_exit;
         }
         insize += rsize;
      }

      inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 : (insize<<3)-(n_bits-1));

      while (inbits > posbits) {
         if (free_ent > maxcode) {
            posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
            ++n_bits;
            if (n_bits == maxbits) {
               maxcode = maxmaxcode;
            }
            else {
               maxcode = MAXCODE(n_bits)-1;
            }
            bitmask = (1<<n_bits)-1;
            goto resetbuf;
         }

         input(inbuf,posbits,code,n_bits,bitmask);

         if (oldcode == -1) {
            if (code >= 256) {
               printf("point 3!\n");
               error = IcsErr_CorruptedStream;
               goto error_exit;
            }
            ((unsigned char*)outbuf)[outpos++] = (unsigned char)(finchar = (int)(oldcode = code));
            continue;
         }

         if (code == CLEAR && block_mode) {
            clear_tab_prefixof();
            free_ent = FIRST - 1;
            posbits = ((posbits-1) + ((n_bits<<3) - (posbits-1+(n_bits<<3))%(n_bits<<3)));
            maxcode = MAXCODE(n_bits = INIT_BITS)-1;
            bitmask = (1<<n_bits)-1;
            goto resetbuf;
         }

         incode = code;
         stackp = de_stack;

         if (code >= free_ent) { /* Special case for KwKwK string.   */
            if (code > free_ent) {
               printf("point 4!\n");
               error = IcsErr_CorruptedStream;
               goto error_exit;
            }
            *--stackp = (unsigned char)finchar;
            code = oldcode;
         }

         /* Generate output characters in reverse order */
         while (code >= 256) {
            *--stackp = tab_suffixof(code);
            code = tab_prefixof(code);
         }
         *--stackp = (unsigned char)(finchar = tab_suffixof(code));

         /* And put them out in forward order */
         ii = de_stack-stackp;
         if (outpos+ii > len) {
            ii = len-outpos; /* do not write more in buffer than fits! */
         }
         memcpy(((unsigned char*)outbuf)+outpos, stackp, ii);
         outpos += ii;
         if (outpos == len) {
            goto error_exit;
         }

         if ((code = free_ent) < maxmaxcode) { /* Generate the new entry. */
            tab_prefixof(code) = (unsigned short)oldcode;
            tab_suffixof(code) = (unsigned char)finchar;
            free_ent = code+1;
         } 

         oldcode = incode; /* Remember previous code. */
      }

   } while (rsize > 0);
   
   if (outpos != len) {
      error = IcsErr_OutputNotFilled;
   }

error_exit:
   /* Deallocate stuff */
   if (inbuf) free(inbuf);
   if (htab) free(htab);
   if (codetab) free(codetab);
   return error;
}
