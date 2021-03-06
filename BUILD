cc_library(
    name = "dictionary",
    srcs = ["dictionary.cc"],
    hdrs = ["dictionary.h"],
)

cc_library(
    name = "state",
    srcs = ["state.cc"],
    hdrs = ["state.h"],
    deps = [
        ":dictionary",
        "@absl//absl/hash",
        "@absl//absl/numeric:bits",
    ],
)

cc_library(
    name = "color_guess",
    srcs = ["color_guess.cc"],
    hdrs = ["color_guess.h"],
    deps = [":dictionary"],
)

cc_binary(
    name = "generate_tables",
    srcs = ["generate_tables.cc"],
    deps = [
        ":color_guess",
        ":dictionary",
        ":state",
        "@absl//absl/container:flat_hash_map",
    ],
)

genrule(
    name = "raw_data_source",
    srcs = [],
    outs = ["raw_data.cc"],
    cmd = "./$(execpath :generate_tables) > $@",
    tools = [":generate_tables"],
)

cc_library(
    name = "raw_data",
    srcs = [":raw_data.cc"],
    hdrs = ["raw_data.h"],
    deps = [
        ":color_guess",
        ":dictionary",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "partition_map",
    srcs = ["partition_map.cc"],
    hdrs = ["partition_map.h"],
    deps = [
        ":color_guess",
        ":dictionary",
        ":raw_data",
        ":state",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "thread_pool",
    hdrs = ["thread_pool.h"],
    deps = [
        "@absl//absl/algorithm:container",
        "@absl//absl/functional:function_ref",
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
        ":reduced_map",
        ":state",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/synchronization",
    ],
)

cc_library(
    name = "reduced_map",
    hdrs = ["reduced_map.h"],
    deps = [],
)

cc_binary(
    name = "search",
    srcs = ["search.cc"],
    deps = [
        ":color_guess",
        ":partition_map",
        ":score",
        ":thread_pool",
        "@absl//absl/time",
        "@folly",
    ],
)

cc_binary(
    name = "solve",
    srcs = ["solve.cc"],
    deps = [
        ":partition_map",
        ":reduced_map",
        ":score",
        "@absl//absl/time",
    ],
)

cc_binary(
    name = "state_count",
    srcs = ["state_count.cc"],
    deps = [
        ":partition_map",
        ":score",
        ":state",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/synchronization",
    ],
)

cc_binary(
    name = "pure_count",
    srcs = ["pure_count.cc"],
    deps = [
        ":partition_map",
        ":score",
        ":state",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/synchronization",
    ],
)

