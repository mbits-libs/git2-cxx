// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread_pool.hh>
#include <toolkit.hh>

namespace cov::app::collect {
	class task_watcher {
		using clock = std::chrono::steady_clock;
		std::mutex m_;
		size_t counter_{};
		size_t total_{};
		size_t finished_{};
		clock::time_point last_print_{};
		std::condition_variable cv_{};
		std::stop_source run_all_{};

	public:
		void print();
		void task_prepared();
		void task_finished();
		void abort();
		void wait();
	};

	struct closure {
		config const& cfg;
		coverage& cvg;
		thread_pool pool{std::thread::hardware_concurrency() * 2};
		std::atomic<int> return_code{0};
		task_watcher watcher{};

		bool set_return_code(int ret);
		void task_finished();
		void push(std::function<void()> const& task);
		int wait();
	};

}  // namespace cov::app::collect
