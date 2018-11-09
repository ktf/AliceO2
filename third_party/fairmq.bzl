FAIRMQ_BUILD_FILE="""
filegroup(name = "all_sources", srcs = glob(["**/fairmq/*.cxx", "**/fairmq/shmem/*.*", "**/fairmq/tools/*.*", "**/zeromq/*.*", "**/options/*.*", "**/fairmq/plugins/Builtin.*", "**/fairmq/plugins/Control.*"], exclude=["**/run*"]), visibility = ["//visibility:public"])
filegroup(name = "all_headers", srcs = glob(["**/fairmq/*.h"]), visibility = ["//visibility:public"])

cc_library(
  name = "Version",
  srcs = ["FairMQ-1.3.7/fairmq/tools/Version.h"],

)

#Build a library with all the zeromq code in it.
cc_library(
  name = "FairMQ",
  srcs = [":all_sources"],
  hdrs = [":all_headers"],

  #Include the x86 target and all include files.
  #Add those under llvm - master / ... as well because only built files
  #seem to appear under include / ...
  copts = [
           "-IFairMQ-1.3.7",
           "-IFairMQ-1.3.7/include",
           "-IFairMQ-1.3.7/fairmq",
           ],

  #Include here as well, not sure whether this or copts is
  #actually doing the work.
  includes = [
    "FairMQ-1.3.7",
    "FairMQ-1.3.7/include",
    "FairMQ-1.3.7/fairmq",
    ],
  visibility = ["//visibility:public"],
  #Currently picking up some gtest targets, I have that dependency
  #already, so just link it here until I filter those out.
  deps=["@FairLogger//:FairLogger", "@boost//:any", "@boost//:algorithm",
        "@boost//:msm", "@boost//:signals2", "@boost//:program_options",
        "@boost//:dll", "@boost//:filesystem", "@boost//:uuid", "@boost//:interprocess",
        "@boost//:process", "@boost//:property_tree",
        "@zeromq//:zmq"],
)
    """

def _fairmq_impl(ctx):
  #Downloada FairMQ 
  ctx.download_and_extract(url = "https://github.com/FairRootGroup/FairMQ/archive/v1.3.7.tar.gz",
                           sha256 = "105f5d45392c9e1803417c395407f21fca2beaaca4f167be6f1f2a6872c11cb2")
  ctx.template("FairMQ-1.3.7/fairmq/Version.h",
               "FairMQ-1.3.7/fairmq/Version.h.in",
               substitutions = {"@PROJECT_VERSION@": "1.3.7-1",
                                "@PROJECT_VERSION_MAJOR@": "1",
                                "@PROJECT_VERSION_MINOR@": "3",
                                "@PROJECT_VERSION_PATCH@": "7",
                                "@PROJECT_VERSION_HOTFIX@": "1"},
               executable = False)

  #Generate a BUILD file for the LLVM dependency.
  ctx.file('BUILD', FAIRMQ_BUILD_FILE)

  #Generate an empty workspace file
  ctx.file('WORKSPACE', '')

get_fairmq = repository_rule(implementation = _fairmq_impl)
