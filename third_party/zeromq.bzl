ZEROMQ_BUILD_FILE = """
#Build a library with all the zeromq code in it.
cc_library(
  name = "zmq",
  srcs = glob(["zeromq-4.2.5/src/*.cpp", "**/*.hpp", "**/*.h"]),
  hdrs = glob(["zeromq-4.2.5/include/**/*.hpp", "**/*.h"]),

  #Include the x86 target and all include files.
  #Add those under llvm - master / ... as well because only built files
  #seem to appear under include / ...
  copts = [
           "-Iinclude",
           "-Izeromq-4.2.5/include",
           ],

  #Include here as well, not sure whether this or copts is
  #actually doing the work.
  includes = [
    "include",
    "zeromq-4.2.5/include",
    ],
  visibility = ["//visibility:public"],
  #Currently picking up some gtest targets, I have that dependency
  #already, so just link it here until I filter those out.
  deps = [
  ],
)
    """

def _zeromq_impl(ctx):
  #Download LLVM master
  ctx.download_and_extract(url = "https://github.com/zeromq/libzmq/releases/download/v4.2.5/zeromq-4.2.5.tar.gz",
                           sha256 = "cc9090ba35713d59bb2f7d7965f877036c49c5558ea0c290b0dcc6f2a17e489f")

  #Run `cmake llvm - master` to generate configuration.
  ctx.execute(["cmake", "zeromq-4.2.5"])

  #The bazel - discuss thread says to delete llvm - master, but I've
  #found that only generated files are pulled out of master, so all
  #the non - generated ones get dropped if I delete this.
  #ctx.execute([ "rm", "-r", "llvm-master" ])

  #Generate a BUILD file for the LLVM dependency.
  ctx.file('BUILD', ZEROMQ_BUILD_FILE )

  #Generate an empty workspace file
  ctx.file('WORKSPACE', '')

get_zeromq = repository_rule(implementation = _zeromq_impl)
