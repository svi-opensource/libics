#include <windows.h>
#include <stdio.h>
#include "..\..\libics.h"

int IsICS (char *filename) {
   if (IcsVersion (filename, 0) != 0)
      return 1;
   else
      return 0;
}

int NumberOfPlanesInICS (char *filename, char *errortext) {
   ICS* ip;
   Ics_Error retval;
   int ii, numberofplanes = 0;
   int ndims;
   int dims[ICS_MAXDIM];
   Ics_DataType dt;

   /* Read ICS header. */
   for (ii=0; ii<ICS_MAXDIM; ii++) {
      dims[ii] = 1; /* so we can ignore ndims<2 */
   }
   retval = IcsOpen (&ip, filename, "r");
   switch (retval) {
      case IcsErr_Ok:
         break;
      case IcsErr_NotIcsFile:
         strcpy (errortext, "The file is not an ICS file.");
         return 0;
      case IcsErr_UnknownCompression:
         strcpy (errortext, "Unsupported compression method.");
         return 0;
      case IcsErr_FOpenIcs:
         strcpy (errortext, "Couldn't open the ICS file for reading.");
         return 0;
      case IcsErr_Alloc:
         strcpy (errortext, "Couldn't allocate memory to read ICS file.");
         return 0;
      default:
         strcpy (errortext, "Error reading ICS header.");
         return 0;
   }
   IcsGetLayout (ip, &dt, &ndims, dims);
   IcsClose (ip);

   numberofplanes = 1;
   for (ii=2; ii<ndims; ii++) {
      numberofplanes *= dims[ii];
   }

   return numberofplanes;
}

HANDLE ReadICS (char *filename, char *errortext) {
   BITMAPINFO* bi;
   RGBQUAD* cmap;
   HANDLE DIB = 0;
   ICS* ip;
   Ics_DataType dt;
   int ndims;
   int dims[ICS_MAXDIM];
   int bufsize;
   char *buf, *tmp;
   Ics_Error retval;
   int ii, jj, padding;

   int planenumber = 0; /* This value indicates which plane to read, if the image is 3D. */

   /* Read ICS header. */
   for (ii=0; ii<ICS_MAXDIM; ii++) {
      dims[ii] = 1; /* so we can ignore ndims<2 */
   }
   retval = IcsOpen (&ip, filename, "r");
   switch (retval) {
      case IcsErr_Ok:
         break;
      case IcsErr_NotIcsFile:
         strcpy (errortext, "The file is not an ICS file.");
         return DIB;
      case IcsErr_UnknownCompression:
         strcpy (errortext, "Unsupported compression method.");
         return DIB;
      case IcsErr_FOpenIcs:
         strcpy (errortext, "Couldn't open the ICS file for reading.");
         return DIB;
      case IcsErr_Alloc:
         strcpy (errortext, "Couldn't allocate memory to read ICS file.");
         return DIB;
      default:
         strcpy (errortext, "Error reading ICS header.");
         return DIB;
   }
   IcsGetLayout (ip, &dt, &ndims, dims);
   ndims = 2; /* If ndims==3 or more, we only read one plane. */
   /* Apparently, Windows requires that each of the lines starts on a 4-byte boundary */
   padding = ((dims[0]+3)/4)*4 - dims[0];

   /*
   sprintf (errortext, "Image size: %d x %d. Padding = %d.", dims[0], dims[1], padding);
   MessageBox (0, errortext, "ICSviewer", MB_ICONINFORMATION|MB_OK);
   */

   /* Create DIB. */
   bufsize = (dims[0]+padding) * dims[1];
   DIB = GlobalAlloc (GHND, sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD) + bufsize);
   if (!DIB) {
      strcpy (errortext, "Couldn't allocate memory for bitmap.");
      return DIB;
   }
   bi = (BITMAPINFO*)GlobalLock (DIB);
   bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bi->bmiHeader.biWidth = dims[0];
   bi->bmiHeader.biHeight = -dims[1];
   bi->bmiHeader.biPlanes = 1;
   bi->bmiHeader.biBitCount = 8;
   bi->bmiHeader.biCompression = BI_RGB;
   bi->bmiHeader.biSizeImage = 0;
   bi->bmiHeader.biXPelsPerMeter = 0;
   bi->bmiHeader.biYPelsPerMeter = 0;
   bi->bmiHeader.biClrUsed = 0 /*256*/;
   bi->bmiHeader.biClrImportant = 0 /*256*/;
   cmap = bi->bmiColors;
   for (ii=0; ii<256; ii++) {
      cmap->rgbBlue = ii;
      cmap->rgbRed = ii;
      cmap->rgbGreen = ii;
      cmap++;
   }

   /* Read ICS data. */
   buf = (char*)bi + sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD);
   retval = IcsGetPreviewData (ip, buf, dims[0]*dims[1], planenumber);
   IcsClose (ip);
   /* Move the data so that each line starts at a four-byte boundary */
   if (padding != 0) {
      tmp = buf + dims[0]*dims[1] - 1;            /* points to last byte read */
      buf += (dims[0]+padding)*dims[1]-1-padding; /* points to where last byte needs to be */
      for (ii=dims[1]-1; ii>=0; ii--) {
         for (jj=dims[0]-1; jj>=0; jj--) {
            *buf = *tmp;
            tmp--;
            buf--;
         }
         buf -= padding;
      }
   }
   GlobalUnlock (DIB);
   /* Error messages? */
   switch (retval) {
      case IcsErr_FOpenIds:
         strcpy (errortext, "Failed to open the data file.");
         break;
      case IcsErr_FReadIds:
      case IcsErr_CorruptedStream:
      case IcsErr_DecompressionProblem:
      case IcsErr_EndOfStream:
         strcpy (errortext, "Failed reading the data file.");
         break;
      case IcsErr_IllegalROI:
         strcpy (errortext, "Requested plane is outside the image.");
         break;
      case IcsErr_UnknownDataType:
         strcpy (errortext, "Unsupported pixel data type.");
         break;
      case IcsErr_UnknownCompression:
         strcpy (errortext, "Unsupported compression method.");
         break;
      case IcsErr_Alloc:
         strcpy (errortext, "Couldn't allocate memory to read ICS file.");
         break;
      default:
         strcpy (errortext, "Unspecified error reading data.");
      case IcsErr_Ok:
      case IcsErr_OutputNotFilled:
         /* It's OK. */
         return DIB;
   }
   /* There's an error */
   GlobalFree (DIB);
   return 0;
}
