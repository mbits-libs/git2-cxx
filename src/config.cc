// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "git2-c++/config.hh"
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

	int config::set_string(const char* name, const char* value) const noexcept {
		return git_config_set_string(get(), name, value);
	}

	int config::get_unsigned(unsigned& out, const char* name) const noexcept {
		out = 0;

		int64_t i64{0};
		auto const result = git_config_get_int64(&i64, get(), name);
		if (result) return result;

		if (i64 > 0) {
			if constexpr (sizeof(int64_t) > sizeof(unsigned)) {
				if (i64 >
				    static_cast<int64_t>(std::numeric_limits<unsigned>::max()))
					out = std::numeric_limits<unsigned>::max();
				else
					out = static_cast<unsigned>(i64);
			} else {
				out = static_cast<unsigned>(i64);
			}
		}
		return GIT_OK;
	}

	int config::get_unsigned_or(unsigned& out,
	                            const char* name,
	                            unsigned if_missing) const noexcept {
		auto result = get_unsigned(out, name);
		if (result == GIT_ENOTFOUND) {
			result = GIT_OK;
			out = if_missing;
		}
		return result;
	}

	int config::get_string(std::string& out, const char* name) const noexcept {
		git_buf result = GIT_BUF_INIT_CONST(nullptr, 0);
		auto const ret = git_config_get_string_buf(&result, get(), name);
		out = result.ptr ? result.ptr : std::string{};
		git_buf_dispose(&result);

		return ret;
	}

	int config::get_path(std::string& out, const char* name) const noexcept {
		git_buf buf{};
		auto const ret = git_config_get_path(&buf, get(), name);
		if (ret) {
			out.clear();
			git_buf_dispose(&buf);
			return ret;
		}

		try {
			out = buf.ptr;
			git_buf_dispose(&buf);
		} catch (std::bad_cast&) {
			git_buf_dispose(&buf);
			out.clear();
			return GIT_ERROR;
		}
		return 0;
	}

	int config::get_entry(config_entry& out, char const* name) const noexcept {
		out = {};

		git_config_entry* entry{};
		auto const result = git_config_get_entry(&entry, get(), name);
		if (result) {
			if (entry) git_config_entry_free(entry);
			return result;
		}

		out = config_entry{entry};
		return GIT_OK;
	}

	int config::delete_entry(const char* name) const noexcept {
		return git_config_delete_entry(get(), name);
	}
}  // namespace git
