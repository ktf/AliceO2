workspace(name="O2")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "com_github_nelhage_rules_boost",
    remote = "https://github.com/ktf/rules_boost",
    commit = "4acf275af3096931ef959ff97fcfa638751670da",
)


#new_git_repository(
#  name = "FairLogger",
#  remote = "https://github.com/FairRootGroup/FairLogger.git",
#  commit = "7d0411b",
#  build_file = "//:third_party/FairLogger.BUILD"
#)

http_archive(
  name = "FairLogger",
  strip_prefix = "FairLogger-1.3.0",
  url = "https://github.com/FairRootGroup/FairLogger/archive/v1.3.0.tar.gz",
  sha256 = "5cedea2773f7091d69aae9fd8f724e6e47929ee3784acdd295945a848eb36b93",
  build_file = "//:third_party/FairLogger.BUILD"
)

http_archive(
  name = "arrow",
  strip_prefix = "arrow-apache-arrow-0.12.0",
  url = "https://github.com/apache/arrow/archive/apache-arrow-0.12.0.tar.gz",
  sha256 = "e2eb87a68d200df5ff405fd7e0ab3d0549ccd5275f3a698439bb202918a000a8",
  build_file = "//:third_party/arrow.BUILD"
)

http_archive(
  name = "Monitoring",
  strip_prefix = "Monitoring-1.9.5",
  url = "https://github.com/AliceO2Group/Monitoring/archive/v1.9.5.tar.gz",
  sha256 = "a42631d2e017069d4e871ecd262db16e988db40e2798ab78f3f2e28451487538",
  build_file = "//:third_party/Monitoring.BUILD"
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

#http_archive(
#  name = "rules_foreign_cc",
#  strip_prefix = "rules_foreign_cc-master",
#  url = "https://github.com/bazelbuild/rules_foreign_cc/archive/master.zip",
#)
#
#load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")

#rules_foreign_cc_dependencies()
#
#all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""
#
#http_archive(
#  name = "FairMQ",
#  build_file_content = all_content,
#  strip_prefix = "FairMQ-1.2.7",
#  urls = ["https://github.com/FairRootGroup/FairMQ/archive/v1.2.7.tar.gz"],
#)

load("//:third_party/fairmq.bzl", "get_fairmq")
get_fairmq(name = "fairmq")

load("//:third_party/zeromq.bzl", "get_zeromq")
get_zeromq(name = "zeromq")

#http_archive(
#  name = "root",
#  strip_prefix = "root-6-14-00",
#  url = "https://github.com/root-project/root/archive/v6-14-00.tar.gz",
#  sha256 = "5c7b31479cf3734d5b090e7f552359d3e9601383ebddbf8f23114a788d62dff4",
#  build_file = "//:third_party/root/ROOT.BUILD"
#)

new_local_repository(
    name="root",
    build_file="//:third_party/root/ROOT.BUILD",
    path="/Users/ktf/work/active/ROOT"
    )


new_git_repository(
  name = "msgsl",
  remote = "https://github.com/Microsoft/GSL.git",
  commit = "b014508",
  build_file = "//:third_party/msgsl.BUILD"
)

http_archive(
  name = "llvm",
  build_file = "//third_party/llvm:llvm.autogenerated.BUILD",
  sha256 = "d522eda97835a9c75f0b88ddc81437e5edbb87dc2740686cb8647763855c2b3c",
  strip_prefix = "llvm-5.0.2.src",
  url = "http://releases.llvm.org/5.0.2/llvm-5.0.2.src.tar.xz"
)

http_archive(
  name = "zlib_archive",
  build_file = "//third_party:zlib.BUILD",
  sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
  strip_prefix = "zlib-1.2.11",
  urls = [
    "https://mirror.bazel.build/zlib.net/zlib-1.2.11.tar.gz",
    "https://zlib.net/zlib-1.2.11.tar.gz",
  ],
)



load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()
