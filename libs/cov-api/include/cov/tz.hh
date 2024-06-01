// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <date/tz.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace cov::init_magic {
	struct setup_tz_archives {
		setup_tz_archives();
	};
	static setup_tz_archives const setup_tz_archives_init{};
}  // namespace cov::init_magic
