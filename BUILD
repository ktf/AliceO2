load("@rules_foreign_cc//tools/build_defs:cmake.bzl", "cmake_external")
load("@rules_foreign_cc//tools/build_defs:boost_build.bzl", "boost_build")
load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")

boost_build(
  name = "boost_fairmq",
  lib_source = "@boost//:all",
  shared_libraries = ["libboost_regex.dylib",
                      "libboost_filesystem.dylib",
                      "libboost_container.dylib",
                      "libboost_date_time.dylib",
                      "libboost_program_options.dylib",
                      "libboost_serialization.dylib",
                      ],
  static_libraries = ["libboost_regex.a",
                      "libboost_filesystem.a",
                      "libboost_container.a",
                      "libboost_date_time.a",
                      "libboost_program_options.a",
                      "libboost_serialization.a",
                      ],
  user_options = [
      "--with-regex",
      "--with-filesystem",
      "--with-container",
      "--with-date_time",
      "--with-program_options",
      "--with-serialization",
  ],
)

boost_build(
  name = "boost_filesystem",
  lib_source = "@boost//:all",
  static_libraries = ["libboost_filesystem.a"],
  user_options = ["--with-filesystem"],
)

config_setting(
  name = "darwin",
  values = {"cpu": "darwin"},
  visibility = ["//visibility:public"],
)

config_setting(
  name = "windows",
  values = {"cpu": "x64_windows"},
  visibility = ["//visibility:public"],
)

config_setting(
  name = "linux_ppc64le",
  values = {"cpu": "ppc"},
  visibility = ["//visibility:public"],
)

configure_make(
  name = "ZeroMQ",
  lib_source = "@zeromq//:all",
  configure_in_place = True,
  static_libraries = ["libzmq.a"],
  configure_options = [
      "--disable-dependency-tracking"
  ],
  # libevent script uses it's own libtool for linking;
  # so do not specify linker tool for it
  # (otherwise, if the libtool from bazel's toolchain is supplied,
  # the build script has problems with passing output file to libtool)
  # see #315
  configure_env_vars = {
    "AR": "",
  },
#  out_lib_dir = "lib",
)

cmake_external(
  name = "RapidJSON",
  lib_source = "@RapidJSON//:all",
  static_libraries = ["librapidjson.a"],
  cmake_options = ["-GNinja"],
  headers_only = True,
  cache_entries = {
      "CXXFLAGS": "-Wno-error",
      },
  make_commands = [
    "ninja",
    "ninja install",
  ],
)

cmake_external(
  name = "FairLogger",
  # Values to be passed as -Dkey=value on the CMake command line;
  # here are serving to provide some CMake script configuration options
  cmake_options = ["-GNinja"],

  cache_entries = {
      "BUILD_TESTING": "OFF",
      "PROJECT_VERSION": "1.5.0",
      "PROJECT_GIT_VERSION": "1.5.0"
  },
  make_commands = [
    "ninja",
    "ninja install",
  ],
  lib_source = "@FairLogger//:all",
  shared_libraries=["libFairLogger.dylib"],
)

cmake_external(
  name = "ROOT",
  # Values to be passed as -Dkey=value on the CMake command line;
  # here are serving to provide some CMake script configuration options
  cmake_options = [
      "-GNinja",
      "-Dfreetype=ON",
      "-Dbuiltin_freetype=OFF",
      "-Dpcre=OFF",
      "-Dbuiltin_pcre=ON",
      "-Dsqlite=OFF",
      "-Dpgsql=OFF",
      "-Dminuit2=ON",
      "-Dpythia6_nolink=ON",
      "-Droofit=ON",
      "-Dhttp=ON",
      "-Droot7=OFF",
      "-Dsoversion=ON",
      "-Dshadowpw=OFF",
      "-Dvdt=on",
      "-Dbuiltin_vdt=ON",
      "-Dkrb5=OFF",
      "-Dgviz=OFF",
      "-Dbuiltin_davix=OFF",
      "-Dvmc=ON",
      "-Ddavix=OFF",
      "-Dmysql=OFF",
      "-Dpython=OFF",
  ],
  cache_entries = {
      "ENABLE_COCOA": "1",
      "DISABLE_MYSQL": "1",
      "PROJECT_GIT_VERSION": "1.5.0"
  },
  make_commands = [
    "ninja",
    "ninja install",
  ],
  lib_source = "@ROOT//:all",
)


#cmake_external(
#  name = "arrow",
#  cmake_options = ["-GNinja"],
#  cache_entries = {
#      "ARROW_DEPENDENCY_SOURCE": "SYSTEM",
#      "ARROW_BUILD_BENCHMARKS": "OFF",
#      "ARROW_BUILD_TESTS": "OFF",
#      "ARROW_USE_GLOG": "OFF",
#      "ARROW_JEMALLOC": "OFF",
#      "ARROW_IPC": "ON",
#      "ARROW_PYTHON": "OFF",
#      "ARROW_WITH_SNAPPY": "OFF",
#      "ARROW_WITH_ZSTD": "OFF",
#      "ARROW_WITH_BROTLI": "OFF",
#      "ARROW_WITH_ZLIB": "OFF",
#      "ARROW_WITH_LZ4": "OFF",
#      "BOOST_ROOT": "$EXT_BUILD_DEPS/boost_fairmq/",
#      "BOOST_INCLUDEDIR": "$EXT_BUILD_DEPS/boost_fairmq/include",
#      "DoubleConversion_INCLUDE_DIR": "$EXT_BUILD_DEPS/include",
#      "FLATBUFFERS_INCLUDE_DIR": "$EXT_BUILD_DEPS/include",
#      "RapidJSON_INCLUDE_DIR": "$EXT_BUILD_DEPS/RapidJSON/include",
#      "FLATC": "@flatbuffers//:flatc"
#  },
#  make_commands = [
#    "ninja",
#    "ninja install",
#  ],
#  lib_source = "@arrow//:all",
#  shared_libraries=["libarrow.dylib"],
#  deps = [":boost_fairmq", "@double-conversion//:double-conversion", ":RapidJSON", "@flatbuffers//:flatbuffers", "@flatbuffers//:flatc"]
#)

cmake_external(
  name = "FairMQ",
  cmake_options = ["-GNinja"],

  cache_entries = {
      "BUILD_TESTING": "OFF",
      "FairLogger_ROOT": "$EXT_BUILD_DEPS/FairLogger",
      "CMAKE_DISABLE_FIND_PACKAGE_BOOST": "True",
      "BOOST_VERSION": "1.68.0",
      "BOOST_ROOT": "$EXT_BUILD_DEPS/boost_fairmq/",
      "ZEROMQ_ROOT": "$EXT_BUILD_DEPS/ZeroMQ/",
      "BOOST_INCLUDEDIR": "$EXT_BUILD_DEPS/boost_fairmq/include",
  },
  make_commands = [
    "ninja",
    "ninja install",
  ],
  lib_source = "@FairMQ//:all",
  shared_libraries=["libFairMQ.dylib"],
  deps = [":FairLogger", ":boost_fairmq", ":ZeroMQ"]
)
