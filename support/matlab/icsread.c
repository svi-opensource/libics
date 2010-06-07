/* ICSREAD   Reads a numeric array from an ICS file.
 *     A = ICSWRITE(FILENAME) reads the numeric data in
 *     an ICS image file named FILENAME into A.
 *
 *     Known limitations:
 *        - Complex data is not read.
 *
 * Copyright (C) 2000-2007 Cris Luengo and others
 * email: clluengo@users.sourceforge.net
 * Last change: April 30, 2007
 */

/* For MATLAB 7.3 and newer, remove the following line: */
typedef int mwSize;

#include "mex.h"
#include <string.h>
#include "libics.h"

void mexFunction (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
   ICS* ip;
   Ics_DataType dt;
   mwSize mx_dims[ICS_MAXDIM];
   int ndims;
   size_t dims[ICS_MAXDIM];
   size_t strides[ICS_MAXDIM];
   size_t bufsize;
   void* buf;
   Ics_Error retval;
   mxClassID class;
   char filename[ICS_MAXPATHLEN];
   int elemsize;
   int ii;
   size_t tmp;
   char errormessage[2048];

   if (strcmp (ICSLIB_VERSION, IcsGetLibVersion ()))
      mexErrMsgTxt ("Linking against the wrong version of the library.");

   /* There should be one output argument. */
   if (nlhs > 1)
      mexErrMsgTxt ("Too many output arguments.");

   /* There should be one input argument. */
   if (nrhs > 1)
      mexErrMsgTxt ("Too many input arguments.");
   if (nrhs < 1)
      mexErrMsgTxt ("Not enough input arguments.");

   /* First input argument. */
   filename[0] = '\0';
   if (mxGetString (prhs[0], filename, ICS_MAXPATHLEN)) {
      if (filename[0] == '\0')
         mexErrMsgTxt ("FILENAME should be a character array.");
      else
         mexErrMsgTxt ("The given filename is too long.");
   }

   /* Read ICS header. */
   retval = IcsOpen (&ip, filename, "r");
   switch (retval) {
      case IcsErr_Ok:
         break;
      case IcsErr_NotIcsFile:
         mexErrMsgTxt ("The file is not an ICS file.");
         break;
      case IcsErr_UnknownCompression:
         mexErrMsgTxt ("Unsupported compression method.");
         break;
      case IcsErr_FOpenIcs:
         mexErrMsgTxt ("Couldn't open the file for reading.");
         break;
      default:
         sprintf (errormessage, "Couldn't read the ICS header: %s", IcsGetErrorText (retval));
         mexErrMsgTxt (errormessage);
   }
   IcsGetLayout (ip, &dt, &ndims, dims);
   switch (dt) {
      case Ics_real64:
         class = mxDOUBLE_CLASS;
         elemsize = 8;
         break;
      case Ics_real32:
         class = mxSINGLE_CLASS;
         elemsize = 4;
         break;
      case Ics_sint8:
         class = mxINT8_CLASS;
         elemsize = 1;
         break;
      case Ics_uint8:
         class = mxUINT8_CLASS;
         elemsize = 1;
         break;
      case Ics_sint16:
         class = mxINT16_CLASS;
         elemsize = 2;
         break;
      case Ics_uint16:
         class = mxUINT16_CLASS;
         elemsize = 2;
         break;
      case Ics_sint32:
         class = mxINT32_CLASS;
         elemsize = 4;
         break;
      case Ics_uint32:
         class = mxUINT32_CLASS;
         elemsize = 4;
         break;
      case Ics_complex64:
      case Ics_complex32:
         mexErrMsgTxt ("Cannot read complex data (I'm too lazy).");
         break;
      default:
         mexErrMsgTxt ("Unknown data type in ICS file.");
   }
   strides[0] = 1;
   for (ii=1;ii<ndims;ii++)
      strides[ii] = strides[ii-1]*dims[ii-1];
   if (ndims>1) {
      /* This is to swap the first two dimensions; MATLAB does y-x-z indexing. */
      strides[0] = dims[1];
      strides[1] = 1;
      tmp = dims[0];
      dims[0] = dims[1];
      dims[1] = tmp;
   }
   for (ii=0;ii<ndims;ii++)
      mx_dims[ii] = (mwSize)(dims[ii]);
   plhs[0] = mxCreateNumericArray ((mwSize)ndims, mx_dims, class, mxREAL);
   buf = mxGetData (plhs[0]);
   bufsize = mxGetNumberOfElements (plhs[0]) * elemsize;
   retval = IcsGetDataWithStrides (ip, buf, bufsize, strides, ndims);
   if (retval != IcsErr_Ok) {
      sprintf (errormessage, "Couldn't read the image data: %s", IcsGetErrorText (retval));
      mexErrMsgTxt (errormessage);
   }
   retval = IcsClose (ip);
   if (retval != IcsErr_Ok) {
      sprintf (errormessage, "Couldn't close the file pointer: %s", IcsGetErrorText (retval));
      mexErrMsgTxt (errormessage);
   }
}
