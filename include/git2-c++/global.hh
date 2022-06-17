#pragma once
#include "git2/global.h"

namespace git {
	struct init {
		init() { git_libgit2_init(); }
		~init() { git_libgit2_shutdown(); }

		init(const init&) : init() {}
		init(init&&) : init() {}

		init& operator=(const init&) = default;
		init& operator=(init&&) = default;
	};
}