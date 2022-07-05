// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstddef>

void set_oom(bool value, size_t threshold = 0);

struct emulate_oom {
	emulate_oom(size_t threshold) { set_oom(true, threshold); }
	~emulate_oom() { set_oom(false); }
};

#ifdef _WIN32
#define OOM_STR_THRESHOLD 16u
#else
#define OOM_STR_THRESHOLD 24u
#endif

#define OOM_BEGIN(THRESHOLD)             \
	try {                                \
		{                                \
			emulate_oom here{THRESHOLD}; \
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
