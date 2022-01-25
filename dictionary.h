#pragma once

#include <string_view>

namespace wordle {

constexpr int kNumTargets = 2315;
constexpr int kNumNonTargets = 10657;

extern const char* targets[kNumTargets];
extern const char* non_targets[kNumNonTargets];

}  // namespace wordle