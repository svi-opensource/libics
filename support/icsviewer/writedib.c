#include <windows.h>
#include <stdio.h>

/* WriteDIB - Writes a DIB to file
 * Returns  - TRUE on success
 * szFile   - Name of file to write to
 * hDIB     - Handle of the DIB
 */

/* 20020912 - Patched by Frank de Jong from CodeGuru original */

BOOL WriteDIB( LPTSTR szFile, HANDLE hDIB, LPBITMAPINFO lpbi )
{
   BITMAPFILEHEADER  hdr;
   LPBITMAPINFOHEADER   lpbih;
   FILE *fp;
   int nColors;
   int bitSize;

   if (!hDIB)
      return FALSE;

   if( (fp = fopen( szFile, "wb" )) == 0 )
      return FALSE;

   lpbih = (LPBITMAPINFOHEADER) lpbi;

   nColors = 1 << lpbih->biBitCount;

   // Fill in the fields of the file header
   hdr.bfType      = ((WORD) ('M' << 8) | 'B');  // is always "BM"
   hdr.bfSize      = GlobalSize (hDIB) + sizeof( hdr );
   hdr.bfReserved1 = 0;
   hdr.bfReserved2 = 0;
   hdr.bfOffBits   = (DWORD) (sizeof( hdr ) + lpbih->biSize +
                             nColors * sizeof(RGBQUAD));

   // Write the file header
   fwrite( &hdr, 1, sizeof(hdr), fp );

   // Write the DIB header and the bits
   bitSize = GlobalSize(hDIB);
   fwrite( lpbih, 1, bitSize, fp );

   fclose( fp );

   return TRUE;
}
