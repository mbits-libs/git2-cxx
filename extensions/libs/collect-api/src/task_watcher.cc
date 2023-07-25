// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "task_watcher.hh"
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::literals;

namespace cov::app::collect {
	void task_watcher::print() {
		auto const now = clock::now();
		auto const duration = now - last_print_;
		if (duration < 1s) return;
		last_print_ = now;
		fmt::print(stderr, "[{}/{}] collecting...\r", finished_, total_);
		std::fflush(stderr);
	}

	void task_watcher::task_prepared() {
		std::lock_guard lock{m_};
		if (run_all_.stop_requested()) return;
		++counter_;
		++total_;
		print();
	}

	void task_watcher::task_finished() {
		std::lock_guard lock{m_};
		if (run_all_.stop_requested()) return;
		if (counter_) {
			++finished_;
			--counter_;
			if (!counter_) cv_.notify_one();
			print();
		}
	}

	void task_watcher::abort() {
		run_all_.request_stop();
		cv_.notify_one();
	}

	void task_watcher::wait() {
		std::unique_lock lock{m_};
		cv_.wait(lock,
		         [this] { return counter_ == 0 || run_all_.stop_requested(); });
		fmt::print(stderr, "[{}/{}] finished        \n", finished_, total_);
	}

	bool closure::set_return_code(int ret) {
		if (!ret) return false;
		int zero{};
		if (return_code.compare_exchange_weak(zero, ret,
		                                      std::memory_order_relaxed))
			fmt::print(stderr, "[report] aborting...\n");
		watcher.abort();
		return true;
	}

	void closure::task_finished() { watcher.task_finished(); }
	void closure::push(std::function<void()> const& task) {
		watcher.task_prepared();
		pool.push(task);
	}

	int closure::wait() {
		watcher.wait();
		return return_code;
	}
}  // namespace cov::app::collect
