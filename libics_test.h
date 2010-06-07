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
 * FILE : libics_test.h
 *
 * Only needed to add debug functionality.
 */

#ifndef LIBICS_TEST_H
#define LIBICS_TEST_H

#ifndef LIBICS_H
#include "libics.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Function declarations and short explanation:
 */

void IcsPrintIcs (ICS const* ics);
/* Prints the contents of the ICS structure to stdout (using only printf). */

void IcsPrintError (Ics_Error error);
/* Prints a textual representation of the error message to stdout (using
 * only printf). */

#ifdef __cplusplus
}
#endif

#endif /* LIBICS_TEST_H */
