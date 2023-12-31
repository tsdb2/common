package(default_visibility = ["//visibility:public"])

cc_library(
    name = "to_array",
    hdrs = ["to_array.h"],
)

cc_library(
    name = "fixed",
    hdrs = ["fixed.h"],
)

cc_library(
    name = "testing",
    testonly = True,
    hdrs = ["testing.h"],
)

cc_library(
    name = "flat_container_internal",
    hdrs = ["flat_container_internal.h"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "flat_container_testing",
    testonly = True,
    hdrs = ["flat_container_testing.h"],
    visibility = ["//visibility:private"],
    deps = [
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "flat_set",
    hdrs = ["flat_set.h"],
    deps = [
        ":flat_container_internal",
        ":to_array",
        "@com_google_absl//absl/base",
    ],
)

cc_test(
    name = "flat_set_test",
    srcs = ["flat_set_test.cc"],
    deps = [
        ":fixed",
        ":flat_container_testing",
        ":flat_set",
        "@com_google_absl//absl/hash",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "flat_map",
    hdrs = ["flat_map.h"],
    deps = [
        ":flat_container_internal",
        ":to_array",
        "@com_google_absl//absl/base",
    ],
)

cc_test(
    name = "flat_map_test",
    srcs = ["flat_map_test.cc"],
    deps = [
        ":flat_container_testing",
        ":flat_map",
        "@com_google_absl//absl/hash",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "murmur3",
    srcs = ["murmur3.cc"],
    hdrs = ["murmur3.h"],
)

cc_test(
    name = "murmur3_test",
    srcs = ["murmur3_test.cc"],
    deps = [
        ":murmur3",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fingerprint",
    hdrs = ["fingerprint.h"],
    deps = [
        ":flat_map",
        ":flat_set",
        ":murmur3",
        "@com_google_absl//absl/types:span",
    ],
)

cc_test(
    name = "fingerprint_test",
    srcs = ["fingerprint_test.cc"],
    deps = [
        ":fingerprint",
        ":flat_set",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest_main",
    ],
)

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
    name = "ref_count",
    hdrs = ["ref_count.h"],
    deps = [
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "ref_count_test",
    srcs = ["ref_count_test.cc"],
    deps = [
        ":ref_count",
        ":reffed_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sequence_number",
    hdrs = ["sequence_number.h"],
)

cc_test(
    name = "sequence_number_test",
    srcs = ["sequence_number_test.cc"],
    deps = [
        ":sequence_number",
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
        ":reffed_ptr",
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
    name = "no_destructor",
    hdrs = ["no_destructor.h"],
)

cc_test(
    name = "no_destructor_test",
    srcs = ["no_destructor_test.cc"],
    deps = [
        ":no_destructor",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "singleton",
    hdrs = ["singleton.h"],
    deps = [
        ":no_destructor",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "singleton_test",
    srcs = ["singleton_test.cc"],
    deps = [
        ":scoped_override",
        ":singleton",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "scoped_override",
    testonly = True,
    hdrs = ["scoped_override.h"],
)

cc_test(
    name = "scoped_override_test",
    srcs = ["scoped_override_test.cc"],
    deps = [
        ":overridable",
        ":scoped_override",
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
        ":flat_map",
        ":simple_condition",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/functional:function_ref",
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

cc_library(
    name = "scheduler",
    srcs = ["scheduler.cc"],
    hdrs = ["scheduler.h"],
    deps = [
        ":clock",
        ":sequence_number",
        ":simple_condition",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "scheduler_test",
    srcs = ["scheduler_test.cc"],
    deps = [
        ":mock_clock",
        ":scheduler",
        ":simple_condition",
        ":testing",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "periodic_thread",
    hdrs = ["periodic_thread.h"],
    deps = [
        ":clock",
        ":scheduler",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "periodic_thread_test",
    srcs = ["periodic_thread_test.cc"],
    deps = [
        ":mock_clock",
        ":periodic_thread",
        ":testing",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)
