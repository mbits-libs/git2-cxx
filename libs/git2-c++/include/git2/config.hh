// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <git2/config.h>
#include <git2/ptr.hh>

#include <filesystem>
#include <optional>
#include <string>

namespace git {
	GIT_PTR_FREE(git_config);
	GIT_PTR_FREE(git_config_entry);

	struct config_entry : ptr<git_config_entry> {
		using ptr<git_config_entry>::ptr;
		std::string_view name() const noexcept { return get()->name; }
		std::string_view value() const noexcept { return get()->value; }
	};

	struct config : ptr<git_config> {
		using ptr<git_config>::ptr;

		static config create() noexcept {
			std::error_code ec{};
			return create(ec);
		}
		static config create(std::error_code& ec) noexcept;

		static config open_default(std::string_view dot_name,
		                           std::string_view app,
		                           std::error_code& ec);

		std::error_code add_file_ondisk(
		    char const* path,
		    git_config_level_t level = GIT_CONFIG_LEVEL_LOCAL,
		    const git_repository* repo = nullptr,
		    int force = 1) const noexcept;
		std::error_code add_file_ondisk(
		    std::filesystem::path const& path,
		    git_config_level_t level = GIT_CONFIG_LEVEL_LOCAL,
		    const git_repository* repo = nullptr,
		    int force = 1) const noexcept;
		std::error_code add_local_config(std::filesystem::path const& directory,
		                                 const git_repository* repo = nullptr,
		                                 int force = 1) const;
		std::error_code set_unsigned(char const* name,
		                             unsigned value) const noexcept;
		std::error_code set_bool(char const* name, bool value) const noexcept;
		std::error_code set_string(char const* name,
		                           char const* value) const noexcept;
		std::error_code set_string(char const* name,
		                           std::string const& value) const noexcept {
			return set_string(name, value.c_str());
		}
		std::error_code set_path(
		    char const* name,
		    std::filesystem::path const& value) const noexcept;

		std::optional<unsigned> get_unsigned(char const* name) const noexcept;
		std::optional<bool> get_bool(char const* name) const noexcept;
		std::optional<std::string> get_string(char const* name) const noexcept;
		std::optional<std::filesystem::path> get_path(
		    char const* name) const noexcept;
		config_entry get_entry(char const* name) const noexcept;

		std::error_code delete_entry(char const* name) const noexcept;

		template <typename Callback>
		std::error_code foreach (Callback cb) const {
			auto payload = reinterpret_cast<void*>(&cb);
			return as_error(git_config_foreach(get(), callback_erased<Callback>,
			                                   payload));
		}

	private:
		template <typename Callback>
		static int callback_erased(git_config_entry const* entry,
		                           void* payload) {
			auto* cb = reinterpret_cast<Callback*>(payload);
			return (*cb)(entry);
		}
	};
}  // namespace git
