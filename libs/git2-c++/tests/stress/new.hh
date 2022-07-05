// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>

void set_oom_as_threshold(size_t threshold);
void set_oom_as_limit(size_t limit);
void reset_oom();

struct emulate_oom_as_threshold {
	emulate_oom_as_threshold(size_t threshold) {
		set_oom_as_threshold(threshold);
	}
	~emulate_oom_as_threshold() { reset_oom(); }
};

struct emulate_oom_as_limit {
	emulate_oom_as_limit(size_t limit) { set_oom_as_limit(limit); }
	~emulate_oom_as_limit() { reset_oom(); }
};

#ifdef _WIN32
#define OOM_STR_THRESHOLD 16u
#else
#define OOM_STR_THRESHOLD 24u
#endif

#define OOM_BEGIN(THRESHOLD)                          \
	try {                                             \
		{                                             \
			emulate_oom_as_threshold here{THRESHOLD}; \
			{
#define OOM_LIMIT(LIMIT)                      \
	try {                                     \
		{                                     \
			emulate_oom_as_limit here{LIMIT}; \
			{
#define OOM_END                                \
	}                                          \
	}                                          \
	FAIL() << "Expected std::bad_alloc\n";     \
	}                                          \
	catch (std::bad_alloc&) {                  \
	}                                          \
	catch (...) {                              \
		FAIL() << "Expected std::bad_alloc\n"; \
	}
