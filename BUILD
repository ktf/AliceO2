load("@rules_foreign_cc//tools/build_defs:cmake.bzl", "cmake_external")
load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")
load("@rules_foreign_cc//tools/build_defs:boost_build.bzl", "boost_build")

boost_build(
  name = "boost",
  lib_source = "@boost_sources//:all",
  user_options = [
	"--with-atomic"
  ],
  static_libraries = [
      "libboost_atomic.a",
  ],
)

configure_make(
    name = "m4",
    configure_options = [
        "--disable-dependecy-tracking",
        "--disable-shared",
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@m4_sources//:all",
)

configure_make(
    name = "elfio",
    configure_options = [
        "--disable-dependecy-tracking",
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@elfio_sources//:all",
    headers_only = True,
)

configure_make(
    name = "glog",
    configure_options = [
        "--disable-dependecy-tracking",
        "--disable-shared",
      	"CFLAGS='-O2 -fPIC -Dredacted=\"redacted\"'",
      	"CXXFLAGS='-O2 -fPIC -Dredacted=\"redacted\"'",
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@glog_sources//:all",
    static_libraries = [
        "libglog.a",
    ],
)

configure_make(
    name = "zlib",
    configure_options = [
    ],
    configure_env_vars = {
    #     "CFLAGS": "'-O2 -fPIC -Dredacted=\"redacted\"'",
    },
    lib_source = "@zlib_sources//:all",
    static_libraries = [
        "libz.a",
    ],
)

cmake_external(
    name = "leveldb",
    lib_source = "@leveldb_sources//:all",
    cache_entries = {
	"LEVELDB_BUILD_TESTS": "NO",
    },
    static_libraries = [
        "libleveldb.a",
    ],
)

configure_make(
    name = "libtool_pkg",
    configure_options = [
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@libtool_sources//:all",
    binaries = ["libtool"]
)

configure_make(
    name = "bison",
    configure_options = [
        "--disable-dependecy-tracking",
        "--disable-shared",
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@bison_sources//:all",
    binaries = ["bison"],
    static_libraries = [
        "liby.a",
    ],
)

configure_make(
    name = "sqlite",
    configure_options = [
        "--disable-dependecy-tracking",
        "--disable-shared",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@sqlite_sources//:all",
    static_libraries = [
        "libsqlite3.a",
    ],
)

configure_make(
    name = "openssl",
    configure_command = "config",
    configure_options = [
        "no-shared",
    ],
    lib_source = "@openssl_sources//:all",
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = [
        "libcrypto.a",
        "libssl.a",
    ],
)

configure_make(
    name = "apr",
    configure_options = [
        "--enable-shared=no",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
    ],
    configure_env_vars = {
      "AR": "",
    },
    lib_source = "@apr_sources//:all",
    static_libraries = ["libapr-1.a"],
)

configure_make(
    name = "apr_util",
    lib_source = "@apr_util_sources//:all",
    deps = [":apr"],
    configure_options = [
        "--with-apr=$$EXT_BUILD_DEPS$$/apr",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
        ],
    configure_env_vars = {
      "AR": "",
      "CPP": "cpp",
      "LIBTOOL": "libtool",
    },
    static_libraries = ["libaprutil-1.a"],
)

configure_make(
    name = "curl",
    lib_source = "@curl_sources//:all",
    deps = [":openssl"],
    configure_options = [
	"--disable-shared",
	"--disable-ldap",
	"--enable-static",
        "--with-openssl=$$EXT_BUILD_DEPS$$/openssl",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
        ],
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = ["libcurl.a"],
)

configure_make(
    name = "krb5",
    lib_source = "@krb5_sources//:all",
    deps = [":openssl", ":bison"],
    configure_options = [
        "--disable-shared",
	"--enable-static",
        "--with-openssl=$$EXT_BUILD_DEPS$$/openssl",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
	"YACC=\"$$EXT_BUILD_DEPS$$/bison/bin/bison -y\""
    ],
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = ["libkrb5.a"],
)

configure_make(
    name = "sasl2",
    lib_source = "@sasl2_sources//:all",
    deps = [":openssl", ":krb5"],
    configure_options = [
        "--disable-shared",
        "--with-openssl=$$EXT_BUILD_DEPS$$/openssl",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
    ],
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = ["libsasl2.a"],
)

configure_make(
    name = "uuid",
    lib_source = "@uuid_sources//:all",
    deps = [],
    configure_options = [
        "--disable-shared",
	"--disable-all-programs",
	"--disable-silent-rules",    
	"--disable-tls",  
	"--disable-rpath", 
	"--without-ncurses",
	"--enable-libuuid",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
    ],
    make_commands = [
      "mkdir $$BUILD_TMPDIR$$/uuid",
      "make libuuid.la",
      "mkdir -p $$INSTALLDIR$$/lib",
      "cp .libs/libuuid.* $$INSTALLDIR$$/lib"
    ],
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = ["libuuid.a"],
)

configure_make(
    name = "expat",
    lib_source = "@expat_sources//:all",
    deps = [],
    configure_options = [
        "--disable-shared",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
    ],
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = ["libexpat.a"],
)

configure_make(
    name = "zookeeper",
    lib_source = "@zookeeper_sources//:all",
    deps = [],
    configure_options = [
        "--disable-shared",
	"--without-cppunit",
        "CFLAGS='-w -fPIC -Dredacted=\"redacted\"'",
    ],
    configure_env_vars = {
      "AR": "",
    },
    static_libraries = ["libzookeeper_mt.a"],
)

configure_make(
    name = "svn",
    lib_source = "@svn_sources//:all",
    deps = [":apr", ":apr_util", ":sqlite"],
    configure_options = [
      	"--disable-shared",
      	"--enable-static",
	"--enable-all-static",
	"--without-swig",
        "--with-apr=$$EXT_BUILD_DEPS$$/apr",
        "--with-apr-util=$$EXT_BUILD_DEPS$$/apr_util",
        "--with-lz4=internal",
        "--with-sqlite=$$EXT_BUILD_DEPS$$/sqlite",
        "--with-utf8proc=internal",
	"--without-apache-libexecdir",
	"--disable-mod-activation",
	"--disable-debug",
	"--without-jikes",
	"--with-apxs=no",
        "CFLAGS='-fPIC -Dredacted=\"redacted\"'",
        "LIBS='-ldl -lm'"
        ],
    configure_env_vars = {
      "AR": "",
      "CPP": "cpp",
      "LIBTOOL": "$$EXT_BUILD_DEPS$$/libtool_pkg/bin/libtool",
    },
    static_libraries = ["libsvn_subr-1.a", "libsvn_delta-1.a"],
)

configure_make(
    name = "python",
    lib_source = "@python_sources//:all",
    deps = [":sqlite", ":openssl"],
    configure_options = [
      "--disable-shared",
      "--with-openssl=$$EXT_BUILD_DEPS$$/openssl",
      "--with-sqlite=$$EXT_BUILD_DEPS$$/sqlite",
      "CFLAGS='-fPIC -Dredacted=\"redacted\"'"
        ],
    configure_env_vars = {
      "AR": "",
      "CPP": "cpp",
    },
    out_lib_dir = "lib",
    static_libraries = ["libpython2.7.a"],
    configure_in_place = True
)

configure_make(
   name = "mesos",
   # Values to be passed as -Dkey=value on the CMake command line
   # here are serving to provide some CMake script configuration options
   lib_source = "@mesos_sources//:all",
   deps = [":apr", ":svn", ":curl", ":openssl", ":sasl2", 
	   ":expat", ":uuid", ":elfio", ":glog", ":zookeeper",
	   ":leveldb", ":zlib"],
   configure_options = [
       "--sbindir=$$BUILD_TMPDIR$$/$$INSTALL_PREFIX$$/bin",
       "--with-apr=$$EXT_BUILD_DEPS$$/apr",
       "--with-svn=$$EXT_BUILD_DEPS$$/svn",
       "--with-curl=$$EXT_BUILD_DEPS$$/curl",
       "--with-sasl=$$EXT_BUILD_DEPS$$/sasl2",
       "--with-ssl=$$EXT_BUILD_DEPS$$/openssl",
#       "--with-boost=$$EXT_BUILD_DEPS$$/boost",
       "--with-elfio=$$EXT_BUILD_DEPS$$/elfio",
       "--with-glog=$$EXT_BUILD_DEPS$$/glog",
       "--with-zookeeper=$$EXT_BUILD_DEPS$$/zookeeper",
       "--with-leveldb=$$EXT_BUILD_DEPS$$/leveldb",
       "--with-zlib=$$EXT_BUILD_DEPS$$/zlib",
#       "--with-gmock=$$EXT_BUILD_DEPS$$/gtest",
       "--disable-java",
       "--disable-python",
       "--disable-shared",
       "--enable-static",
       "--enable-tests-install",
       "--disable-maintainer-mode",
       "CFLAGS='-w -pthread -fPIC -Dredacted=\"redacted\"'",
       "CXXFLAGS='-w -pthread -fPIC -Dredacted=\"redacted\"'"
   ],
   configure_env_vars = {
     "AR": "",
     "CXXFLAGS": "-O2 -fPIC -pthread",
     "TAR": "tar --no-same-owner",
     "LDFLAGS": "-L$$EXT_BUILD_DEPS$$/expat/lib -L$$EXT_BUILD_DEPS$$/uuid/lib -I$$EXT_BUILD_DEPS$$/gtest/include -I$$EXT_BUILD_DEPS$$/gtest/src",
     "LIBS": "-ldl -laprutil-1 -lapr-1 -lexpat -luuid -lssl -lcrypto",
   },
   make_commands = ["make -j 20", "make install"],

   # We are selecting the resulting static library to be passed in C/C++ provider
   # as the result of the build;
   # However, the cmake_external dependants could use other artefacts provided by the build,
   # according to their CMake script
   static_libraries = ["libmesos.a"],
   binaries = ["mesos-slave", 
	       "mesos-agent",
	       "mesos-cat",
	       "mesos-daemon.sh",
	       "mesos-execute",
	       "mesos-local",
	       "mesos-log",
	       "mesos-master",
	       "mesos-ps",
	       "mesos-resolve",
	       "mesos-scp",
	       "mesos-slave",
	       "mesos-tail",
	       "mesos-master"],
)

filegroup(
  name = "mesos-standard-config",
  srcs = glob(["mesos-agent-config/etc/**"]),
)

genrule(
  name = "copy-config",
  srcs = [":mesos-standard-config"],
  cmd = "ls && ls mesos-agent-config && mkdir $@ && rsync -av mesos-agent-config/etc/ $@/",
  outs = ["//:mesos-install/etc"]
)

genrule(
  name = "copy-mesos",
  srcs = [":mesos"],
  cmd = "mkdir $@ && rsync -av bazel-out/k8-fastbuild/bin/copy_mesos/mesos/ $@/ && rm -rf $@/libexec/mesos/tests/",
  outs = ["//:mesos-install/usr"]
)

genrule(
  name = "copy-aurora",
  srcs = ["//:third_party/aurora/aurora-executor_0.17.0_amd64.deb"],
  cmd = "ar xv $(location //:third_party/aurora/aurora-executor_0.17.0_amd64.deb) && xz -d data.tar.xz && tar xvf data.tar && cp -r usr/share/aurora/bin/ $@",
  outs = ["//:mesos-install/copy-aurora"]
)

load("@rules_pkg//:pkg.bzl", "pkg_tar", "pkg_deb")

pkg_tar(
  name = "mesos_tar",
  package_dir = "/",
  srcs = ["//:copy-config", "//:copy-mesos", "//:copy-aurora"],
  remap_paths = {
    "mesos/bin/": "usr/bin/",
    "mesos/lib/": "usr/lib/",
    "mesos/include": "usr/include",
    "copy-aurora/": "usr/",
  },
  mode = "0755",
)

pkg_deb(
  name = "alice-build-cluster",
  architecture = "amd64",
  built_using = "unzip (6.0.1)",
  data = ":mesos_tar",
  depends = [
    "unzip",
  ],
  description = "ALICE Build Cluster setup",
  homepage = "http://alisw.github.io",
  maintainer = "Giulio Eulisse <giulio.eulisse@cern.ch>",
  package = "alice-build-cluster",
  version = "0.1.0",
)
