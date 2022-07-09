// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/config.hh>
#include <git2/error.hh>
#include <numeric>

#if __has_include(<unistd.h>)
#include <unistd.h>

static int _taccess(const char* pathname, int mode) noexcept {
	return access(pathname, mode);
}
#else
// source:
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/access-waccess
enum : int {
	F_OK = 0,
	R_OK = 2,
	W_OK = 4,
	RW_OK = (R_OK | W_OK),
};

#include <io.h>

static int _taccess(const wchar_t* pathname, int mode) noexcept {
	return _waccess(pathname, mode);
}
#endif

#ifdef __cpp_lib_char8_t
#define PATH_C_STR(PATH) reinterpret_cast<char const*>((PATH).c_str())
#define STR_FROM_GENERIC(PATH) \
	std::string { PATH_C_STR(PATH), (PATH).size() }
#else
#define PATH_C_STR(PATH) ((PATH).c_str())
#define STR_FROM_GENERIC(PATH) std::move(PATH)
#endif

namespace git {
	namespace {
		using namespace std::literals;
		namespace names {
			constexpr auto config = "config"sv;
			constexpr auto dotconfig = ".config"sv;
			constexpr auto etc = "/etc/"sv;
			constexpr char XDG_CONFIG_HOME[] = "XDG_CONFIG_HOME";
			constexpr char HOME[] = "HOME";
			constexpr char USERPROFILE[] = "USERPROFILE";
			constexpr char ProgramData[] = "ProgramData";
		}  // namespace names

		char const* get_home() noexcept {
			if (auto var = std::getenv(names::HOME); var && *var) return var;
			if (auto var = std::getenv(names::USERPROFILE); var && *var)
				return var;
			return nullptr;
		}

		std::filesystem::path get_path_(auto calc) {
			auto result = calc();
			if (!result.empty() && _taccess(result.c_str(), R_OK))
				result.clear();

			return result;
		}
		std::filesystem::path calc_system_path(std::string_view app) {
			std::string result{};
			result.reserve(names::etc.length() + app.length() +
			               names::config.length());

			result.append(names::etc);
			result.append(app);
			result.append(names::config);

			return std::filesystem::path{std::move(result)};
		}

		std::filesystem::path get_system_path(std::string_view app) {
			return get_path_([app] { return calc_system_path(app); });
		}

		std::filesystem::path calc_program_data_path(
		    std::string_view dot_name) {
			auto const* program_data = std::getenv(names::ProgramData);

			if (program_data && *program_data)
				return std::filesystem::path{program_data} / dot_name;

			return {};
		}

		std::filesystem::path get_program_data_path(std::string_view dot_name) {
			return get_path_(
			    [dot_name] { return calc_program_data_path(dot_name); });
		}

		std::pair<std::filesystem::path, std::filesystem::path>
		calc_global_path(std::string_view dot_name, std::string_view app) {
			std::pair<std::filesystem::path, std::filesystem::path> result{};

			std::filesystem::path xdg_config_path{};
			auto const* xdg_config = std::getenv(names::XDG_CONFIG_HOME);
			auto const home = get_home();

			if (xdg_config && !*xdg_config) xdg_config = nullptr;
			if (xdg_config) {
				xdg_config_path = xdg_config;
			} else if (home) {
				xdg_config_path =
				    std::filesystem::path{home} / names::dotconfig;
			}

			if (!xdg_config_path.empty())
				std::get<0>(result) = xdg_config_path / app / names::config;

			if (home)
				std::get<1>(result) = std::filesystem::path{home} / dot_name;

			return result;
		}

		std::pair<std::filesystem::path, std::filesystem::path> get_global_path(
		    std::string_view dot_name,
		    std::string_view app) {
			auto result = calc_global_path(dot_name, app);
			std::get<0>(result) =
			    get_path_([&] { return std::get<0>(result); });
			std::get<1>(result) =
			    get_path_([&] { return std::get<1>(result); });
			return result;
		}  // GCOV_EXCL_LINE
	}      // namespace

	config config::create(std::error_code& ec) noexcept {
		git_config* out{};
		ec = git::make_error_code(git_config_new(&out));

		config ptr{out};
		if (ec) ptr = nullptr;

		return ptr;
	}

	config config::open_default(std::string_view dot_name,
	                            std::string_view app,
	                            std::error_code& ec) {
		ec.clear();

		auto result = create();
		if (!result) return result;
		int error = 0;

		auto const [xdg, global] = get_global_path(dot_name, app);
		auto const system = get_system_path(app);
		auto const program_data = get_program_data_path(dot_name);

		if (!system.empty())
			error = result.add_file_ondisk(  // GCOV_EXCL_LINE
			    system, GIT_CONFIG_LEVEL_SYSTEM);
		if (!error && !xdg.empty())
			error = result.add_file_ondisk(xdg, GIT_CONFIG_LEVEL_XDG);
		if (!error && !global.empty())
			error = result.add_file_ondisk(global, GIT_CONFIG_LEVEL_GLOBAL);
		if (!error && !program_data.empty())
			error = result.add_file_ondisk(program_data,
			                               GIT_CONFIG_LEVEL_PROGRAMDATA);

		if (error) result = nullptr;
		ec = git::make_error_code(error);
		return result;
	}

	int config::add_file_ondisk(const char* path,
	                            git_config_level_t level,
	                            const git_repository* repo,
	                            int force) const noexcept {
		return git_config_add_file_ondisk(get(), path, level, repo, force);
	}

	int config::add_file_ondisk(std::filesystem::path const& path,
	                            git_config_level_t level,
	                            const git_repository* repo,
	                            int force) const noexcept {
		auto const generic = path.generic_u8string();
		return add_file_ondisk(PATH_C_STR(generic), level, repo, force);
	}

	int config::add_local_config(std::filesystem::path const& directory,
	                             const git_repository* repo,
	                             int force) const {
		return add_file_ondisk(directory / names::config,
		                       GIT_CONFIG_LEVEL_LOCAL, repo, force);
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
		return set_string(name, PATH_C_STR(generic));
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
