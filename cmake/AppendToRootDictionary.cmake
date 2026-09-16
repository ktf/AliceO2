# Copyright 2019-2020 CERN and copyright holders of ALICE O2.
# See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
# All rights not expressly granted are reserved.
#
# This software is distributed under the terms of the GNU General Public
# License v3 (GPL Version 3), copied verbatim in the file "COPYING".
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

# Appends PATCH to the end of DICTIONARY. Run with cmake -P, as CMake has no
# `cmake -E` equivalent of `cat a >> b`.

if(NOT DICTIONARY OR NOT PATCH)
  message(FATAL_ERROR "Both DICTIONARY and PATCH must be given")
endif()

file(READ ${PATCH} patchContent)
file(APPEND ${DICTIONARY} "${patchContent}")
