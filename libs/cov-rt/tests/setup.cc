// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "setup.hh"
#include <cov/io/file.hh>
#include <cstdio>
#include <filesystem>
#include <git2/global.hh>
#include <string>

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
			static constexpr auto playground = "runtime-tests"sv;
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

	test_initializer::test_initializer() {
		test_globals::get().enter();
	}
	test_initializer::~test_initializer() {
		test_globals::get().leave();
	}

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

	path make_path(std::string_view utf8) {
		return conv<char8_t>(utf8);
	}

#else
	std::string get_path(path const& p) {
		return p.generic_u8string();
	}

	path make_path(std::string_view utf8) {
		return std::filesystem::u8path(utf8);
	}
#endif

	std::string get_oid(git_oid const& id) {
		char buffer[42] = "";
		git_oid_fmt(buffer, &id);
		return buffer;
	}

	void test_globals::setup_test_env() {
		printf("Setting up test environment\n");
		using namespace std::filesystem;

		std::error_code ignore{};
		remove_all(test_dir(), ignore);
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
		remove_all(test_dir(), ignore);
	}
}  // namespace cov::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os) {
	*os << cov::testing::setup::get_path(path);
}
