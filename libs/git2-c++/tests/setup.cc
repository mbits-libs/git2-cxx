// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "setup.hh"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <git2-c++/global.hh>

namespace git::testing::setup {
	using namespace ::std::literals;

	namespace {
		class test_globals {
		public:
			static test_globals& get() noexcept {
				static test_globals self{};
				return self;
			}

			void enter() {
				if (!counter++) setup_test_env();
			}

			void leave() {
				if (!--counter) teardown_test_env();
			}

		private:
			static void setup_test_env();
			static void teardown_test_env();

			init thread{};
			int counter{};
		};
		std::filesystem::path get_test_dir() {
			static constexpr auto repos = "repos"sv;
			std::error_code ec{};
			auto const temp = std::filesystem::temp_directory_path(ec);
			if (ec) return repos;
			return temp / repos;
		}
	};  // namespace

	test_initializer::test_initializer() { test_globals::get().enter(); }
	test_initializer::~test_initializer() { test_globals::get().leave(); }

	std::filesystem::path test_dir() {
		std::filesystem::path dirname = get_test_dir();
		return dirname;
	}

#ifdef __cpp_lib_char8_t
	template <typename CharTo, typename Source>
	std::basic_string_view<CharTo> conv(Source const& view) {
		return {reinterpret_cast<CharTo const*>(view.data()), view.length()};
	}

	std::string get_path(path const& p) {
		auto const s8 = p.generic_u8string();
		auto const view = conv<char>(s8);
		return {view.data(), view.length()};
	}

	path make_path(std::string_view utf8) { return conv<char8_t>(utf8); }

#else
	std::string get_path(path const& p) { return p.generic_u8string(); }

	path make_path(std::string_view utf8) {
		return std::filesystem::u8path(utf8);
	}
#endif

	namespace {
		namespace file {
			static constexpr auto HEAD = "ref: refs/heads/main\n"sv;
			static constexpr auto config = R"([core]
	repositoryformatversion = 0
	filemode = false
	bare = false
	logallrefupdates = true
	ignorecase = true
)"sv;
			static constexpr auto config_bare = R"([core]
	repositoryformatversion = 0
	filemode = false
	bare = true
	ignorecase = true
)"sv;

			struct type {
				std::string_view path{};
				std::string_view content{};
			};
		}  // namespace file

		constexpr std::string_view subdirs[] = {
		    "gitdir/.git/objects/info"sv, "gitdir/.git/objects/pack"sv,
		    "gitdir/.git/refs/heads"sv,   "gitdir/.git/refs/tags"sv,
		    "bare.git/objects/info"sv,    "bare.git/objects/pack"sv,
		    "bare.git/refs/heads"sv,      "bare.git/refs/tags"sv,
		};

		constexpr file::type files[] = {
		    {"gitdir/.git/HEAD", file::HEAD},
		    {"gitdir/.git/config", file::config},
		    {"bare.git/HEAD", file::HEAD},
		    {"bare.git/config", file::config_bare},
		    {"gitdir/subdir/a-file"},
		};
	}  // namespace

	void test_globals::setup_test_env() {
		printf("Setting up test environment\n");
		using namespace std::filesystem;

		std::error_code ignore{};
		remove_all(test_dir(), ignore);

		for (auto const subdir : setup::subdirs) {
			create_directories(test_dir() / make_path(subdir), ignore);
		}

		for (auto const [filename, contents] : setup::files) {
			auto const p = test_dir() / make_path(filename);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p};
			out.write(contents.data(), contents.size());
		}
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
		remove_all(test_dir(), ignore);
	}
}  // namespace git::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os) {
	*os << git::testing::setup::get_path(path);
}
