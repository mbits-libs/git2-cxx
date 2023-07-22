// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "setup.hh"
#include <cov/git2/oid.hh>
#include <cov/io/file.hh>
#include <cstdio>
#include <filesystem>
#include <tar.hh>

#include "setup.cc.inc"

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
			static constexpr auto playground =
			    "libcov-tests-c1e671e2b66e37396b593fa98488c1656813af29"sv;
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

	void test_globals::setup_test_env() {
		printf("Setting up test environment\n");
		unpack_files(test_dir(), setup::subdirs, setup::text, setup::binary);
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
#if 1
		remove_all(test_dir(), ignore);
#else
		fmt::print("TEMP: {}\n", get_path(test_dir()));
#endif
	}
}  // namespace cov::testing::setup
