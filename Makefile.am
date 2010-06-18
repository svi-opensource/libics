#
# libics: Image Cytometry Standard file reading and writing.
#
# Copyright (C) 2000-2010 Cris Luengo and others
# email: clluengo@sourceforge.net
#
# Script for automake and autoconfig.
# Written by Peter Verveer, Cris Luengo
#

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libics.la
libics_la_LDFLAGS = -version-info $(ICS_LT_VERSION)

# list all sources and include files that are not installed, but are
# distributed, except for libics_conf.h, which is generated from
# libics_conf.h.in:
libics_la_SOURCES = libics_binary.c \
                    libics_compress.c \
                    libics_data.c \
                    libics_gzip.c \
                    libics_history.c \
                    libics_preview.c \
                    libics_read.c \
                    libics_sensor.c \
                    libics_test.c \
                    libics_top.c \
                    libics_util.c \
                    libics_write.c \
                    libics_intern.h

# list all include files that must be installed and distributed:
include_HEADERS = libics.h \
                  libics_ll.h \
                  libics_sensor.h \
                  libics_test.h

# list all test programs
check_PROGRAMS = test_ics1 \
                 test_ics2a \
                 test_ics2b \
                 test_compress \
                 test_gzip \
                 test_strides \
                 test_strides2 \
                 test_metadata \
                 test_history

test_ics1_SOURCES = test_ics1.c
test_ics2a_SOURCES = test_ics2a.c
test_ics2b_SOURCES = test_ics2b.c
test_compress_SOURCES = test_compress.c
test_gzip_SOURCES = test_gzip.c
test_strides_SOURCES = test_strides.c
test_strides2_SOURCES = test_strides2.c
test_metadata_SOURCES = test_metadata.c
test_history_SOURCES = test_history.c

test_ics1_LDADD = libics.la
test_ics2a_LDADD = libics.la
test_ics2b_LDADD = libics.la
test_compress_LDADD = libics.la
test_gzip_LDADD = libics.la
test_strides_LDADD = libics.la
test_strides2_LDADD = libics.la
test_metadata_LDADD = libics.la
test_history_LDADD = libics.la

TESTS = test_ics1.sh \
        test_ics2a.sh \
        test_ics2b.sh \
        test_compress.sh \
        test_gzip.sh \
        test_strides.sh \
        test_strides2.sh \
        test_metadata.sh \
        test_history.sh

# list other files that must go into the distribution:
EXTRA_DIST = INSTALL \
             GNU_LICENSE \
             README \
             bootstrap.sh \
             Makefile.bcc \
             Makefile.vc6 \
             Makefile.vc9 \
             Documentation/index.html \
             Documentation/Credits.html \
             Documentation/Documentation.html \
             Documentation/Enums.html \
             Documentation/Ics_DataRepresentation.html \
             Documentation/Ics_Error.html \
             Documentation/Ics_Header.html \
             Documentation/Ics_ImelRepresentation.html \
             Documentation/LowLevelFunctions.html \
             Documentation/TopLevelFunctions.html \
             Documentation/Usage.html \
             Documentation/libics.css \
             Documentation/iexplorefix.css \
             support/icsviewer/README \
             support/icsviewer/icsviewer.dsp \
             support/icsviewer/icsviewer.dsw \
             support/icsviewer/readics.c \
             support/icsviewer/resource.h \
             support/icsviewer/viewer.c \
             support/icsviewer/viewer.ico \
             support/icsviewer/viewer.rc \
             support/icsviewer/writedib.c \
             support/matlab/README \
             support/matlab/icsread.c \
             support/matlab/icswrite.c \
             support/matlab/makefile \
             test/testim.ics \
             test/testim.ids \
             test/testim_c.ics \
             test/testim_c.ids.Z \
             $(TESTS)

# list other files that should be cleaned
#MOSTLYCLEANFILES = result_v1.ics result_v1.ids  result_v2a.ics result_v2b.ics
mostlyclean-local:
	-rm -f result*
