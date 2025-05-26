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

# Given GCC 7.3 does not provide std::filesystem we use Boost instead
# Drop this once we move to GCC 8.2+

if(TARGET JAliEn::JAliEn)
  set(EXTRA_TARGETS XRootD::Client JAliEn::JAliEn)
endif()

o2_add_library(FrameworkAnalysisSupport
               SOURCES src/Plugin.cxx
                       src/DataInputDirector.cxx
                       src/AODJAlienReaderHelpers.cxx
                       src/AODWriterHelpers.cxx
               PRIVATE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/src
               PUBLIC_LINK_LIBRARIES O2::Framework ${EXTRA_TARGETS} ROOT::TreePlayer)

# To support both pre and post 6.36 API changes in RNTuple
if(ROOT_MAJOR_VERSION EQUAL 6 AND ROOT_MINOR_VERSION LESS 36)
o2_add_library(FrameworkAnalysisRNTupleSupport
               SOURCES src/RNTupleExperimentalPlugin.cxx
               PRIVATE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/src
               PUBLIC_LINK_LIBRARIES O2::Framework ${EXTRA_TARGETS} ROOT::ROOTNTuple ROOT::ROOTNTupleUtil)
else()
o2_add_library(FrameworkAnalysisRNTupleSupport
               SOURCES src/RNTuplePlugin.cxx
               PRIVATE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/src
               PUBLIC_LINK_LIBRARIES O2::Framework ${EXTRA_TARGETS} ROOT::ROOTNTuple ROOT::ROOTNTupleUtil)
endif()

o2_add_library(FrameworkAnalysisTTreeSupport
               SOURCES src/TTreePlugin.cxx
               PRIVATE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/src
               PUBLIC_LINK_LIBRARIES O2::Framework ${EXTRA_TARGETS} ROOT::TreePlayer)

o2_add_test(DataInputDirector NAME test_Framework_test_DataInputDirector
               SOURCES test/test_DataInputDirector.cxx
               COMPONENT_NAME Framework
               LABELS framework
               PUBLIC_LINK_LIBRARIES O2::FrameworkAnalysisSupport)
