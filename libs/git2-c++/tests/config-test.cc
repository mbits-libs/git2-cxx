// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <git2/config.hh>
#include "setup.hh"

namespace git::testing {
	using namespace ::std::literals;
	using std::filesystem::path;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;

		std::string entries;
		cfg.foreach ([&entries](git_config_entry const* entry) {
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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_unsigned(test_name, 3456);
		ASSERT_EQ(3456, *cfg.get_unsigned(test_name));
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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "-20");
		if constexpr (sizeof(int64_t) > sizeof(unsigned)) {
			constexpr auto value =
			    static_cast<int64_t>(std::numeric_limits<unsigned>::max()) + 1;
			auto const svalue = std::to_string(value);
			cfg.set_string(test_name_2, svalue);
		}

		ASSERT_EQ(0, *cfg.get_unsigned(test_name));
		if constexpr (sizeof(int64_t) > sizeof(unsigned)) {
			constexpr auto value = std::numeric_limits<unsigned>::max();
			ASSERT_EQ(value, *cfg.get_unsigned(test_name_2));
		}
	}

	TEST(config, string) {
		auto cfg = git::config::create();
		auto const repo = setup::get_path(setup::test_dir() / "config.string");
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_string(test_name, "3456"s);
		ASSERT_EQ(3456, *cfg.get_unsigned(test_name));
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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
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
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
		cfg.set_path(test_name, "dir/subdir"sv);
		ASSERT_FALSE(cfg.get_unsigned(test_name));
		auto const bool_opt = cfg.get_bool(test_name);
		if (bool_opt) ASSERT_FALSE(*bool_opt);
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

	TEST(config, delete_entry) {
		auto cfg = git::config::create();
		auto const repo =
		    setup::get_path(setup::test_dir() / "config.delete_entry");
		ASSERT_EQ(0, cfg.add_file_ondisk(repo.c_str())) << repo;
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
}  // namespace git::testing
