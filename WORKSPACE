workspace(name="O2")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""
arrow_all_content = """filegroup(name = "all", srcs = glob(["cpp/**"]), visibility = ["//visibility:public"])"""

http_archive(
  name = "rules_foreign_cc",
  strip_prefix = "rules_foreign_cc-master",
  url = "https://github.com/bazelbuild/rules_foreign_cc/archive/master.zip",
  sha256 = "bdfc2734367a1242514251c7ed2dd12f65dd6d19a97e6a2c61106851be8e7fb8"
)

load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

http_archive(
  name = "boost",
  build_file_content = all_content,
  strip_prefix = "boost_1_68_0",
  sha256 = "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf",
  urls = ["https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz"],
)

http_archive(
  name = "FairLogger",
  strip_prefix = "FairLogger-1.5.0",
  url = "https://github.com/FairRootGroup/FairLogger/archive/v1.5.0.tar.gz",
  sha256 = "8e74e0b1e50ee86f4fca87a44c6b393740b32099ac3880046bf252c31c58dd42",
  build_file_content = all_content
)

http_archive(
  name = "FairMQ",
  strip_prefix = "FairMQ-1.4.7",
  url = "https://github.com/FairRootGroup/FairMQ/archive/v1.4.7.tar.gz",
  sha256 = "a8243be49bea2fca46642f2fa059782a8c24379f5220d84c271630a0b4deb238",
  build_file_content = all_content
)

http_archive(
  name = "zeromq",
  strip_prefix = "zeromq-4.1.5",
  url = "https://github.com/zeromq/libzmq/releases/download/v4.3.2/zeromq-4.3.2.tar.gz",
  sha256 = "704ecd0e495192994419404c6c2d2bc955c9bcb1c00e002233101f17bcd71310",
  build_file_content = all_content,
  patch_cmds = ["./autogen.sh"],
)

http_archive(
  name = "double-conversion",
  strip_prefix = "double-conversion-3.1.5",
  url = "https://github.com/google/double-conversion/archive/v3.1.5.tar.gz",
  sha256 = "a63ecb93182134ba4293fd5f22d6e08ca417caafa244afaa751cbfddf6415b13",
)

http_archive(
  name = "flatbuffers",
  strip_prefix = "flatbuffers-1.11.0",
  url = "https://github.com/google/flatbuffers/archive/v1.11.0.tar.gz",
  sha256 = "3f4a286642094f45b1b77228656fbd7ea123964f19502f9ecfd29933fd23a50b",
)

http_archive(
  name = "RapidJSON",
  strip_prefix = "rapidjson-1.1.0",
  url = "https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz",
  sha256 = "bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e",
  build_file_content = all_content,
)

http_archive(
  name = "arrow",
  strip_prefix = "arrow-apache-arrow-0.14.0",
  url = "https://github.com/apache/arrow/archive/apache-arrow-0.14.0.tar.gz",
  sha256 = "e6444a73cc7987245e0c89161e587337469d26a518c9af1e6d7dba47027e0cd1",
#  build_file_content = arrow_all_content,
  build_file = "//:third_party/arrow.BUILD"
)

http_archive(
  name = "ROOT",
  strip_prefix = "root-6-18-04",
  url = "https://github.com/root-project/root/archive/v6-18-04.tar.gz",
  sha256 = "82421a5f0486a2c66170300285dce49a961e3459cb5290f6fa579ef617dc8b0a",
  build_file_content = all_content,
)

http_archive(
  name = "Monitoring",
  strip_prefix = "Monitoring-1.9.5",
  url = "https://github.com/AliceO2Group/Monitoring/archive/v1.9.5.tar.gz",
  sha256 = "a42631d2e017069d4e871ecd262db16e988db40e2798ab78f3f2e28451487538",
)


#http_archive(
#  name = "root",
#  strip_prefix = "root-6-14-00",
#  url = "https://github.com/root-project/root/archive/v6-14-00.tar.gz",
#  sha256 = "5c7b31479cf3734d5b090e7f552359d3e9601383ebddbf8f23114a788d62dff4",
#  build_file = "//:third_party/root/ROOT.BUILD"
#)

#new_local_repository(
#    name="root",
#    build_file="//:third_party/root/ROOT.BUILD",
#    path="/Users/ktf/work/active/ROOT"
#    )
#
#
#new_git_repository(
#  name = "msgsl",
#  remote = "https://github.com/Microsoft/GSL.git",
#  commit = "b014508",
#  build_file = "//:third_party/msgsl.BUILD"
#)
#
#http_archive(
#  name = "llvm",
#  build_file = "//third_party/llvm:llvm.autogenerated.BUILD",
#  sha256 = "d522eda97835a9c75f0b88ddc81437e5edbb87dc2740686cb8647763855c2b3c",
#  strip_prefix = "llvm-5.0.2.src",
#  url = "http://releases.llvm.org/5.0.2/llvm-5.0.2.src.tar.xz"
#)
#
#http_archive(
#  name = "zlib_archive",
#  build_file = "//third_party:zlib.BUILD",
#  sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
#  strip_prefix = "zlib-1.2.11",
#  urls = [
#    "https://mirror.bazel.build/zlib.net/zlib-1.2.11.tar.gz",
#    "https://zlib.net/zlib-1.2.11.tar.gz",
#  ],
#)
#
#
#
