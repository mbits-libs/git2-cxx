// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/discover.hh>
#include <cov/init.hh>
#include <git2/config.hh>

namespace cov {
	struct repository {
		~repository();

		std::filesystem::path const& commondir() const noexcept {
			return commondir_;
		}

		git::config const& config() const noexcept { return cfg_; }

		static std::filesystem::path discover(
		    std::filesystem::path const& current_dir,
		    discover across_fs = discover::within_fs) {
			return discover_repository(current_dir, across_fs);
		}

		static repository init(std::filesystem::path const& base,
		                       std::filesystem::path const& git_dir,
		                       std::error_code& ec,
		                       init_options const& options = {}) {
			auto common = init_repository(base, git_dir, ec, options);
			if (ec) return repository{};
			return repository{common, ec};
		}

		static repository open(std::filesystem::path const& common,
		                       std::error_code& ec) {
			return repository{common, ec};
		}

	protected:
		repository() = default;
		explicit repository(std::filesystem::path const& common, std::error_code&);

	private:
		std::filesystem::path commondir_{};
		git::config cfg_{};
	};
}  // namespace cov
