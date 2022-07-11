// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/discover.hh>
#include <cov/init.hh>
#include <cov/reference.hh>
#include <git2/config.hh>

namespace cov {
	struct repository {
		~repository();

		static std::filesystem::path discover(
		    std::filesystem::path const& current_dir,
		    discover across_fs, std::error_code& ec) {
			return discover_repository(current_dir, across_fs, ec);
		}

		static std::filesystem::path discover(
		    std::filesystem::path const& current_dir,
		    std::error_code& ec) {
			return discover(current_dir, discover::within_fs, ec);
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

		std::filesystem::path const& commondir() const noexcept {
			return commondir_;
		}

		git::config const& config() const noexcept { return cfg_; }
		ref_ptr<references> const& refs() const noexcept { return refs_; }
		ref_ptr<object> dwim(std::string_view) const;

	protected:
		repository();
		explicit repository(std::filesystem::path const& common,
		                    std::error_code&);

	private:
		std::filesystem::path commondir_{};
		git::config cfg_{};
		ref_ptr<references> refs_{};
	};
}  // namespace cov
