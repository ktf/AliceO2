load("@rules_foreign_cc//tools/build_defs:cmake.bzl", "cmake_external")

filegroup(name = "all_sources", srcs = glob(["**/logger/**/*.cxx"]), visibility = ["//visibility:public"])
filegroup(name = "all_headers", srcs = glob(["**/logger/**/*.h"]), visibility = ["//visibility:public"])

cmake_external(
   name = "FairLogger",
   # Values to be passed as -Dkey=value on the CMake command line;
   # here are serving to provide some CMake script configuration options
   cache_entries = {
       "NOFORTRAN": "on",
       "BUILD_WITHOUT_LAPACK": "no",
   },
   lib_source = "@FairLogger//:all",

   # We are selecting the resulting static library to be passed in C/C++ provider
   # as the result of the build;
   # However, the cmake_external dependants could use other artefacts provided by the build,
   # according to their CMake script
   static_libraries = ["libFairLogger.a"],
)
