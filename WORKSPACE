workspace(name = "com_verylowsodium_wordle")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# abseil
http_archive(
    name = "absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

# folly
http_archive(
    name = "com_github_storypku_rules_folly",
    sha256 = "16441df2d454a6d7ef4da38d4e5fada9913d1f9a3b2015b9fe792081082d2a65",
    strip_prefix = "rules_folly-0.2.0",
    urls = [
        "https://github.com/storypku/rules_folly/archive/v0.2.0.tar.gz",
    ],
)

load("@com_github_storypku_rules_folly//bazel:folly_deps.bzl", "folly_deps")
folly_deps()

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()