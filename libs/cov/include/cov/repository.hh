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
		template <typename Object>
		ref_ptr<Object> lookup(git_oid const& id, std::error_code& ec) {
			return as_a<Object>(lookup_object(id, ec));
		}
		bool write(git_oid&, ref_ptr<object> const&);
		bool write(git_oid& out, git::bytes const& bytes) {
			return git_.write(out, bytes);
		}

	protected:
		repository();
		explicit repository(std::filesystem::path const& common,
		                    std::error_code&);

		ref_ptr<object> lookup_object(git_oid const& id, std::error_code&);

	private:
		struct git_repo {
			git_repo();
			void open(std::filesystem::path const& common,
			          git::config const& cfg,
			          std::error_code&);
			ref_ptr<blob> lookup(git_oid const& id, std::error_code&);
			bool write(git_oid&, git::bytes const&);

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
