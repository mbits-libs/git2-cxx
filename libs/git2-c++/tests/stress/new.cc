// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "new.hh"
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>

enum POLICY { USE_EVERYTHING, THRESHOLD, LIMIT };

struct terminator {
	static void abort() {
		fprintf(stderr, "\nKilled by std::terminate()\n");
		std::abort();
	}
	terminator() { std::set_terminate(abort); }
} install_terminate{};

static POLICY policy = POLICY::USE_EVERYTHING;
static size_t argument = 0;

void set_oom_as_threshold(size_t threshold) {
	fprintf(stderr, "threshold: %zu\n", threshold);
	policy = POLICY::THRESHOLD;
	argument = threshold;
}

void set_oom_as_limit(size_t limit) {
	fprintf(stderr, "limit: %zu\n", limit);
	policy = POLICY::LIMIT;
	argument = limit;
}

void reset_oom() {
	policy = POLICY::USE_EVERYTHING;
}

namespace {
	bool can_allocate(size_t size) {
		if (policy == POLICY::THRESHOLD) {
			if (size > argument) return false;
		} else if (policy == POLICY::LIMIT) {
			if (size > argument) return false;
			argument -= size;
		}
		return true;
	}

	void* throw_bad_alloc() {
		throw std::bad_alloc();
	}
	void* return_nullptr() noexcept {
		return nullptr;
	}
	void* alloc(size_t count, auto handler) noexcept(noexcept(handler())) {
		auto const saved = argument;
		auto allowed = can_allocate(count);
		auto result = !allowed ? nullptr : std::malloc(count);
		if (policy == POLICY::LIMIT) {
			fprintf(stderr, "count: (%c) %zu / %zu\n", allowed ? '+' : '-',
			        saved, count);
		} else if (policy == POLICY::THRESHOLD) {
			fprintf(stderr, "count: (%c) %zu\n", allowed ? '+' : '-', count);
		}
		if (!result) return handler();
		return result;
	}
}  // namespace

#define AS_BAD_ALLOC \
	{ return alloc(count, throw_bad_alloc); }

#define AS_NOTHROW \
	{ return alloc(count, return_nullptr); }

[[nodiscard]] void* operator new(std::size_t count) AS_BAD_ALLOC;
[[nodiscard]] void* operator new[](std::size_t count) AS_BAD_ALLOC;
[[nodiscard]] void* operator new(std::size_t count,
                                 std::align_val_t) AS_BAD_ALLOC;
[[nodiscard]] void* operator new[](std::size_t count,
                                   std::align_val_t) AS_BAD_ALLOC;

[[nodiscard]] void* operator new(std::size_t count,
                                 const std::nothrow_t&) noexcept AS_NOTHROW;
[[nodiscard]] void* operator new[](std::size_t count,
                                   const std::nothrow_t&) noexcept AS_NOTHROW;
[[nodiscard]] void* operator new(std::size_t count,
                                 std::align_val_t,
                                 const std::nothrow_t&) noexcept AS_NOTHROW;
[[nodiscard]] void* operator new[](std::size_t count,
                                   std::align_val_t,
                                   const std::nothrow_t&) noexcept AS_NOTHROW;

#define FREE            \
	noexcept {          \
		std::free(ptr); \
	}
void operator delete(void* ptr) FREE;
void operator delete[](void* ptr) FREE;
void operator delete(void* ptr, std::align_val_t) FREE;
void operator delete[](void* ptr, std::align_val_t) FREE;
void operator delete(void* ptr, std::size_t) FREE;
void operator delete[](void* ptr, std::size_t) FREE;
void operator delete(void* ptr, std::size_t, std::align_val_t) FREE;
void operator delete[](void* ptr, std::size_t, std::align_val_t) FREE;

void operator delete(void* ptr, const std::nothrow_t&) FREE;
void operator delete[](void* ptr, const std::nothrow_t&) FREE;
void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) FREE;
void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) FREE;

#include <gtest/gtest.h>
#include <string>

TEST(stress, allocation_works) {
	std::string allocate{
	    "very long, long, long, long, long, long, long, long, long, long, "
	    "long, long string"};
}

TEST(stress, oom_works) {
	OOM_BEGIN(OOM_STR_THRESHOLD);
	std::string allocate{
	    "long, long, long, long, long, long, long, long, long, long, long, "
	    "long string 1234567890"};
	OOM_END;
}
