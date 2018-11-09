cc_library(
  name = "CommonUtils",
  srcs = glob(["src/*.cxx", "include/CommonUtils/*.h"]),
  hdrs = glob(["include/CommonUtils/*.h"]),
  includes = ["include"],
  visibility = ["//visibility:public"],
  deps = [
      "@FairLogger//:FairLogger",
      "@root//:Core",
      "@root//:Tree",
      "@boost//:interprocess",
      "@boost//:iostreams"
  ]
)
