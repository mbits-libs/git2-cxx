// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <c++filt/expression.hh>
#include <c++filt/parser.hh>
#include <cov/repository.hh>
#include <filesystem>
#include <vector>

namespace cov::core {
	cxx_filt::Replacements load_replacements(
	    std::filesystem::path const& system,
	    cov::repository const& repo,
	    bool debug = false);
};  // namespace cov::core
