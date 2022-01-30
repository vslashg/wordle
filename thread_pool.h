#pragma once

#include <algorithm>
#include <functional>
#include <thread>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/functional/function_ref.h"
#include "absl/synchronization/mutex.h"

namespace wordle {

void RunThreads(int num_threads, std::vector<std::function<int()>> fns,
                absl::FunctionRef<void(int)> callback) {
  if (num_threads <= 1) {
    for (const auto& fn : fns) {
      callback(fn());
    }
    return;
  }

  // Reverse the vector so we can pop things off the back.
  absl::c_reverse(fns);
  absl::Mutex in_mutex;
  absl::Mutex out_mutex;
  num_threads = std::min<int>(num_threads, fns.size());
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&] {
      while (true) {
        std::function<int()> fn;
        {
          absl::MutexLock mu(&in_mutex);
          if (fns.empty()) {
            return;
          }
          fn = std::move(fns.back());
          fns.pop_back();
        }
        int ret = fn();
        {
          absl::MutexLock mu(&out_mutex);
          callback(ret);
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

}  // namespace wordle