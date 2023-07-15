// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cov/git2/config.hh>
#include <cov/git2/oid.hh>
#include <cov/git2/repository.hh>
#include <cov/object.hh>
#include <cov/report.hh>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace cov {
	struct repository;

	struct module_info {
		std::string name;
		std::vector<std::string> prefixes;
		auto operator<=>(module_info const&) const noexcept = default;
	};

	struct module_view {
		std::string_view name;
		std::vector<files::entry const*> items;
		auto operator<=>(module_view const&) const noexcept = default;
	};

	enum class [[nodiscard("Propagate towards UI")]] mod_errc{
	    unmodified,
	    needs_update,
	    duplicate,
	    no_module,
	};

	struct modules : public object {
		obj_type type() const noexcept override { return obj_modules; };
		bool is_modules() const noexcept final { return true; }
		virtual std::string_view separator() const noexcept = 0;
		virtual std::vector<module_info> const& entries() const noexcept = 0;
		virtual std::vector<module_view> filter(
		    std::span<std::unique_ptr<files::entry> const> const&) const = 0;
		virtual std::vector<module_view> filter(
		    std::span<files::entry const*> const&) const = 0;
		std::vector<module_view> filter(files const& report) const {
			return filter(report.entries());
		}
		virtual mod_errc set_separator(std::string const& sep) = 0;
		virtual mod_errc add(std::string const& mod,
		                     std::string const& path) = 0;
		virtual mod_errc remove(std::string const& mod,
		                        std::string const& path) = 0;
		virtual mod_errc remove_all(std::string const& mod) = 0;
		virtual std::error_code dump(git::config const& cfg) const noexcept = 0;

		static ref_ptr<modules> make_modules(
		    std::string const& separator,
		    std::vector<module_info> const& mods);
		static ref_ptr<modules> from_config(git::config const& cfg,
		                                    std::error_code& ec);
		static ref_ptr<modules> from_commit(git::oid_view commit,
		                                    git::repository_handle const& repo,
		                                    std::error_code& ec);
		static ref_ptr<modules> from_report(git::oid_view report,
		                                    repository const& repo,
		                                    std::error_code& ec);

		static void cleanup_config(std::filesystem::path const&);
	};
}  // namespace cov
