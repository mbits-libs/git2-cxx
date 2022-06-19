// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "git2-c++/config.hh"
#include <numeric>
#include "git2/errors.h"

namespace git {
	config config::create() noexcept {
		git_config* out{nullptr};
		auto ret = git_config_new(&out);

		config ptr{out};
		if (ret != 0) ptr = nullptr;

		return ptr;
	}

	int config::add_file_ondisk(const char* path,
	                            git_config_level_t level,
	                            const git_repository* repo,
	                            int force) const noexcept {
		return git_config_add_file_ondisk(get(), path, level, repo, force);
	}

	int config::set_unsigned(const char* name, unsigned value) const noexcept {
		return git_config_set_int64(get(), name, value);
	}

	int config::set_bool(char const* name, bool value) const noexcept {
		return git_config_set_bool(get(), name, value ? 1 : 0);
	}

	int config::set_string(const char* name, const char* value) const noexcept {
		return git_config_set_string(get(), name, value);
	}

	int config::set_path(char const* name,
	                     std::filesystem::path const& value) const noexcept {
		auto const generic = value.generic_u8string();
#ifdef __cpp_lib_char8_t
		return set_string(name, reinterpret_cast<char const*>(generic.data()));
#else
		return set_string(name, generic);
#endif
	}

	std::optional<unsigned> config::get_unsigned(
	    const char* name) const noexcept {
		int64_t i64{0};
		auto const result = git_config_get_int64(&i64, get(), name);
		if (result) return std::nullopt;

		if (i64 > 0) {
			if constexpr (sizeof(int64_t) > sizeof(unsigned)) {
				if (i64 >
				    static_cast<int64_t>(std::numeric_limits<unsigned>::max()))
					return std::numeric_limits<unsigned>::max();
				else
					return static_cast<unsigned>(i64);
			} else {
				return static_cast<unsigned>(i64);
			}
		}
		return 0;
	}

	std::optional<bool> config::get_bool(const char* name) const noexcept {
		int out{};
		auto const result = git_config_get_bool(&out, get(), name);
		if (result) return std::nullopt;
		return out != 0;
	}

	std::optional<std::string> config::get_string(
	    const char* name) const noexcept {
		char empty[] = "";
		git_buf result = GIT_BUF_INIT_CONST(empty, 0);
		auto const ret = git_config_get_string_buf(&result, get(), name);

		std::optional<std::string> out{};
		if (!ret) {
			out = std::string{};
			if (result.ptr) {
				try {
					out->assign(result.ptr, result.size);
				} catch (...) {
					out = std::nullopt;
				}
			}
		}
		git_buf_dispose(&result);

		return out;
	}

	std::optional<std::filesystem::path> config::get_path(
	    const char* name) const noexcept {
		using namespace std::filesystem;

		char empty[] = "";
		git_buf result = GIT_BUF_INIT_CONST(empty, 0);
		auto const ret = git_config_get_path(&result, get(), name);

		std::optional<path> out{};
		if (!ret) {
			out = path{};
			if (result.ptr) {
				try {
#ifdef __cpp_lib_char8_t
					auto const view = std::u8string_view{
					    reinterpret_cast<char8_t const*>(result.ptr),
					    result.size};
					*out = view;
#else
					auto const view = std::string_view{result.ptr, result.size};
					*out = u8path(view);
#endif
				} catch (...) {
					out = std::nullopt;
				}
			}
		}
		git_buf_dispose(&result);

		return out;
	}

	config_entry config::get_entry(char const* name) const noexcept {
		config_entry out{};

		git_config_entry* entry{};
		auto const result = git_config_get_entry(&entry, get(), name);
		out = config_entry{entry};
		if (result) out = config_entry{};

		return out;
	}

	int config::delete_entry(const char* name) const noexcept {
		return git_config_delete_entry(get(), name);
	}
}  // namespace git
