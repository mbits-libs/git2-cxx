#pragma once
#include "git2++/ptr.hh"
#include "git2/config.h"

#include <string>

namespace git {
	GIT_PTR_FREE(git_config);
	GIT_PTR_FREE(git_config_entry);

	struct config_entry : ptr<git_config_entry> {
		using ptr<git_config_entry>::ptr;
		char const* name() const noexcept { return get()->name; }
		char const* value() const noexcept { return get()->value; }
	};

	struct config : ptr<git_config> {
		using ptr<git_config>::ptr;

		static config create() noexcept;

		int add_file_ondisk(const char* path,
		                    git_config_level_t level = GIT_CONFIG_LEVEL_LOCAL,
		                    const git_repository* repo = nullptr,
		                    int force = 1) const noexcept;
		int set_unsigned(const char* name, unsigned value) const noexcept;
		int set_string(const char* name, const char* value) const noexcept;

		int get_unsigned(unsigned& out, const char* name) const noexcept;
		int get_unsigned_or(unsigned& out,
		                    const char* name,
		                    unsigned if_missing) const noexcept;
		int get_string(std::string& out, const char* name) const noexcept;
		int get_path(std::string& out, const char* name) const noexcept;
		int get_entry(config_entry& out, char const* name) const
		    noexcept;

		int delete_entry(const char* name) const noexcept;

		template <typename Callback>
		int foreach (Callback cb) const {
			auto payload = reinterpret_cast<void*>(&cb);
			return git_config_foreach(get(), callback_erased<Callback>,
			                          payload);
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