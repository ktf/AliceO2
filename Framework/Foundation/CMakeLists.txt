# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

if (TARGET LibCAP::LibCAP)
  set(LIBCAP_TARGET LibCAP::LibCAP)
endif()
o2_add_library(FrameworkFoundation
               SOURCES src/RuntimeError.cxx
               PUBLIC_LINK_LIBRARIES ${LIBCAP_TARGET}
               TARGETVARNAME targetName)
if (TARGET LibCAP::LibCAP)
  target_compile_definitions(${targetName} PUBLIC HAS_LIBCAP)
endif()

o2_add_test(test_FunctionalHelpers NAME test_FrameworkFoundation_test_FunctionalHelpers
            COMPONENT_NAME FrameworkFoundation
            SOURCES test/test_FunctionalHelpers.cxx
            PUBLIC_LINK_LIBRARIES O2::FrameworkFoundation)

o2_add_test(test_Traits NAME test_FrameworkFoundation_test_Traits
            COMPONENT_NAME FrameworkFoundation
            SOURCES test/test_Traits.cxx
            PUBLIC_LINK_LIBRARIES O2::FrameworkFoundation)

o2_add_test(test_StructToTuple NAME test_FrameworkFoundation_StructToTuple
            COMPONENT_NAME FrameworkFoundation
            SOURCES test/test_StructToTuple.cxx
            PUBLIC_LINK_LIBRARIES O2::FrameworkFoundation)

o2_add_test(test_CompilerBuiltins NAME test_FrameworkFoundation_CompilerBuiltins
            COMPONENT_NAME FrameworkFoundation
            SOURCES test/test_CompilerBuiltins.cxx
            PUBLIC_LINK_LIBRARIES O2::FrameworkFoundation)

o2_add_test(test_Signpost NAME test_FrameworkFoundation_Signpost
            COMPONENT_NAME FrameworkFoundation
            SOURCES test/test_Signpost.cxx
            PUBLIC_LINK_LIBRARIES O2::FrameworkFoundation)

o2_add_test(test_RuntimeError NAME test_FrameworkFoundation_RuntimeError
            COMPONENT_NAME FrameworkFoundation
            SOURCES test/test_RuntimeError.cxx
            PUBLIC_LINK_LIBRARIES O2::FrameworkFoundation)
