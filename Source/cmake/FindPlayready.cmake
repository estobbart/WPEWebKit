# - Try to find playready.
# Once done, this will define
#
#  PLAYREADY_INCLUDE_DIRS - the playready include directories
#  PLAYREADY_LIBRARIES - link these to use playready.
#
# Copyright (C) 2016 Igalia S.L.
# Copyright (C) 2016 Metrological
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# TODO: Fix this..
# bitbake playready -f -c clean
# bitbake playready -f -c build

#find_package(PkgConfig)
#pkg_check_modules(PLAYREADY playready)
find_library(PLAYREADY_LIBRARIES NAMES playready)
find_path(PLAYREADY_INCLUDE_DIRS NAMES drmtypes.h PATH_SUFFIXES playready)

include(FindPackageHandleStandardArgs)
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLAYREADY DEFAULT_MSG PLAYREADY_LIBRARIES)

list(APPEND PLAYREADY_LIBRARIES 
    IARMBus
    systemd
)


list(APPEND PLAYREADY_INCLUDE_DIRS 
    ${PLAYREADY_INCLUDE_DIRS}/oem/inc
    ${PLAYREADY_INCLUDE_DIRS}/oem/ansi/inc
    ${PLAYREADY_INCLUDE_DIRS}/oem/common/inc
)

set(PLAYREADY_LIBRARIES ${PLAYREADY_LIBRARIES} CACHE PATH "Path to Playready library")
set(PLAYREADY_INCLUDE_DIRS ${PLAYREADY_INCLUDE_DIRS} CACHE PATH "Path to Playready include")

FIND_PACKAGE_HANDLE_STANDARD_ARGS(PLAYREADY DEFAULT_MSG PLAYREADY_LIBRARIES PLAYREADY_INCLUDE_DIRS)
mark_as_advanced(PLAYREADY_INCLUDE_DIRS PLAYREADY_LIBRARIES)

