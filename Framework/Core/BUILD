# Zero dependencies
cc_library(
  name="DPLFoundation",
  srcs=["src/Variant.cxx"
        ],
  hdrs=[
    "include/Framework/CallbackService.h",
    "include/Framework/CallbackRegistry.h",
    "include/Framework/ServiceRegistry.h",
    "include/Framework/TypeTraits.h",
    "include/Framework/Variant.h",
  ],
  includes=["include"],
  deps=["@boost//:serialization", "@msgsl//:msgsl"]
)

# Only C++ & Arrow
cc_library(
  name="DPLArrow",
  deps=[
        "@arrow//:arrow",
        ":DPLFoundation"
       ],
  srcs=["include/Framework/TableBuilder.h",
        "include/Framework/TableConsumer.h",
        "src/TableBuilder.cxx",
        "src/TableConsumer.cxx"
        ],
  hdrs=["include/Framework/TableBuilder.h",
        "include/Framework/TableConsumer.h",
        ],
  includes=["include"]
)

cc_library(
  name="DPLCore",
  deps=["@fairmq//:FairMQ", "@root//:Core", "@root//:Net", "@msgsl//:msgsl", "//DataFormats/MemoryResources:MemoryResources",
        "//Common/Utils:CommonUtils",
        "//DataFormats/Headers:Headers",
        "@arrow//:arrow",
        "@Monitoring//:Monitoring", ":DPLFoundation"],
  srcs=glob(["include/Framework/*.h", "include/Framework/*.inc"]),
  includes=["include"]
)

cc_test(
  name="test_ROOT",
  srcs=["test/test_ROOT.cxx"],
  deps=["@root//:Core"],
)

cc_test(
  name="test_Variants",
  srcs=["test/test_Variants.cxx"],
  deps=["@boost//:test", ":DPLFoundation"],
#  visibility = ["//test:__pkg__"]
)

cc_test(
  name="test_TableBuilder",
  srcs=["test/test_TableBuilder.cxx"],
  deps=["@boost//:test", ":DPLArrow"],
)

cc_test(
  name="test_Services",
  srcs=["test/test_Services.cxx"],
  deps=["@boost//:test", ":DPLFoundation"],
)


cc_test(
  name="test_AlgorithmSpec",
  srcs=["test/test_AlgorithmSpec.cxx"],
  deps=["@boost//:test", ":DPLFoundation"],
#  visibility = ["//test:__pkg__"]
)


cc_test(
  name="test_SimpleStatefulProcessing01",
  srcs=["test/test_SimpleStatefulProcessing01.cxx"],
  deps=[":FrameworkCore", "@fairmq//:FairMQ"],
)
