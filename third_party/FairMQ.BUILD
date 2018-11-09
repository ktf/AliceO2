
cc_library(
  name="FairMQ",
  srcs=glob(["fairmq/*.cxx"]),
  hdrs=glob(["fairmq/**/*.h"]),
  includes=["FairMQ/fairmq"],
  deps=["@FairLogger//:FairLogger", "@boost//:any", "@boost//:algorithm",
        "@boost//:msm", "@boost//:signals2", "@boost//:program_options",
        "@boost//:dll", "@boost//:filesystem", "@boost//:uuid", "@boost//:interprocess",
        "@zeromq//:zmq"],
  copts=["-IFairMQ/fairmq", "-IFairMQ"],
  visibility=["//visibility:public"]
)
