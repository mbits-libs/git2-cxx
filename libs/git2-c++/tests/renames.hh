// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/repository.hh>

namespace git::testing::renames {
	repository open_repo(std::error_code& ec);
}  // namespace git::testing::renames
