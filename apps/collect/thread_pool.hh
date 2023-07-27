// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <functional>
#include <queue.hh>
#include <thread>

namespace cov {
	class thread_pool {
	public:
		thread_pool(size_t size = std::thread::hardware_concurrency());
		~thread_pool();

		void push(std::function<void()> const& task) { tasks_.push(task); }

	private:
		static void thread_proc(std::stop_token,
		                        queue<std::function<void()>>& tasks);
		queue<std::function<void()>> tasks_{};
		std::vector<std::jthread> threads_{};
	};
}  // namespace cov
