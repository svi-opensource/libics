/* ICSWRITE   Writes a numeric array to an ICS file.
 *     ICSWRITE(A,FILENAME,COMPRESS) writes the numeric data
 *     in A to an ICS image file named FILENAME. If COMPRESS
 *     is non-zero the data will be written compressed.
 *
 *     Known limitations:
 *        - Complex data is not written.
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
   int ndims;
   const mwSize* mx_dims;
   size_t dims[ICS_MAXDIM];
   size_t strides[ICS_MAXDIM];
   size_t bufsize;
   void* buf;
   Ics_Error retval;
   mxClassID class;
   char filename[ICS_MAXPATHLEN];
   int elemsize, compress = 0;
   int ii;
   size_t tmp;
   char errormessage[2048];

   if (strcmp (ICSLIB_VERSION, IcsGetLibVersion ()))
      mexErrMsgTxt ("Linking against the wrong version of the library.");

   /* There should be no output arguments. */
   if (nlhs > 0)
      mexErrMsgTxt ("Too many output arguments.");

   /* There should be two or three input arguments. */
   if (nrhs > 3)
      mexErrMsgTxt ("Too many input arguments.");
   if (nrhs < 2)
      mexErrMsgTxt ("Not enough input arguments.");

   /* First input argument. */
   class = mxGetClassID (prhs[0]);
   switch (class) {
      case mxDOUBLE_CLASS:
         dt = Ics_real64;
         elemsize = 8;
         break;
      case mxSINGLE_CLASS:
         dt = Ics_real32;
         elemsize = 4;
         break;
      case mxINT8_CLASS:
         dt = Ics_sint8;
         elemsize = 1;
         break;
      case mxUINT8_CLASS:
         dt = Ics_uint8;
         elemsize = 1;
         break;
      case mxINT16_CLASS:
         dt = Ics_sint16;
         elemsize = 2;
         break;
      case mxUINT16_CLASS:
         dt = Ics_uint16;
         elemsize = 2;
         break;
      case mxINT32_CLASS:
         dt = Ics_sint32;
         elemsize = 4;
         break;
      case mxUINT32_CLASS:
         dt = Ics_uint32;
         elemsize = 4;
         break;
      default:
         mexErrMsgTxt ("Input array should be numeric.");
   }
   if (mxIsComplex (prhs[0]))
      mexErrMsgTxt ("Cannot write complex data (I'm too lazy).");
   buf = mxGetData (prhs[0]);
   ndims = (int)mxGetNumberOfDimensions (prhs[0]);
   mx_dims = mxGetDimensions (prhs[0]);
   for (ii=0;ii<ndims;ii++)
      dims[ii] = (size_t)(mx_dims[ii]);
   strides[0] = 1;
   for (ii=1;ii<ndims;ii++)
      strides[ii] = strides[ii-1]*dims[ii-1];
   if (ndims>1) {
      /* This is to swap the first two dimensions; MATLAB does y-x-z indexing. */
      tmp = dims[0];
      dims[0] = dims[1];
      dims[1] = tmp;
      strides[0] = dims[1];
      strides[1] = 1;
   }
   bufsize = mxGetNumberOfElements (prhs[0]) * elemsize;

   /* Second input argument. */
   filename[0] = '\0';
   if (mxGetString (prhs[1], filename, ICS_MAXPATHLEN)) {
      if (filename[0] == '\0')
         mexErrMsgTxt ("FILENAME should be a character array.");
      else
         mexErrMsgTxt ("The given filename is too long.");
   }

   /* Third input argument. */
   if (nrhs > 2) {
      /* No checking whatsoever! */
      compress = (mxGetScalar (prhs[2]) != 0);
   }

   /* Now we know everything we need to write the data. */
   retval = IcsOpen (&ip, filename, "w1");
   if (retval != IcsErr_Ok) {
      sprintf (errormessage, "Couldn't open the file for writing: %s", IcsGetErrorText (retval));
      mexErrMsgTxt (errormessage);
   }
   IcsSetLayout (ip, dt, ndims, dims);
   retval = IcsGuessScilType (ip);
   if (retval == IcsErr_NoScilType)
      mexWarnMsgTxt ("Couldn't create a SCIL_TYPE string.");
   retval = IcsSetDataWithStrides (ip, buf, bufsize, strides, ndims);
   if (retval != IcsErr_Ok) {
      sprintf (errormessage, "Failed to set the data: %s", IcsGetErrorText (retval));
      mexErrMsgTxt (errormessage);
   }
   if (compress)
      IcsSetCompression (ip, IcsCompr_gzip, 0);
   IcsAddHistory (ip, "software", "ICSWRITE under MATLAB with libics");
   retval = IcsClose (ip);
   if (retval != IcsErr_Ok) {
      sprintf (errormessage, "Failed to create the ICS file: %s", IcsGetErrorText (retval));
      mexErrMsgTxt (errormessage);
   }
}
