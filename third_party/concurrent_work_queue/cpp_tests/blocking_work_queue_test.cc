// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

//===- blocking_work_queue_test.cc ------------------------------*- C++ -*-===//
//
// Unit tests and benchmarks for BlockingWorkQueue.
//
//===----------------------------------------------------------------------===//

#include "blocking_work_queue.h"

#include "benchmark/benchmark.h"
#include "environment.h"
#include "llvm/Support/FormatVariadic.h"
#include "tfrt/host_context/task_function.h"
#include "tfrt/support/latch.h"

namespace tfrt {
namespace {

using ThreadingEnvironment = ::tfrt::internal::StdThreadingEnvironment;
using WorkQueue = ::tfrt::internal::BlockingWorkQueue<ThreadingEnvironment>;

// Benchmark work queue throughput.
//
// Submit `num_producers` tasks to `producer` work queue, each submitting
// `num_tasks` no-op tasks into the `worker` work queue.
//
// Mesures the time to complete all the tasks inside the `worker` queue.
void NoOp(WorkQueue& producer, WorkQueue& worker, benchmark::State& state) {
  const int num_producers = state.range(0);
  const int num_tasks = state.range(1);

  std::atomic<int> num_overflow = 0;

  for (auto _ : state) {
    ::tfrt::latch latch(num_producers);

    std::atomic<int>* counters = new std::atomic<int>[num_producers];
    for (int i = 0; i < num_producers; ++i) counters[i] = num_tasks;

    // Submit `num_producers` tasks to `producer` queue, each submitting
    // `num_tasks` to `worker` queue.
    for (int i = 0; i < num_producers; ++i) {
      auto producer_overflow = producer.AddBlockingTask(TaskFunction([&, i] {
        for (int j = 0; j < num_tasks; ++j) {
          auto worker_overflow = worker.AddBlockingTask(TaskFunction([&, i]() {
            if (counters[i].fetch_sub(1) == 1) latch.count_down();
          }));

          if (worker_overflow) {
            (*worker_overflow)();
            num_overflow++;
          }
        }
      }));
      if (producer_overflow) (*producer_overflow)();
    }

    latch.wait();
    delete[] counters;
  }

  std::string label = llvm::formatv("overflow: {0} / {1}", num_overflow,
                                    num_producers * num_tasks);
  state.SetLabel(label);
  state.SetItemsProcessed(num_producers * num_tasks * state.iterations());
}

#define BM_Run(FN, producer_threads, worker_threads)                       \
  static void BM_##FN##_tpool_##producer_threads##x##worker_threads(       \
      benchmark::State& state) {                                           \
    BenchmarkUseRealTime();                                                \
    WorkQueue producer(producer_threads, 1024, std::chrono::seconds(1));   \
    WorkQueue worker(worker_threads, 100 * 1024, std::chrono::seconds(1)); \
    FN(producer, worker, state);                                           \
  }                                                                        \
  BENCHMARK(BM_##FN##_tpool_##producer_threads##x##worker_threads)

#define BM_NoOp(producer_threads, worker_threads) \
  BM_Run(NoOp, producer_threads, worker_threads)  \
      ->ArgPair(10, 10)                           \
      ->ArgPair(10, 100)                          \
      ->ArgPair(10, 1000)                         \
      ->ArgPair(10, 10000)                        \
      ->ArgPair(100, 10)                          \
      ->ArgPair(100, 100)                         \
      ->ArgPair(100, 1000)

BM_NoOp(4, 4);
BM_NoOp(8, 8);
BM_NoOp(16, 16);
BM_NoOp(32, 32);

}  // namespace
}  // namespace tfrt