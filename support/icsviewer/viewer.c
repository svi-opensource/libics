#include <windows.h>
#pragma hdrstop
#include "stdio.h"
#include "..\..\libics.h"

/* Functions in readics.c */
HANDLE ReadICS (char *filename, char *errortext);
int NumberOfPlanesInICS (char *filename, char *errortext);
/* Functions in writedib.c */
BOOL WriteDIB( LPTSTR szFile, HANDLE hDIB, LPBITMAPINFO );

LRESULT CALLBACK WndProc  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About    (HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst;
LPCTSTR lpszAppName  = "IcsViewer";
LPCTSTR lpszTitle    = "ICS Viewer Test";
HANDLE DIB;
BITMAPINFO* bi;
char errortext[300];
char filename[300];

/**********************************************************************/

void SetClientRect( HWND hWnd, int width, int height )
{
   RECT clientRect;
   RECT windowRect;
   int widthExtra, heightExtra;

   GetClientRect( hWnd, &clientRect );
   GetWindowRect( hWnd, &windowRect );

   widthExtra = windowRect.right - windowRect.left - clientRect.right;
   heightExtra = windowRect.bottom - windowRect.top - clientRect.bottom;

   windowRect.right = windowRect.left + width + widthExtra;
   windowRect.bottom = windowRect.top + height + heightExtra;

   SetWindowPos( hWnd, 0,
      windowRect.left, windowRect.top,
      windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
      SWP_NOMOVE | SWP_NOZORDER );
}

/**********************************************************************/

int CleanupResources( void )
{
   if( DIB )
   {
      GlobalUnlock (DIB);
      GlobalFree (DIB);
      DIB = 0;
      bi = 0;
   }

   return 1;
}

/**********************************************************************/

int LoadICSFile( HWND hWnd, char *filename )
{
   int planes;
   int newWidth, newHeight;

   /* Release previous bitmap */
   CleanupResources();

   planes = NumberOfPlanesInICS (filename, errortext);
   if (planes < 1) {
      MessageBox (hWnd, errortext, "ICSviewer", MB_ICONSTOP|MB_OK);
      return 0L;
   }
   else {
      //sprintf (errortext, "%d planes in file.", planes);
      //MessageBox (hWnd, errortext, "ICSviewer", MB_ICONINFORMATION|MB_OK);
   }

   /* Read in the ICS file and convert it to a DIB. */
   DIB = ReadICS (filename, errortext);
   if (!DIB) {
      MessageBox (hWnd, errortext, "ICSviewer", MB_ICONSTOP|MB_OK);
      return 0L;
   }

   /* Lock the info */
   bi = (BITMAPINFO*)GlobalLock (DIB);

   /* Resize the window to the image size */
   if( bi->bmiHeader.biWidth < 100 )
      newWidth = 100;
   else
      newWidth = bi->bmiHeader.biWidth;

   if( abs(bi->bmiHeader.biHeight) < 40 )
      newHeight = 40;
   else
      newHeight = abs(bi->bmiHeader.biHeight);

   SetClientRect( hWnd, newWidth, newHeight );


   /* Invalidate so we get a repaint message */
   InvalidateRect( hWnd, NULL, TRUE );

   return 1;
}

/**********************************************************************/

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch( uMsg ) {
      case WM_COMMAND :
         switch( LOWORD( wParam ) ) {
            case 200: /* Load menu item */
            {
               OPENFILENAME ofn;

               /* Get file name. */
               filename[0] = 0;
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hWnd;
               ofn.lpstrFile = filename;
               ofn.nMaxFile = sizeof(filename);
               ofn.lpstrFilter = "ICS image files\0*.ics\0\0";
               ofn.nFilterIndex = 1;
               ofn.lpstrFileTitle = NULL;
               ofn.nMaxFileTitle = 0;
               ofn.lpstrInitialDir = NULL;
               ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

               if (!GetOpenFileName (&ofn) )
                  return 0L;

               if( LoadICSFile( hWnd, filename ) == 0 )
                  return 0;
               break;
            }

            case 300: /* Save bitmap */
            {
               OPENFILENAME ofn;

               /* Can't save an empty bitmap */
               if ( DIB == 0 ) {
                  MessageBox( hWnd, "Load an ICS file first", "ICSviewer", MB_OK|MB_ICONSTOP );
                  break;
               }

               /* Get file name. */
               filename[0] = 0;
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hWnd;
               ofn.lpstrFile = filename;
               ofn.nMaxFile = sizeof(filename);
               ofn.lpstrFilter = "BMP files\0*.bmp\0\0";
               ofn.nFilterIndex = 1;
               ofn.lpstrFileTitle = NULL;
               ofn.nMaxFileTitle = 0;
               ofn.lpstrInitialDir = NULL;
               ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

               if (!GetSaveFileName(&ofn) ) {
                  return 0L;
               }

               /* Force .bmp extension */
               if( !strstr( ofn.lpstrFile, ".bmp" ))
                  strcat( ofn.lpstrFile, ".bmp" );

               /*
               sprintf( textBuf, "Saving to %s", ofn.lpstrFile );
               MessageBox( hWnd, textBuf, "Boo", MB_OK|MB_ICONINFORMATION );
               */

               WriteDIB( ofn.lpstrFile, DIB, bi );

            }
            break;

            case 100 /* Exit menu item */ :
               DestroyWindow( hWnd );
               break;
         }
         break;      /* WM_COMMAND */

      case WM_PAINT :
      {
         HDC hDC;
         PAINTSTRUCT ps;
         int height;

         hDC = BeginPaint( hWnd, &ps );

         if( DIB )
         {
            /* Create a memory device context and select the DIB into it. */
            height = bi->bmiHeader.biHeight;
            if (height<0)
            height = -height;

            StretchDIBits (hDC, 0, 0, bi->bmiHeader.biWidth, height,
            0, 0, bi->bmiHeader.biWidth, height,
            (char*)bi + sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD),
            bi, DIB_RGB_COLORS, SRCCOPY);
         }
         else
         {
            char *text = "No image loaded";
            TextOut( hDC, 5, 5, text, strlen( text ) );
         }

         EndPaint( hWnd, &ps );
         break;
      }

      case WM_DESTROY :
         CleanupResources();
         PostQuitMessage(0);
         break;

      default :
         return( DefWindowProc( hWnd, uMsg, wParam, lParam ) );
   }

   return 0L;
}

/**********************************************************************/

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine, int nCmdShow) {
   MSG      msg;
   HWND     hWnd;
   WNDCLASSEX wc;

   /* Register the main application window class. */
   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = (WNDPROC)WndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon( hInstance, lpszAppName );
   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
   wc.lpszMenuName  = lpszAppName;
   wc.lpszClassName = lpszAppName;
   wc.cbSize = sizeof(WNDCLASSEX);
   wc.hIconSm = LoadImage(hInstance, lpszAppName,
                IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
   RegisterClassEx( &wc );

   hInst = hInstance;

   /* Create the main application window. */
   hWnd = CreateWindow( lpszAppName,
      lpszTitle,
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0,
      CW_USEDEFAULT, 0,
      NULL,
      NULL,
      hInstance,
      NULL );

   if ( !hWnd )
      return( FALSE );


   /* If there was a file in the command line, load it */
   if( lpCmdLine && lpCmdLine[0] )
      LoadICSFile( hWnd, lpCmdLine );

   ShowWindow( hWnd, nCmdShow );
   UpdateWindow( hWnd );


   while( GetMessage( &msg, NULL, 0, 0) )    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
   }

   return( msg.wParam );
}
