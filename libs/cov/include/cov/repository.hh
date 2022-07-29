// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <cov/db.hh>
#include <cov/discover.hh>
#include <cov/init.hh>
#include <cov/reference.hh>
#include <git2/config.hh>
#include <git2/odb.hh>
#include <git2/repository.hh>
#include <map>
#include <string>
#include <vector>

namespace git {
	struct blob;
}  // namespace git

namespace cov {
	enum class origin {
		git,
		cov,
		neither,
	};

	struct blob : object {
		obj_type type() const noexcept override { return obj_blob; }
		bool is_blob() const noexcept final { return true; }
		virtual git::blob const& peek() const noexcept = 0;
		virtual git::blob peel_and_unlink() noexcept = 0;
		virtual cov::origin origin() const noexcept = 0;

		static ref_ptr<blob> wrap(git::blob&& ref, cov::origin);
	};

	struct file_diff {
		enum kind { normal, renamed, copied, added, deleted };
		enum initial { initial_add_all, initial_with_self };
		struct {
			io::v1::coverage_stats::ratio<> current{};
			io::v1::coverage_stats::ratio<int> diff{};
		} coverage;
		struct {
			io::v1::coverage_stats current{};
			io::v1::coverage_diff diff{};
		} stats;
	};

	struct file_stats {
		std::string filename{};
		io::v1::coverage_stats current{};
		io::v1::coverage_stats previous{};
		std::string previous_name{};
		file_diff::kind diff_kind{};

		bool operator==(file_stats const&) const noexcept = default;

		file_diff diff(unsigned char digits = 2) const noexcept {
			auto const curr = current.calc(digits);
			auto const prev = previous.calc(digits);
			return {
			    .coverage = {.current = curr, .diff = io::v1::diff(curr, prev)},
			    .stats = {.current = current,
			              .diff = io::v1::diff(current, previous)},
			};
		}
	};

	struct commit_file_diff {
		std::string previous_name{};
		file_diff::kind diff_kind{};
	};

	struct repository {
		~repository();

		static std::filesystem::path discover(
		    std::filesystem::path const& current_dir,
		    discover across_fs,
		    std::error_code& ec) {
			return discover_repository(current_dir, across_fs, ec);
		}

		static std::filesystem::path discover(
		    std::filesystem::path const& current_dir,
		    std::error_code& ec) {
			return discover(current_dir, discover::within_fs, ec);
		}

		static repository init(std::filesystem::path const& sysroot,
		                       std::filesystem::path const& base,
		                       std::filesystem::path const& git_dir,
		                       std::error_code& ec,
		                       init_options const& options = {}) {
			auto common = init_repository(base, git_dir, ec, options);
			if (ec) return repository{};
			return repository{sysroot, common, ec};
		}

		static repository open(std::filesystem::path const& sysroot,
		                       std::filesystem::path const& common,
		                       std::error_code& ec) {
			return repository{sysroot, common, ec};
		}

		std::filesystem::path const& commondir() const noexcept {
			return commondir_;
		}

		std::string_view git_commondir() const noexcept {
			return git_.repo().commondir();
		}

		git::repository_handle git() const noexcept { return git_.repo(); }

		git::config const& config() const noexcept { return cfg_; }
		ref_ptr<references> const& refs() const noexcept { return refs_; }
		ref_ptr<object> dwim(std::string_view) const;
		template <typename Object>
		ref_ptr<Object> lookup(git_oid const& id, std::error_code& ec) const {
			return as_a<Object>(lookup_object(id, ec));
		}
		bool write(git_oid&, ref_ptr<object> const&);
		bool write(git_oid& out, git::bytes const& bytes) {
			return git_.write(out, bytes);
		}

		std::map<std::string, commit_file_diff> diff_betwen_commits(
		    git_oid const& newer,
		    git_oid const& older,
		    std::error_code& ec,
		    git_diff_find_options const* opts = nullptr) const;

		std::vector<file_stats> diff_betwen_reports(
		    ref_ptr<report> const& newer,
		    ref_ptr<report> const& older,
		    std::error_code& ec,
		    git_diff_find_options const* opts = nullptr) const;

		std::vector<file_stats> diff_with_parent(
		    ref_ptr<report> const& current,
		    std::error_code& ec,
		    git_diff_find_options const* opts = nullptr,
		    file_diff::initial initial_policy =
		        file_diff::initial_with_self) const;

	protected:
		repository();
		explicit repository(std::filesystem::path const& sysroot,
		                    std::filesystem::path const& common,
		                    std::error_code&);

		ref_ptr<object> lookup_object(git_oid const& id,
		                              std::error_code&) const;

	private:
		struct git_repo {
			git_repo();
			void open(std::filesystem::path const& common,
			          git::config const& cfg,
			          std::error_code&);
			ref_ptr<blob> lookup(git_oid const& id, std::error_code&) const;
			bool write(git_oid&, git::bytes const&);

			git::repository_handle repo() const noexcept { return git_; }

		private:
			git::repository git_{};
			git::repository local_{};
			git::odb odb_{};
		};

		std::filesystem::path commondir_{};
		git::config cfg_{};
		git_repo git_{};
		ref_ptr<references> refs_{};
		ref_ptr<backend> db_{};
	};
}  // namespace cov
