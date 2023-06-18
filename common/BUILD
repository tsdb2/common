package(default_visibility = ["//visibility:public"])

cc_library(
    name = "clock",
    srcs = ["clock.cc"],
    hdrs = ["clock.h"],
    deps = [
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "clock_test",
    srcs = ["clock_test.cc"],
    deps = [
        ":clock",
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
        "@com_google_absl//absl/base",
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
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)