// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/object.h>
#include <git2/repository.h>
#include <cov/git2/odb.hh>
#include <cov/git2/oid.hh>
#include <cov/git2/ptr.hh>
#include <cov/git2/submodule.hh>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace git {
	GIT_PTR_FREE(git_repository);
	GIT_PTR_FREE(git_reference);

	struct reference : ptr<git_reference> {
		using ptr<git_reference>::ptr;

		reference resolve(std::error_code& ec) const noexcept {
			return git::create_handle<reference>(ec, git_reference_resolve,
			                                     this->get());
		}
	};

	template <class Holder>
	struct basic_repository : Holder {
		using Holder::Holder;

		std::string_view git_dir() const noexcept {
			return git_repository_path(this->get());
		}
		std::optional<std::string_view> work_dir() const noexcept {
			return safe(git_repository_workdir(this->get()));
		}
		std::string_view common_dir() const noexcept {
			return git_repository_commondir(this->get());
		}

		template <typename SubmoduleCallback>
		std::error_code submodule_foreach(SubmoduleCallback cb) const {
			return as_error(git_submodule_foreach(
			    this->get(), submodule_callback<SubmoduleCallback>,
			    reinterpret_cast<void*>(&cb)));
		}

		template <typename ObjectType>
		ObjectType lookup(std::string_view id,
		                  std::error_code& ec) const noexcept {
			git_oid oid;
			if (auto const err =
			        git_oid_fromstrn(&oid, id.data(), id.length())) {
				ec = as_error(err);
				return {};
			}

			return lookup<ObjectType>(oid, (id.length() + 1) / 2, ec);
		}

		template <typename ObjectType>
		ObjectType lookup(git::oid_view id,
		                  std::error_code& ec) const noexcept {
			return lookup<ObjectType>(id, GIT_OID_RAWSZ, ec);
		}
		template <typename ObjectType>
		ObjectType lookup(git::oid_view id,
		                  size_t prefix,
		                  std::error_code& ec) const noexcept {
			return git::create_object<ObjectType>(ec, git_object_lookup_prefix,
			                                      this->get(), id.ref, prefix,
			                                      ObjectType::OBJECT_TYPE);
		}

		reference head(std::error_code& ec) const noexcept {
			return git::create_handle<reference>(ec, git_repository_head,
			                                     this->get());
		}

		bool is_worktree() const noexcept {
			return !!git_repository_is_worktree(get());
		}

		auto get() const { return Holder::get(); }

	protected:
		std::optional<std::string_view> safe(char const* str) const noexcept {
			if (!str) return std::nullopt;
			return str;
		}

	private:
		template <typename SubmoduleCallback>
		static int submodule_callback(git_submodule* sm,
		                              const char* name,
		                              void* payload) {
			auto& cb = *reinterpret_cast<SubmoduleCallback*>(payload);
			cb(submodule_handle(sm), std::string_view(name));
			return 0;
		}
	};

	struct repository_handle : basic_repository<handle<git_repository>> {
		using basic_repository<handle<git_repository>>::basic_repository;
	};

	enum class Discover { WithinFs = false, AcrossFs = true };

	struct repository : basic_repository<ptr<git_repository>> {
		// GCOV_EXCL_START - ctor seems to be inlined away
		using basic_repository<ptr<git_repository>>::basic_repository;
		// GCOV_EXCL_STOP

		static repository init(std::filesystem::path const& path,
		                       bool is_bare,
		                       std::error_code& ec);
		static repository open(std::filesystem::path const& path,
		                       std::error_code& ec);
		static repository wrap(const git::odb& odb, std::error_code& ec);
		static std::filesystem::path discover(
		    std::filesystem::path const& start_pathstart_path,
		    Discover accross_fs,
		    std::error_code& ec) noexcept;
		static std::filesystem::path discover(
		    std::filesystem::path const& start_path,
		    std::error_code& ec) noexcept {
			return discover(start_path, Discover::WithinFs, ec);
		}

		operator repository_handle() const noexcept {
			return repository_handle{get()};
		}
		repository_handle handle_() const noexcept { return *this; }
	};
}  // namespace git
