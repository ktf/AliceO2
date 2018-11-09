filegroup(name = "all_sources", srcs = glob(["**/logger/**/*.cxx"]), visibility = ["//visibility:public"])
filegroup(name = "all_headers", srcs = glob(["**/logger/**/*.h"]), visibility = ["//visibility:public"])

cc_library(
  name="FairLogger",
  srcs=[":all_sources", ":all_headers"],
  hdrs=[":all_headers", ":all_sources"],
  includes=["logger"],
  include_prefix="fairlogger",
  strip_include_prefix="logger",
  visibility=["//visibility:public"]
)

