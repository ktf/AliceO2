cc_library(
  name="Foundation",
  srcs=["src/Traits.cxx"
        ],
  hdrs=[
    "include/Framework/CompilerBuiltins.h",
    "include/Framework/Traits.h",
    "include/Framework/TypeIdHelpers.h",
    "include/Framework/FunctionalHelpers.h",
    "include/Framework/Signpost.h",
    "include/Framework/VariantHelpers.h",
    "include/Framework/StructToTuple.h",
  ],
  includes=["include"],
  deps=[],
  visibility = ["//visibility:public"],
)

cc_test(
  name="test_CompilerBuiltins",
  srcs=["test/test_CompilerBuiltins.cxx"],
  deps=[":Foundation"],
)

cc_test(
  name="test_FunctionalHelpers",
  srcs=["test/test_FunctionalHelpers.cxx"],
  deps=[":Foundation"],
)

cc_test(
  name="test_StructToTuple",
  srcs=["test/test_StructToTuple.cxx"],
  deps=[":Foundation"],
)

cc_test(
  name="test_Traits",
  srcs=["test/test_Traits.cxx"],
  deps=[":Foundation"],
)
