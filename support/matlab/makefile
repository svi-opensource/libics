#
# Copyright (C) 2000-2007 Cris Luengo and others
# email: clluengo@users.sourceforge.net
#
# Makefile to compile matlab interface under Win32
#

ARCH = win32
MEXEXT = dll

MATLAB_ROOT = C:\MATLAB701
ZLIB_ROOT = ..\..\..\vc_zlib-1.2.3
ICS_ROOT = ..\..
LIBPATH = $(ICS_ROOT)
INCLUDEPATH = $(ICS_ROOT)

MEX = $(MATLAB_ROOT)\bin\win32\mex

MEXFLAGS = -DWIN32

all : icswrite.$(MEXEXT) icsread.$(MEXEXT)
  @echo Done!

icsread.$(MEXEXT): icsread.c $(LIBPATH)\libics.lib $(ZLIB_ROOT)\zlib.lib
  $(MEX) $(MEXFLAGS) -output icsread -I$(INCLUDEPATH) -O $**  


icswrite.$(MEXEXT): icswrite.c $(LIBPATH)\libics.lib $(ZLIB_ROOT)\zlib.lib
  $(MEX) $(MEXFLAGS) -output icswrite -I$(INCLUDEPATH) -O $**  

clean:
  @-del /q *.$(MEXEXT)
