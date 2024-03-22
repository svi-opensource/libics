#
# Copyright (C) 2000-2010 Cris Luengo and others
#
# Makefile to compile matlab interface under Win32
#

MEXEXT = mexw32

MATLAB_ROOT = C:\MATLAB701
ZLIB_ROOT = C:\src\zlib
ICS_ROOT = ..\..
LIBPATH = $(ICS_ROOT)
INCLUDEPATH = $(ICS_ROOT)

MEX = $(MATLAB_ROOT)\bin\win32\mex

MEXFLAGS = -O -DWIN32
# The next line is for the DLL version of libics. If you want to use
# the static library, remove the next line.
MEXFLAGS = $(MEXFLAGS) -DUSE_ICSLIB_DLL

all : icswrite.$(MEXEXT) icsread.$(MEXEXT)
	@echo Done!

icsread.$(MEXEXT): icsread.c
	$(MEX) $(MEXFLAGS) $^ -I$(INCLUDEPATH) $(LIBPATH)\libics.lib $(ZLIB_ROOT)\zlib.lib


icswrite.$(MEXEXT): icswrite.c
	$(MEX) $(MEXFLAGS) $^ -I$(INCLUDEPATH) $(LIBPATH)\libics.lib $(ZLIB_ROOT)\zlib.lib

clean:
	@-del /q *.$(MEXEXT)
