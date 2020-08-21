# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

FIND_PATH(CAP_INCLUDE_DIR 
          NAMES
          sys/capability.h
          capability.h
)
  
FIND_LIBRARY(CAP_LIBRARY
             NAMES 
             cap
)

SET(CAP_INCLUDE_DIRS ${CAP_INCLUDE_DIR})
SET(CAP_LIBRARIES ${CAP_LIBRARY})

IF(CAP_INCLUDE_DIRS)
  MESSAGE(STATUS "Cap include dirs set to ${CAP_INCLUDE_DIRS}")
ELSE(CAP_INCLUDE_DIRS)
  MESSAGE(FATAL " Cap include dirs cannot be found")
ENDIF(CAP_INCLUDE_DIRS)

IF(CAP_LIBRARIES)
  MESSAGE(STATUS "Pcap library set to ${CAP_LIBRARIES}")
ELSE(CAP_LIBRARIES)
  MESSAGE(FATAL "Pcap library cannot be found")
ENDIF(CAP_LIBRARIES)

INCLUDE(CheckFunctionExists)
SET(CMAKE_REQUIRED_INCLUDES ${CAP_INCLUDE_DIRS})
SET(CMAKE_REQUIRED_LIBRARIES ${CAP_LIBRARIES})


IF(CAP_INCLUDE_DIRS AND CAP_LIBRARIES)
  SET(CAP_FOUND "YES" )
ENDIF(CAP_INCLUDE_DIRS AND CAP_LIBRARIES)

find_package_handle_standard_args(LibCAP DEFAULT_MSG
    CAP_LIBRARIES
    CAP_INCLUDE_DIRS
)

MARK_AS_ADVANCED(
  CAP_LIBRARIES
  CAP_INCLUDE_DIRS
)

if(CAP_FOUND)
  if(NOT TARGET LibCAP::LibCAP)
    add_library(LibCAP::LibCAP UNKNOWN IMPORTED)
    set_target_properties(LibCAP::LibCAP PROPERTIES
      IMPORTED_LOCATION "${CAP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${CAP_INCLUDE_DIRS}"
      )
  endif()
endif()
