cc_library(
    name = "dictionary",
    srcs = ["dictionary.cc"],
    hdrs = ["dictionary.h"],
)

cc_library(
    name = "state",
    hdrs = ["state.h"],
    deps = ["@absl//absl/numeric:bits"],
)

cc_library(
    name = "score",
    srcs = ["score.cc"],
    hdrs = ["score.h"],
)

cc_library(
    name = "partition_map",
    srcs = ["partition_map.cc"],
    hdrs = ["partition_map.h"],
    deps = [
        ":dictionary",
        ":score",
        ":state",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_binary(
    name = "repl",
    srcs = ["repl.cc"],
    deps = [
        ":partition_map",
        ":score",
        "@absl//absl/time",
    ],
)

cc_binary(
    name = "solve",
    srcs = ["solve.cc"],
    deps = [
        ":partition_map",
    ],
)