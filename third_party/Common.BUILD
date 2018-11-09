filegroup(name = "all_sources", 
          srcs = glob(["**/cpp/src/arrow/*.cc", "**/cpp/src/arrow/util/visibility.cc", "**/cpp/src/arrow/array/*.cc"], 
                      exclude=["**/*benchmark*", "**/*test*", "**/hdfs.*", "**/*_snappy.*", "**/dbi/**"]), 
          visibility = ["//visibility:public"])
filegroup(name = "all_headers", 
          srcs = glob(["**/cpp/src/arrow/*.h",
                      "**/cpp/src/arrow/util/*.h",
                      "**/cpp/src/arrow/vendored/string_view.hpp",
                      "**/cpp/src/arrow/array/*.h"],
                      exclude=["**/*benchmark*", "**/*test*", "**/hdfs.*", "**/*_snappy.*", "**/dbi/**"]), 
          visibility = ["//visibility:public"])

cc_library(
  name="arrow",
  srcs=[":all_sources", ":all_headers"],
  hdrs=[":all_headers", ":all_sources"],
  strip_include_prefix="cpp/src",
  visibility=["//visibility:public"]
)

