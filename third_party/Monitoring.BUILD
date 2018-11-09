filegroup(name = "all_sources", 
          srcs = glob(["**/src/*.*", "**/src/Exceptions/*.*", "**/src/UriParser/*.*", 
                       "**/src/BackEnds/*.*", "**/src/Transports/*.*"],
                      exclude=["**/BackEnds/ApMonBackend.*"]),
          visibility = ["//visibility:public"])

filegroup(name = "all_headers", 
          srcs = glob(["include/Monitoring/*.h"]), 
          visibility = ["//visibility:public"])

cc_library(
  name="Monitoring",
  srcs=[":all_sources", ":all_headers"],
  hdrs=[":all_headers"],
  includes=["src"],
  strip_include_prefix="include",
  visibility=["//visibility:public"],
  deps = [
    "@boost//:algorithm",
    "@boost//:variant",
    "@boost//:lexical_cast",
    "@boost//:property_tree",
    "@boost//:asio"
  ]
)

