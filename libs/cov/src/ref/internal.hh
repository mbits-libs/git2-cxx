// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/reference.hh>
#include <filesystem>
#include <string>

namespace cov {
	enum class ref_tgt : int {
		other,
		branch,
		tag,
	};
}  // namespace cov
