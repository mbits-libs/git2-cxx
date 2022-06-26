// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include "setup.hh"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <git2/global.hh>

namespace cov::testing::setup {
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

			int counter{};
		};
		std::filesystem::path get_test_dir() {
			static constexpr auto playground = "libcov-tests"sv;
			std::error_code ec{};
			auto const temp = std::filesystem::temp_directory_path(ec);
			if (ec) {
				printf("Running tests from %.*s\n",
				       static_cast<int>(playground.length()),
				       playground.data());
				return playground;
			}
			printf("Running tests from %s\n",
			       get_path(temp / playground).c_str());
			return temp / playground;
		}
	};  // namespace

	test_initializer::test_initializer() { test_globals::get().enter(); }
	test_initializer::~test_initializer() { test_globals::get().leave(); }

	std::filesystem::path test_dir() {
		static std::filesystem::path dirname = get_test_dir();
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
			static constexpr auto safe = "first line\nsecond line\n"sv;

			struct text {
				std::string_view path{};
				std::string_view content{};
			};

			struct binary {
				std::string_view path{};
				std::basic_string_view<unsigned char> content{};
			};
		}  // namespace file

		template <size_t Length>
		constexpr std::basic_string_view<unsigned char> span(
		    unsigned char const (&v)[Length]) noexcept {
			return {v, Length};
		}

#if 0
		constexpr std::string_view subdirs[] = {
		};
#endif

		constexpr file::text text[] = {
		    {"secured.txt"sv, file::safe},
		    {"unsecured.txt"sv, file::safe},
		};

#if 0
		constexpr file::binary binary[] = {
		};
#endif
	}  // namespace

	void test_globals::setup_test_env() {
		printf("Setting up test environment\n");
		using namespace std::filesystem;

		std::error_code ignore{};
		remove_all(test_dir(), ignore);

#if 0
		for (auto const subdir : setup::subdirs) {
			create_directories(test_dir() / make_path(subdir), ignore);
		}
#endif

		for (auto const [filename, contents] : setup::text) {
			auto const p = test_dir() / make_path(filename);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p, std::ios::binary};
			out.write(contents.data(), contents.size());
		}

#if 0
		for (auto const nfo : setup::binary) {
			auto const p = test_dir() / make_path(nfo.path);
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p, std::ios::binary};
			out.write(reinterpret_cast<char const*>(nfo.content.data()),
			          nfo.content.size());
		}
#endif
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
		// remove_all(test_dir(), ignore);
	}
}  // namespace cov::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os) {
	*os << cov::testing::setup::get_path(path);
}
