#include "new.hh"
#include <cstdio>
#include <cstdlib>

static bool oom = false;
static size_t threshold = 0;

void set_oom(bool value, size_t next_threshold) {
	oom = value;
	threshold = next_threshold;
}

namespace {
	void* throw_bad_alloc() { throw std::bad_alloc(); }
	void* return_nullptr() noexcept { return nullptr; }
	void* alloc(size_t count, auto handler) noexcept(noexcept(handler())) {
		auto result = oom && count > threshold ? nullptr : std::malloc(count);
		if (oom) {
			fprintf(stderr, "count: (%c) %zu\n", count > threshold ? '-' : '+',
			        count);
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

#define FREE \
	noexcept { std::free(ptr); }
void operator delete(void* ptr)FREE;
void operator delete[](void* ptr) FREE;
void operator delete(void* ptr, std::align_val_t)FREE;
void operator delete[](void* ptr, std::align_val_t) FREE;
void operator delete(void* ptr, std::size_t)FREE;
void operator delete[](void* ptr, std::size_t) FREE;
void operator delete(void* ptr, std::size_t, std::align_val_t)FREE;
void operator delete[](void* ptr, std::size_t, std::align_val_t) FREE;

void operator delete(void* ptr, const std::nothrow_t&)FREE;
void operator delete[](void* ptr, const std::nothrow_t&) FREE;
void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&)FREE;
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
