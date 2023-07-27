// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <thread_pool.hh>

using namespace std::literals;

// define REPORT_TIMES 1

namespace cov {
	thread_pool::thread_pool(size_t size) {
		if (!size) size = 1;
		threads_.reserve(size);
		for (size_t index = 0; index < size; ++index)
			threads_.push_back(std::jthread{thread_proc, std::ref(tasks_)});
	}
	thread_pool::~thread_pool() {
		for (auto& thread : threads_)
			thread.request_stop();
		tasks_.wake();
	}

	std::atomic<size_t> counter{1};
	void thread_pool::thread_proc(std::stop_token tok,
	                              queue<std::function<void()>>& tasks) {
#if REPORT_TIMES
		using namespace std::chrono;
		size_t id{1}, internal{};
		steady_clock::duration runtime{};
		while (!counter.compare_exchange_weak(id, id + 1,
		                                      std::memory_order_relaxed))
			;
#endif
		while (!tok.stop_requested()) {
			std::function<void()> task;
			if (tasks.wait_and_pop(task, tok)) {
#if REPORT_TIMES
				++internal;
				auto const then = steady_clock::now();
#endif
				task();
#if REPORT_TIMES
				auto const now = steady_clock::now();
				runtime += now - then;
#endif
			}
		}

#if REPORT_TIMES
		auto const ms = duration_cast<milliseconds>(runtime);
		auto const cent_s = (ms.count() + 5) / 10;
		fmt::print(stderr, "[#{}] {} call{}, {}.{:02} s\n", id, internal,
		           internal == 1 ? ""sv : "s"sv, cent_s / 100, cent_s % 100);
#endif
	}
}  // namespace cov
