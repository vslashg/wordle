cc_library(
    name = "dictionary",
    srcs = ["dictionary.cc"],
    hdrs = ["dictionary.h"],
)

cc_library(
    name = "state",
    hdrs = ["state.h"],
    deps = ["@absl//absl/numeric:bits", "@absl//absl/hash"],
)

cc_library(
    name = "color_guess",
    srcs = ["color_guess.cc"],
    hdrs = ["color_guess.h"],
    deps = [":dictionary"]
)

cc_library(
    name = "partition_map",
    srcs = ["partition_map.cc"],
    hdrs = ["partition_map.h"],
    deps = [
        ":dictionary",
        ":color_guess",
        ":state",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "thread_pool",
    hdrs = ["thread_pool.h"],
    deps = [
        "@absl//absl/functional:function_ref",
        "@absl//absl/algorithm:container",
        "@absl//absl/synchronization",
    ],
)

cc_library(
    name = "score",
    srcs = ["score.cc"],
    hdrs = ["score.h"],
    deps = [
        ":dictionary",
        ":partition_map",
        ":state",
    ]
)

cc_binary(
    name = "search",
    srcs = ["search.cc"],
    deps = [
        ":partition_map",
        ":color_guess",
        ":thread_pool",
        "@absl//absl/time",
        "@folly//:folly",
    ],
)

cc_binary(
    name = "solve",
    srcs = ["solve.cc"],
    deps = [
        ":partition_map",
        ":score",
    ],
)

cc_binary(
    name = "state_count",
    srcs = ["state_count.cc"],
    deps = [
        ":partition_map",
    ],
)