##############################################################################
#
# libics: Image Cytometry Standard file reading and writing.
#
# Copyright (C) 2000-2013, 2016 Cris Luengo and others
# Copyright 2015, 2016:
#   Scientific Volume Imaging Holding B.V.
#   Laapersveld 63, 1213 VB Hilversum, The Netherlands
#   https://www.svi.nl
#
#   CMakeLists.txt
#   Created by Paul Barber <paul.barber@oncology.ox.ac.uk>
#
#   To aid building on Windows 10 and with Visual Studio 2015 (VC14).
#   Requires libics.def
#
# To compile and build on Windows:
# 	cmake CMakeLists.txt   OR use cmake GUI
#   Open sln file in Visual Studio and build solution
#
# To compile and build on linux:
# 	cmake CMakeLists.txt
#   make
#
# To clean on linux:
# 	make clean
#
# TODO: There may be unresolved issues with stricmp vs strcasecmp on Linux.
#
##############################################################################
cmake_minimum_required(VERSION 2.6)

project(libics)

set(USE_ZLIB TRUE CACHE BOOL "Use Zlib")

if(USE_ZLIB)
    find_path(ZLIB_PATH zlib.h ${CMAKE_SOURCE_DIR}/zlib)
	find_library(ZLIB_LIB NAMES zlib libz.so HINTS ${ZLIB_PATH} ${ZLIB_PATH}/build ${ZLIB_PATH}/build/Release)
	find_library(ZLIB_STATIC_LIB NAMES zlibstatic libz.a HINTS ${ZLIB_PATH} ${ZLIB_PATH}/build ${ZLIB_PATH}/build/Release)
    configure_file(libics_conf_with_zlib.h.in ${CMAKE_SOURCE_DIR}/libics_conf.h COPYONLY)
    include_directories(${ZLIB_PATH})
else(USE_ZLIB)
    configure_file(libics_conf.h.in ${CMAKE_SOURCE_DIR}/libics_conf.h COPYONLY)
endif(USE_ZLIB)

set (SOURCES libics_read.c
          libics_write.c
          libics_binary.c
          libics_gzip.c
          libics_compress.c
          libics_data.c
          libics_util.c
          libics_top.c
          libics_history.c
          libics_preview.c
          libics_sensor.c
          libics_test.c
          libics.def
)

include_directories(${CMAKE_SOURCE_DIR})

# Build a dll
add_library(libics SHARED ${SOURCES})

# Build a static library
add_library(libics_static STATIC ${SOURCES})

if(USE_ZLIB)
    target_link_libraries(libics ${ZLIB_LIB})
    target_link_libraries(libics_static ${ZLIB_STATIC_LIB})
endif(USE_ZLIB)

