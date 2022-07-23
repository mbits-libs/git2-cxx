// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <fstream>
#include <git2/config.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	using std::filesystem::path;
	using ::testing::Test;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;
	using ::testing::WithParamInterface;

	std::string_view view_of(char const* ptr) {
		return ptr ? ptr : std::string_view{};
	}

	class base_config_test : public Test {
	protected:
		void setup_env(
		    std::vector<std::pair<std::string_view, std::string_view>> const&
		        files,
		    std::vector<std::pair<std::string_view, std::string_view>> const&
		        env) {
			for (auto const& [filename, contents] : files) {
				auto const file_path = setup::test_dir() / filename;
				create_directories(file_path.parent_path());
				std::ofstream out{file_path};
				out.write(contents.data(),
				          static_cast<std::streamsize>(contents.size()));
			}

			for (auto const& [name, value] : env) {
#ifdef _WIN32
				std::wstring var = std::filesystem::path{name}.native();
				std::filesystem::path path{};
				var.push_back(L'=');
				if (!value.empty()) {
					path = setup::test_dir() / value;
					path.make_preferred();
					var.append(path.native());
				}
				auto const result = _wputenv(var.c_str());
				ASSERT_EQ(0, result) << "Variable: " << name;
				if (value.empty()) {
					ASSERT_EQ(nullptr, std::getenv(name.data()));
				} else {
					ASSERT_EQ(path.string(), view_of(std::getenv(name.data())));
				}
#else
				if (value.empty()) {
					auto const result = unsetenv(name.data());
					ASSERT_EQ(0, result) << "Variable: " << name;
					ASSERT_EQ(nullptr, std::getenv(name.data()));
					continue;
				}
				auto const path = setup::test_dir() / value;
				auto const result = setenv(name.data(), path.c_str(), 1);
				ASSERT_EQ(0, result) << "Variable: " << name;
				ASSERT_EQ(path.string(), std::getenv(name.data()));
#endif
			}
		}
	};

	namespace {
		constexpr char test_name[] = "group.value";
		constexpr char test_name_2[] = "elsewhere.value";
	}  // namespace

	std::string get_repo_config(std::string_view path) {
		return setup::get_path(setup::test_dir() / setup::make_path(path) /
		                       "config"sv);
	}

	struct config_info {
		std::string_view repo;
		std::string_view values{};

		friend std::ostream& operator<<(std::ostream& out,
		                                config_info const& param) {
			return out << "[$REPOS/" << param.repo << "config]";
		}
	};

	class config : public TestWithParam<config_info> {};

	TEST_P(config, local) {
		auto const& param = GetParam();
		auto const repo = get_repo_config(param.repo);
		auto cfg = git::config::create();
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;

		std::string entries;
		cfg.foreach_entry([&entries](git_config_entry const* entry) {
			if (!entries.empty()) entries.push_back('\n');
			entries.append(entry->name);
			entries.push_back('=');
			entries.append(entry->value);
			return 0;
		});
		ASSERT_EQ(param.values, entries);
	}

	TEST(config, unsigned) {
		auto cfg = git::config::create();
		auto const repo =
		    setup::get_path(setup::test_dir() / "config.unsigned");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_unsigned(test_name, 3456);
		ASSERT_EQ(3456u, *cfg.get_unsigned(test_name));
		ASSERT_TRUE(*cfg.get_bool(test_name));
		ASSERT_EQ("3456"sv, *cfg.get_string(test_name));
		ASSERT_EQ("3456"sv, setup::get_path(*cfg.get_path(test_name)));
		auto const entry = cfg.get_entry(test_name);
		ASSERT_TRUE(entry);
		ASSERT_EQ(std::string_view{test_name}, entry.name());
		ASSERT_EQ("3456"sv, entry.value());

		ASSERT_FALSE(cfg.get_unsigned(test_name_2));
		ASSERT_FALSE(cfg.get_bool(test_name_2));
		ASSERT_FALSE(cfg.get_string(test_name_2));
		ASSERT_FALSE(cfg.get_path(test_name_2));
		auto const entry2 = cfg.get_entry(test_name_2);
		ASSERT_FALSE(entry2);
	}

	TEST(config, unsigned_limits) {
		auto cfg = git::config::create();
		auto const repo =
		    setup::get_path(setup::test_dir() / "config.unsigned_limits");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "-20");
		if constexpr (sizeof(int64_t) > sizeof(unsigned)) {
			constexpr auto value =
			    static_cast<int64_t>(std::numeric_limits<unsigned>::max()) + 1;
			auto const svalue = std::to_string(value);
			cfg.set_string(test_name_2, svalue);
		}

		ASSERT_EQ(0u, *cfg.get_unsigned(test_name));
		if constexpr (sizeof(int64_t) > sizeof(unsigned)) {
			constexpr auto value = std::numeric_limits<unsigned>::max();
			ASSERT_EQ(value, *cfg.get_unsigned(test_name_2));
		}
	}

	TEST(config, string) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.string");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "3456"s);
		ASSERT_EQ(3456u, *cfg.get_unsigned(test_name));
		ASSERT_TRUE(*cfg.get_bool(test_name));
		ASSERT_EQ("3456"sv, *cfg.get_string(test_name));
		ASSERT_EQ("3456"sv, setup::get_path(*cfg.get_path(test_name)));
		auto const entry = cfg.get_entry(test_name);
		ASSERT_TRUE(entry);
		ASSERT_EQ(std::string_view{test_name}, entry.name());
		ASSERT_EQ("3456"sv, entry.value());

		ASSERT_FALSE(cfg.get_unsigned(test_name_2));
		ASSERT_FALSE(cfg.get_bool(test_name_2));
		ASSERT_FALSE(cfg.get_string(test_name_2));
		ASSERT_FALSE(cfg.get_path(test_name_2));
		auto const entry2 = cfg.get_entry(test_name_2);
		ASSERT_FALSE(entry2);
	}

	TEST(config, string_true) {
		auto cfg = git::config::create();
		auto const repo =
		    setup::get_path(setup::test_dir() / "config.string_true");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "true"s);
		ASSERT_FALSE(cfg.get_unsigned(test_name));
		ASSERT_TRUE(*cfg.get_bool(test_name));
		ASSERT_EQ("true"sv, *cfg.get_string(test_name));
		ASSERT_EQ("true"sv, setup::get_path(*cfg.get_path(test_name)));
		auto const entry = cfg.get_entry(test_name);
		ASSERT_TRUE(entry);
		ASSERT_EQ(std::string_view{test_name}, entry.name());
		ASSERT_EQ("true"sv, entry.value());

		ASSERT_FALSE(cfg.get_unsigned(test_name_2));
		ASSERT_FALSE(cfg.get_bool(test_name_2));
		ASSERT_FALSE(cfg.get_string(test_name_2));
		ASSERT_FALSE(cfg.get_path(test_name_2));
		auto const entry2 = cfg.get_entry(test_name_2);
		ASSERT_FALSE(entry2);
	}

	TEST(config, string_false) {
		auto cfg = git::config::create();
		auto const repo =
		    setup::get_path(setup::test_dir() / "config.string_false");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "false"s);
		ASSERT_FALSE(cfg.get_unsigned(test_name));
		ASSERT_FALSE(*cfg.get_bool(test_name));
		ASSERT_EQ("false"sv, *cfg.get_string(test_name));
		ASSERT_EQ("false"sv, setup::get_path(*cfg.get_path(test_name)));
		auto const entry = cfg.get_entry(test_name);
		ASSERT_TRUE(entry);
		ASSERT_EQ(std::string_view{test_name}, entry.name());
		ASSERT_EQ("false"sv, entry.value());

		ASSERT_FALSE(cfg.get_unsigned(test_name_2));
		ASSERT_FALSE(cfg.get_bool(test_name_2));
		ASSERT_FALSE(cfg.get_string(test_name_2));
		ASSERT_FALSE(cfg.get_path(test_name_2));
		auto const entry2 = cfg.get_entry(test_name_2);
		ASSERT_FALSE(entry2);
	}

	TEST(config, boolean) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.boolean");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_bool(test_name, true);
		ASSERT_FALSE(cfg.get_unsigned(test_name));
		ASSERT_TRUE(*cfg.get_bool(test_name));
		ASSERT_EQ("true"sv, *cfg.get_string(test_name));
		ASSERT_EQ("true"sv, setup::get_path(*cfg.get_path(test_name)));
		auto const entry = cfg.get_entry(test_name);
		ASSERT_TRUE(entry);
		ASSERT_EQ(std::string_view{test_name}, entry.name());
		ASSERT_EQ("true"sv, entry.value());

		ASSERT_FALSE(cfg.get_unsigned(test_name_2));
		ASSERT_FALSE(cfg.get_bool(test_name_2));
		ASSERT_FALSE(cfg.get_string(test_name_2));
		ASSERT_FALSE(cfg.get_path(test_name_2));
		auto const entry2 = cfg.get_entry(test_name_2);
		ASSERT_FALSE(entry2);
	}

	TEST(config, path) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.path");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_path(test_name, "dir/subdir"sv);
		ASSERT_FALSE(cfg.get_unsigned(test_name));
		auto const bool_opt = cfg.get_bool(test_name);
		if (bool_opt) {
			ASSERT_FALSE(*bool_opt);
		}
		ASSERT_EQ("dir/subdir"sv, *cfg.get_string(test_name));
		ASSERT_EQ("dir/subdir"sv, setup::get_path(*cfg.get_path(test_name)));
		auto const entry = cfg.get_entry(test_name);
		ASSERT_TRUE(entry);
		ASSERT_EQ(std::string_view{test_name}, entry.name());
		ASSERT_EQ("dir/subdir"sv, entry.value());

		ASSERT_FALSE(cfg.get_unsigned(test_name_2));
		ASSERT_FALSE(cfg.get_bool(test_name_2));
		ASSERT_FALSE(cfg.get_string(test_name_2));
		ASSERT_FALSE(cfg.get_path(test_name_2));
		auto const entry2 = cfg.get_entry(test_name_2);
		ASSERT_FALSE(entry2);
	}

	TEST(config, add_strings) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.path");
		std::filesystem::remove(repo);
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 1"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 2"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 3"));

		ASSERT_EQ("value 3", *cfg.get_string(test_name));
		std::vector<std::string> actual{};
		cfg.get_multivar_foreach(test_name, "^val",
		                         [&](git_config_entry const* entry) {
			                         actual.push_back(entry->value);
			                         return 0;
		                         });
		std::vector<std::string> expected{"value 1", "value 2", "value 3"};
		ASSERT_EQ(expected, actual);
	}

	TEST(config, replace_strings) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.path");
		std::filesystem::remove(repo);
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 1"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 2 here"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 3 here"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 4 here"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 3"));

		ASSERT_FALSE(
		    cfg.set_multivar(test_name, "^value [0-9] here$", "value 2"));

		ASSERT_EQ("value 3", *cfg.get_string(test_name));
		std::vector<std::string> actual{};
		cfg.get_multivar_foreach(test_name, "^val",
		                         [&](git_config_entry const* entry) {
			                         actual.push_back(entry->value);
			                         return 0;
		                         });
		std::vector<std::string> expected{"value 1", "value 2", "value 2",
		                                  "value 2", "value 3"};
		ASSERT_EQ(expected, actual);
	}

	TEST(config, remove_strings) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.path");
		std::filesystem::remove(repo);
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 1"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 2 here"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 3 here"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 4 here"));
		ASSERT_FALSE(cfg.set_multivar(test_name, "^$", "value 3"));

		ASSERT_FALSE(cfg.delete_multivar(test_name, "^value [0-9] here$"));

		ASSERT_EQ("value 3", *cfg.get_string(test_name));
		std::vector<std::string> actual{};
		cfg.get_multivar_foreach(test_name, "^val",
		                         [&](git_config_entry const* entry) {
			                         actual.push_back(entry->value);
			                         return 0;
		                         });
		std::vector<std::string> expected{"value 1", "value 3"};
		ASSERT_EQ(expected, actual);
	}

	TEST(config, delete_entry) {
		auto cfg = git::config::create();
		auto const repo =
		    setup::get_path(setup::test_dir() / "config.delete_entry");
		ASSERT_FALSE(cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "a value"s);
		{
			auto const entry = cfg.get_entry(test_name);
			ASSERT_TRUE(entry);
			ASSERT_EQ(std::string_view{test_name}, entry.name());
			ASSERT_EQ("a value"sv, entry.value());
		}
		cfg.delete_entry(test_name);
		{
			auto const entry = cfg.get_entry(test_name);
			ASSERT_FALSE(entry);
		}
	}

	TEST(config, enum_mem) {
		auto cfg = git::config::create();
		auto const ec = cfg.add_memory(R"([group]
  key = value 1
  key = value 2

[titled "group"]
  param = )"sv);
		ASSERT_FALSE(ec);
		std::vector<std::string> actual{};
		cfg.foreach_entry([&](git_config_entry const* entry) {
			actual.push_back(entry->name + "="s + entry->value);
			return 0;
		});
		std::vector<std::string> expected{
		    "group.key=value 1", "group.key=value 2", "titled.group.param="};
		ASSERT_EQ(expected, actual);
	}

	constexpr config_info repos[] = {
	    {
	        "bare.git/"sv,
	        R"(core.repositoryformatversion=0
core.filemode=false
core.bare=true
core.ignorecase=true)"sv,
	    },
	    {
	        "gitdir/.git/"sv,
	        R"(core.repositoryformatversion=0
core.filemode=false
core.bare=false
core.logallrefupdates=true
core.ignorecase=true
submodule.bare.url=/tmp/repos/bare.git
submodule.bare.active=true)"sv,
	    },
	};

	INSTANTIATE_TEST_SUITE_P(repos, config, ValuesIn(repos));

	struct config_global_test {
		std::string_view name;
		std::string_view sysroot;
		std::vector<std::pair<std::string_view, std::string_view>> files{};
		std::vector<std::pair<std::string_view, std::string_view>> env{};
		struct {
			std::string_view open_default{};
			std::string_view open_global{};
			std::string_view open_system{};
			bool has_global{true};
			bool has_system{true};
		} expected{};

		friend std::ostream& operator<<(std::ostream& out,
		                                config_global_test const& param) {
			return out << param.name;
		}
	};

	class config_global : public base_config_test,
	                      public WithParamInterface<config_global_test> {
	protected:
		void setup_env(
		    std::vector<std::pair<std::string_view, std::string_view>> const&
		        files,
		    std::vector<std::pair<std::string_view, std::string_view>> const&
		        env) {
			remove_all(setup::test_dir() / "test_value"sv);
			base_config_test::setup_env(files, env);
		}
	};

#define ASSERT_SETUP(FILES, ENV)                        \
	do {                                                \
		setup_env(FILES, ENV);                          \
		if (::testing::Test::HasFatalFailure()) return; \
	} while (0)

	TEST_P(config_global, test_default) {
		auto const& [_, sysroot, files, env, expected] = GetParam();

		ASSERT_SETUP(files, env);

		std::error_code ec{};
		auto const cfg = git::config::open_default(setup::test_dir() / sysroot,
		                                           ".dotfile"sv, "dot"sv, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(cfg);

		auto const value = cfg.get_string("test.value");
		ASSERT_TRUE(value);
		ASSERT_EQ(expected.open_default, *value);
	}

	TEST_P(config_global, test_system) {
		auto const& [_, sysroot, files, env, expected] = GetParam();

		ASSERT_SETUP(files, env);

		std::error_code ec{};
		auto const cfg =
		    git::config::open_system(setup::test_dir() / sysroot, "dot"sv, ec);
		if (expected.has_system) {
			ASSERT_FALSE(ec);
		}

		if (!ec) {
			ASSERT_TRUE(cfg);

			auto const value = cfg.get_string("test.value");
			if (expected.has_system) {
				ASSERT_TRUE(value);
				ASSERT_EQ(expected.open_system, *value);
			} else {
				ASSERT_FALSE(value);
			}
		}
	}

	TEST_P(config_global, test_global) {
		auto const& [_, sysroot, files, env, expected] = GetParam();

		ASSERT_SETUP(files, env);

		std::error_code ec{};
		auto const cfg =
		    git::config::open_global(".dotfile"sv, "dot"sv, false, ec);
		if (expected.has_global) {
			ASSERT_FALSE(ec);
		}

		if (!ec) {
			ASSERT_TRUE(cfg);

			auto const value = cfg.get_string("test.value");
			if (expected.has_global) {
				ASSERT_TRUE(value);
				ASSERT_EQ(expected.open_global, *value);
			} else {
				ASSERT_FALSE(value);
			}
		}
	}

	static constexpr auto XDG_CONFIG_HOME = "XDG_CONFIG_HOME"sv;
	static constexpr auto HOME = "HOME"sv;
	static constexpr auto USERPROFILE = "USERPROFILE"sv;

	static constexpr auto from_xdg_config = "[test]\nvalue=from xdg config"sv;
	static constexpr auto from_home_xdg_config =
	    "[test]\nvalue=from home/.config config"sv;
	static constexpr auto from_user_profile =
	    "[test]\nvalue=from user profile"sv;
	static constexpr auto from_system_data = "[test]\nvalue=from system data"sv;

	static config_global_test const env[] = {
	    {
	        .name = "Windows"sv,
	        .sysroot = "test-value/data"sv,
	        .files = {{"test-value/.dotfile"sv, from_user_profile},
	                  {"test-value/data/.dotfile"sv, from_system_data}},
	        .env = {{XDG_CONFIG_HOME, ""sv},
	                {HOME, ""sv},
	                {USERPROFILE, "test-value"sv}},
	        .expected =
	            {
	                .open_default = "from user profile"sv,
	                .open_global = "from user profile"sv,
	                .open_system = "from system data"sv,
	            },
	    },
	    {
	        .name = "XDG over $HOME"sv,
	        .sysroot = "test-value/non-existent"sv,
	        .files = {{"test-value/actual/.config/dot/config"sv,
	                   from_xdg_config},
	                  {"test-value/home/.config/dot/config"sv,
	                   from_home_xdg_config}},
	        .env = {{XDG_CONFIG_HOME, "test-value/actual/.config"sv},
	                {HOME, "test-value/home"sv},
	                {USERPROFILE, ""sv}},
	        .expected = {.open_default = "from xdg config"sv,
	                     .open_global = "from xdg config"sv,
	                     .has_system = false},
	    },
	    {
	        .name = "$HOME if no XDG"sv,
	        .sysroot = "test-value/non-existent"sv,
	        .files = {{"test-value/actual/.config/dot/config"sv,
	                   from_xdg_config},
	                  {"test-value/home/.config/dot/config"sv,
	                   from_home_xdg_config}},
	        .env = {{XDG_CONFIG_HOME, ""sv},
	                {HOME, "test-value/home"sv},
	                {USERPROFILE, ""sv}},
	        .expected = {.open_default = "from home/.config config"sv,
	                     .open_global = "from home/.config config"sv,
	                     .has_system = false},
	    },
	    {
	        .name = "fall back to system"sv,
	        .sysroot = "test-value/data"sv,
	        .files = {{"test-value/data/etc/dotconfig"sv, from_system_data}},
	        .env = {{XDG_CONFIG_HOME, ""sv}, {HOME, ""sv}, {USERPROFILE, ""sv}},
	        .expected = {.open_default = "from system data"sv,
	                     .open_system = "from system data"sv,
	                     .has_global = false},
	    },
#if 0
	    {
	        .name = ""sv,
	        .sysroot = {},
	        .files = {},
	        .env = {{XDG_CONFIG_HOME, ""sv},
	                {HOME, ""sv},
	                {USERPROFILE, ""sv}},
	        .expected = { .open_default = ""sv },
	    },
#endif
	};

	INSTANTIATE_TEST_SUITE_P(env, config_global, ValuesIn(env));
}  // namespace git::testing
