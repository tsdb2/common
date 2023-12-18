package(default_visibility = ["//visibility:public"])

cc_library(
    name = "reffed_ptr",
    hdrs = ["reffed_ptr.h"],
)

cc_test(
    name = "reffed_ptr_test",
    srcs = ["reffed_ptr_test.cc"],
    deps = [
        ":reffed_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "simple_condition",
    hdrs = ["simple_condition.h"],
    deps = [
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "simple_condition_test",
    srcs = ["simple_condition_test.cc"],
    deps = [
        ":simple_condition",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "blocking_ref_counted",
    hdrs = ["blocking_ref_counted.h"],
    deps = [
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "blocking_ref_counted_test",
    srcs = ["blocking_ref_counted_test.cc"],
    deps = [
        ":blocking_ref_counted",
        ":reffed_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "overridable",
    hdrs = ["overridable.h"],
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "overridable_test",
    srcs = ["overridable_test.cc"],
    deps = [
        ":overridable",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "clock",
    srcs = ["clock.cc"],
    hdrs = ["clock.h"],
    deps = [
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "clock_test",
    srcs = ["clock_test.cc"],
    deps = [
        ":clock",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_clock",
    testonly = True,
    srcs = ["mock_clock.cc"],
    hdrs = ["mock_clock.h"],
    deps = [
        ":clock",
        ":simple_condition",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "mock_clock_test",
    srcs = ["mock_clock_test.cc"],
    deps = [
        ":mock_clock",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)
