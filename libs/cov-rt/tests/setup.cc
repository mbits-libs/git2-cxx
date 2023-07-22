// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "setup.hh"
#include <cov/git2/global.hh>
#include <cov/io/file.hh>
#include <cstdio>
#include <filesystem>
#include <string>
#include <tar.hh>

namespace cov::app::testing::setup {
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
			    "runtime-tests-9e85987a06593f1112893578b2c8bc701b9d0ad6"sv;
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

	git::repository open_verify_repo() {
		std::error_code ec{};
		return git::repository::open(test_dir() / "verify"sv, ec);
	}

	USE_TAR

	namespace {
#include "verify-tar.inc"
	}  // namespace

	void test_globals::setup_test_env() {
		printf("Setting up test environment\n");

		std::error_code ignore{};
		remove_all(test_dir(), ignore);
		unpack_files(test_dir(), subdirs, text, binary);
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
		remove_all(test_dir(), ignore);
	}
}  // namespace cov::app::testing::setup
