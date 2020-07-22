workspace(name = "mesos")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")


all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""

#gtest_content = all_content + """
#filegroup(name = "gmock_all", srcs = [
#    "googletest/src/gtest-all.cc",
#    "googlemock/src/gmock-all.cc",
#    "googletest/src/gtest.cc",
#    "googlemock/src/gmock.cc"
#  ] + glob(["src/**/*.cc"]) + glob(["googletest/src/*.h", "googletest/include/**/*.h", "googlemock/include/**/*.h"]), visibility = ["//visibility:public"])
#"""

http_archive(
   name = "rules_foreign_cc",
   sha256 = "817c93454dce5feb0fc231c39300c016c74bb6acec223f35d052521b585d4e78",
   strip_prefix = "rules_foreign_cc-master",
   url = "https://github.com/ktf/rules_foreign_cc/archive/master.zip",
)

load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies([])

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
  name = "rules_pkg",
  url = "https://github.com/bazelbuild/rules_pkg/releases/download/0.2.5/rules_pkg-0.2.5.tar.gz",
  sha256 = "352c090cc3d3f9a6b4e676cf42a6047c16824959b438895a76c2989c6d7c246a",
)
load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
rules_pkg_dependencies()

http_archive(
  name = "python_sources",
  build_file_content = all_content,
  strip_prefix = "Python-3.8.2",
  sha256 = "e634a7a74776c2b89516b2e013dda1728c89c8149b9863b8cea21946daf9d561",
  urls = ["https://www.python.org/ftp/python/3.8.2/Python-3.8.2.tgz"]
)

http_archive(
  name = "boost_sources",
  build_file_content = all_content,
  strip_prefix = "boost_1_73_0",
  sha256 = "9995e192e68528793755692917f9eb6422f3052a53c5e13ba278a228af6c7acf",
  urls = ["https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.gz"]
)

http_archive(
  name = "elfio_sources",
  build_file_content = all_content,
  strip_prefix = "elfio-3.5",
  sha256 = "38215f17bb1b140c8afbf4c116d5303047619e70446dc393ef041f3c639a01cd",
  urls = ["https://github.com/serge1/ELFIO/releases/download/Release_3.5/elfio-3.5.tar.gz"]
)

http_archive(
   name = "m4_sources",
   build_file_content = all_content,
   strip_prefix = "m4-1.4.18/",
   sha256 = "ab2633921a5cd38e48797bf5521ad259bdc4b979078034a3b790d7fec5493fab",
   urls = ["https://ftp.gnu.org/gnu/m4/m4-1.4.18.tar.gz"]
)

http_archive(
   name = "uuid_sources",
   build_file_content = all_content,
   strip_prefix = "uuid-2.27.1/",
   sha256 = "1d61b760835118bc8b5ca24e9ff32206ef266f0ebaaf3c412c11a702b94d4b83",
   urls = ["https://github.com/alisw/uuid/archive/v2.27.1.tar.gz"],
   patch_cmds = ["autoreconf -ivf"]
)

http_archive(
   name = "libtool_sources",
   build_file_content = all_content,
   strip_prefix = "libtool-2.4.6/",
   sha256 = "e3bd4d5d3d025a36c21dd6af7ea818a2afcd4dfc1ea5a17b39d7854bcd0c06e3",
   urls = ["https://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.gz"]
)

http_archive(
   name = "curl_sources",
   build_file_content = all_content,
   strip_prefix = "curl-7.70.0/",
   sha256 = "ca2feeb8ef13368ce5d5e5849a5fd5e2dd4755fecf7d8f0cc94000a4206fb8e7",
   urls = ["https://curl.haxx.se/download/curl-7.70.0.tar.gz"]
)

http_archive(
   name = "zlib_sources",
   build_file_content = all_content,
   strip_prefix = "zlib-1.2.11/",
   urls = ["https://www.zlib.net/zlib-1.2.11.tar.gz"]
)

http_archive(
   name = "bison_sources",
   build_file_content = all_content,
   strip_prefix = "bison-3.5.4/",
   sha256 = "c0dd154dfaba63553a892d41dc400c7baa88cc06a1e2e27813fdd503715e4c28",
   urls = ["https://ftp.gnu.org/gnu/bison/bison-3.5.4.tar.gz"]
)

http_archive(
   name = "krb5_sources",
   build_file_content = all_content,
   sha256 = "02a4e700f10936f937cd1a4c303cab8687a11abecc6107bd4b706b9329cd5400",
   strip_prefix = "krb5-1.18.1/src/",
   urls = ["https://kerberos.org/dist/krb5/1.18/krb5-1.18.1.tar.gz"]
)

http_archive(
   name = "sasl2_sources",
   build_file_content = all_content,
   strip_prefix = "cyrus-sasl-2.1.27/",
   sha256 = "26866b1549b00ffd020f188a43c258017fa1c382b3ddadd8201536f72efb05d5",
   urls = ["https://github.com/cyrusimap/cyrus-sasl/releases/download/cyrus-sasl-2.1.27/cyrus-sasl-2.1.27.tar.gz"]
)

http_archive(
   name = "zookeeper_sources",
   build_file_content = all_content,
   strip_prefix = "zookeeper-3.4.14/zookeeper-client/zookeeper-client-c",
   urls = ["https://archive.apache.org/dist/zookeeper/zookeeper-3.4.14/zookeeper-3.4.14.tar.gz"],
   patches = ["//third_party/zookeeper:pkgconfig.patch"],
   patch_args = ["-p4"],
   patch_cmds = ["autoreconf -ivf"]
)

http_archive(
   name = "openssl_sources",
   build_file_content = all_content,
   sha256 = "ddb04774f1e32f0c49751e21b67216ac87852ceb056b75209af2443400636d46",
   strip_prefix = "openssl-1.1.1g",
   urls = ["https://www.openssl.org/source/openssl-1.1.1g.tar.gz"]
)

http_archive(
   name = "apr_sources",
   build_file_content = all_content,
   strip_prefix = "apr-1.7.0/",
   sha256 = "48e9dbf45ae3fdc7b491259ffb6ccf7d63049ffacbc1c0977cced095e4c2d5a2",
   urls = ["https://pub.tutosfaciles48.fr/mirrors/apache/apr/apr-1.7.0.tar.gz"]
)

http_archive(
   name = "sqlite_sources",
   build_file_content = all_content,
   strip_prefix = "sqlite-autoconf-3310100",
   urls = ["https://www.sqlite.org/2020/sqlite-autoconf-3310100.tar.gz"],
   sha256 = "62284efebc05a76f909c580ffa5c008a7d22a1287285d68b7825a2b6b51949ae"
)

http_archive(
   name = "apr_util_sources",
   sha256 = "b65e40713da57d004123b6319828be7f1273fbc6490e145874ee1177e112c459",
   build_file_content = all_content,
   strip_prefix = "apr-util-1.6.1/",
   urls = ["https://downloads.apache.org/apr/apr-util-1.6.1.tar.gz"]
)

http_archive(
   name = "expat_sources",
   build_file_content = all_content,
   strip_prefix = "expat-2.2.9/",
   sha256 = "4456e0aa72ecc7e1d4b3368cd545a5eec7f9de5133a8dc37fdb1efa6174c4947",
   urls = ["https://github.com/libexpat/libexpat/releases/download/R_2_2_9/expat-2.2.9.tar.gz"],
)

http_archive(
   name = "svn_sources",
   sha256 = "daad440c03b8a86fcca804ea82217bb1902cfcae1b7d28c624143c58dcb96931",
   build_file_content = all_content,
   strip_prefix = "subversion-1.13.0/",
   urls = ["https://mirror.ibcp.fr/pub/apache/subversion/subversion-1.13.0.tar.gz"],
)

http_archive(
   name = "glog_sources",
   build_file_content = all_content,
   strip_prefix = "glog-0.4.0/",
   sha256 = "f28359aeba12f30d73d9e4711ef356dc842886968112162bc73002645139c39c",
   urls = ["https://github.com/google/glog/archive/v0.4.0.tar.gz"],
   patch_cmds = ["./autogen.sh"],
)

http_archive(
   name = "leveldb_sources",
   build_file_content = all_content,
   strip_prefix = "leveldb-1.22/",
   sha256 = "55423cac9e3306f4a9502c738a001e4a339d1a38ffbee7572d4a07d5d63949b2",
   urls = ["https://github.com/google/leveldb/archive/1.22.tar.gz"],
)

http_archive(
   name = "com_github_google_googletest",
   strip_prefix = "googletest-release-1.10.0/",
   build_file = "@//:third_party/googletest/googletest.BUILD",
   urls = ["https://github.com/google/googletest/archive/release-1.10.0.tar.gz"],
)

http_archive(
   name = "mesos_sources",
   build_file_content = all_content,
   strip_prefix = "mesos-1.0.4/",
   sha256 = "f34438726b4ae3634d541bfec41f40201e7c986b54c31ec9a20088173061d881",
   urls = ["https://github.com/apache/mesos/archive/1.0.4.tar.gz"],
   patches = ["//third_party/mesos:sysmacros.patch"],
   patch_args = ["-p1"],
   patch_cmds = ["perl -p -i -e 's/elfio.h/elfio.hpp/g' configure.ac",
		 "perl -p -i -e 's/tar xf/tar --no-same-owner -xf/g' 3rdparty/stout/3rdparty/Makefile.am 3rdparty/Makefile.am 3rdparty/libprocess/3rdparty/Makefile.am", "./bootstrap"]
)
