// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/diff.h>

namespace git {
	GIT_PTR_FREE(git_diff);

	struct deltas_iterator {
		using difference_type = std::make_signed<size_t>;
		using value_type = git_diff_delta const;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::input_iterator_tag;

		git_diff* ptr{};
		size_t index{};

		bool operator==(deltas_iterator const&) const = default;
		pointer get() const noexcept { return git_diff_get_delta(ptr, index); }

		deltas_iterator& operator++() noexcept {
			++index;
			return *this;
		}
		deltas_iterator operator++(int) noexcept {
			auto const prev = index;
			++index;
			return {ptr, prev};
		}
		reference operator*() const noexcept { return *get(); }
		pointer operator->() const noexcept { return get(); }
	};

	struct deltas_type {
		git_diff* ptr{};

		size_t size() const noexcept { return git_diff_num_deltas(ptr); }
		git_diff_delta const& at(size_t index) const noexcept {
			return *git_diff_get_delta(ptr, index);
		}
		git_diff_delta const& operator[](size_t index) const noexcept {
			return at(index);
		}
		deltas_iterator begin() const noexcept { return {ptr, 0u}; }
		deltas_iterator end() const noexcept { return {ptr, size()}; }
	};

	struct diff : ptr<git_diff> {
		using ptr<git_diff>::ptr;  // GCOV_EXCL_LINE -- another inlined ctor

		deltas_type deltas() const noexcept { return {get()}; }

		int find_similar(
		    git_diff_find_options const* opts = nullptr) const noexcept {
			return git_diff_find_similar(get(), opts);
		}
	};
}  // namespace git
