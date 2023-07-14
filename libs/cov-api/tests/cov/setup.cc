// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "setup.hh"
#include <cov/git2/oid.hh>
#include <cov/io/file.hh>
#include <cstdio>
#include <filesystem>
#include <tar.hh>

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

	std::string get_oid(git::oid_view id) { return id.str(); }

	USE_TAR

	namespace {
		namespace file {
			static constexpr auto safe = "first line\nsecond line\n"sv;

			static constexpr auto text_1 =
			    "project(test CXX)\n"
			    "add_executable(test\n"
			    "  src/main.cc\n"
			    "  src/greetings.cc\n"
			    ")\n"sv;
			static constexpr auto text_2 = "build/\n"sv;
			static constexpr auto text_3 =
			    "#pragma once\n"
			    "\n"
			    "void greetings();\n"sv;
			static constexpr auto text_4 =
			    "#include \"greetings.hh\"\n"
			    "#include <iostream>\n"
			    "\n"
			    "void greetings() {\n"
			    "  std::cout << \"Hello, world!\\n"
			    "\";\n"
			    "  std::cout << \"TBD\\n"
			    "\";\n"
			    "  std::cout << \"Third thing here\\n"
			    "\";\n"
			    "}"sv;
			static constexpr auto text_5 =
			    "#include \"greetings.hh\"\n"
			    "\n"
			    "int main() {\n"
			    "  greetings();\n"
			    "}"sv;
			static constexpr auto text_6 = "ref: refs/heads/master\n"sv;
			static constexpr auto text_7 =
			    "[core]\n"
			    "	repositoryformatversion = 0\n"
			    "	filemode = true\n"
			    "	bare = false\n"
			    "	logallrefupdates = true\n"sv;
			static constexpr auto text_8 = "ref: refs/heads/main\n"sv;
			static constexpr auto text_9 =
			    "[core]\n"
			    "	gitdir = ..\n"sv;
			static constexpr auto text_10 =
			    "928c96e91cc3b4f2b14038993c9f205a266b1da3\n"sv;
			static constexpr auto text_11 =
			    "9eebbdd12ec79e1cf43731d778d055b248375fda\n"sv;

			static constexpr unsigned char binary_1[] = {
			    0x44, 0x49, 0x52, 0x43, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
			    0x00, 0x05, 0x62, 0xcf, 0x05, 0x2a, 0x29, 0x76, 0x55, 0x00,
			    0x62, 0xcf, 0x05, 0x2a, 0x29, 0x76, 0x55, 0x00, 0x00, 0x01,
			    0x03, 0x02, 0x01, 0x30, 0x7c, 0x33, 0x00, 0x00, 0x81, 0xa4,
			    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
			    0x00, 0x07, 0x56, 0x76, 0x09, 0xb1, 0x23, 0x4a, 0x9b, 0x88,
			    0x06, 0xc5, 0xa0, 0x5d, 0xa6, 0xc8, 0x66, 0xe4, 0x80, 0xaa,
			    0x14, 0x8d, 0x00, 0x0a, 0x2e, 0x67, 0x69, 0x74, 0x69, 0x67,
			    0x6e, 0x6f, 0x72, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x62, 0xcf, 0x06, 0xe5, 0x09, 0x77, 0xf7, 0x8a,
			    0x62, 0xcf, 0x06, 0xe5, 0x09, 0x77, 0xf7, 0x8a, 0x00, 0x01,
			    0x03, 0x02, 0x01, 0x30, 0x7c, 0x17, 0x00, 0x00, 0x81, 0xa4,
			    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
			    0x00, 0x49, 0x76, 0xc1, 0xd3, 0x5a, 0x79, 0x3e, 0x45, 0xe8,
			    0xf8, 0x0e, 0x3c, 0x5c, 0x4e, 0xeb, 0x3f, 0xad, 0x20, 0x7b,
			    0x41, 0xef, 0x00, 0x0e, 0x43, 0x4d, 0x61, 0x6b, 0x65, 0x4c,
			    0x69, 0x73, 0x74, 0x73, 0x2e, 0x74, 0x78, 0x74, 0x00, 0x00,
			    0x00, 0x00, 0x62, 0xcf, 0x07, 0xa5, 0x07, 0x08, 0x78, 0x3c,
			    0x62, 0xcf, 0x07, 0xa5, 0x07, 0x08, 0x78, 0x3c, 0x00, 0x01,
			    0x03, 0x02, 0x01, 0x30, 0x7e, 0x11, 0x00, 0x00, 0x81, 0xa4,
			    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
			    0x00, 0xa0, 0xfa, 0x99, 0x2b, 0x78, 0x1b, 0xa8, 0x2d, 0x8c,
			    0x79, 0x3c, 0xb3, 0xe3, 0xbc, 0x4b, 0xea, 0xa6, 0xe7, 0x31,
			    0xcd, 0x26, 0x00, 0x10, 0x73, 0x72, 0x63, 0x2f, 0x67, 0x72,
			    0x65, 0x65, 0x74, 0x69, 0x6e, 0x67, 0x73, 0x2e, 0x63, 0x63,
			    0x00, 0x00, 0x62, 0xcf, 0x06, 0x03, 0x04, 0x3d, 0xd0, 0xc1,
			    0x62, 0xcf, 0x06, 0x03, 0x04, 0x3d, 0xd0, 0xc1, 0x00, 0x01,
			    0x03, 0x02, 0x01, 0x30, 0x7e, 0x27, 0x00, 0x00, 0x81, 0xa4,
			    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
			    0x00, 0x20, 0xa8, 0xac, 0xd7, 0x7b, 0xb1, 0x62, 0x27, 0xb2,
			    0xa4, 0xa5, 0x51, 0x78, 0x87, 0x40, 0x2c, 0x7e, 0x08, 0x2f,
			    0x58, 0x83, 0x00, 0x10, 0x73, 0x72, 0x63, 0x2f, 0x67, 0x72,
			    0x65, 0x65, 0x74, 0x69, 0x6e, 0x67, 0x73, 0x2e, 0x68, 0x68,
			    0x00, 0x00, 0x62, 0xcf, 0x06, 0x2f, 0x0f, 0xba, 0x87, 0xd9,
			    0x62, 0xcf, 0x06, 0x2f, 0x0f, 0xba, 0x87, 0xd9, 0x00, 0x01,
			    0x03, 0x02, 0x01, 0x30, 0x7c, 0x19, 0x00, 0x00, 0x81, 0xa4,
			    0x00, 0x00, 0x03, 0xe8, 0x00, 0x00, 0x03, 0xe8, 0x00, 0x00,
			    0x00, 0x36, 0x52, 0x33, 0x43, 0x1b, 0x0c, 0x55, 0x24, 0x22,
			    0x82, 0x03, 0x44, 0x52, 0x6e, 0x0d, 0x56, 0x00, 0xbc, 0x8c,
			    0x6d, 0x6e, 0x00, 0x0b, 0x73, 0x72, 0x63, 0x2f, 0x6d, 0x61,
			    0x69, 0x6e, 0x2e, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
			    0x00, 0x00, 0x54, 0x52, 0x45, 0x45, 0x00, 0x00, 0x00, 0x35,
			    0x00, 0x35, 0x20, 0x31, 0x0a, 0x72, 0xc1, 0x61, 0x15, 0xa4,
			    0x05, 0x9d, 0x38, 0xd5, 0x8b, 0x2a, 0x4d, 0xbb, 0x12, 0xa2,
			    0x2c, 0x2a, 0x7a, 0xc8, 0xc3, 0x73, 0x72, 0x63, 0x00, 0x33,
			    0x20, 0x30, 0x0a, 0x72, 0xc5, 0x8f, 0x7a, 0x63, 0x5e, 0xdf,
			    0x60, 0xa6, 0xc3, 0x52, 0xca, 0x71, 0x90, 0xca, 0xe8, 0xac,
			    0x3f, 0x65, 0xfe, 0xd7, 0x47, 0x06, 0xd3, 0x78, 0xe3, 0x99,
			    0x9b, 0x68, 0x36, 0x45, 0xa4, 0xf7, 0x27, 0xe3, 0xf1, 0x0c,
			    0x2d, 0x6b, 0xa4};

			static constexpr unsigned char binary_2[] = {
			    0x78, 0x01, 0x4b, 0xca, 0x49, 0x51, 0x60, 0x60, 0x60, 0x64,
			    0x10, 0x64, 0x00, 0x91, 0x0c, 0x0c, 0x37, 0x52, 0xe7, 0x5d,
			    0x4e, 0x3c, 0xba, 0x5e, 0x61, 0xf2, 0xd6, 0x3f, 0xf2, 0x6b,
			    0x5f, 0x3d, 0x92, 0xb0, 0x4c, 0x33, 0xe4, 0x06, 0x0a, 0x33,
			    0xa8, 0x2a, 0x6c, 0x4c, 0x01, 0xd1, 0xac, 0x40, 0x0c, 0x52,
			    0x87, 0x0b, 0x00, 0x00, 0xe9, 0x80, 0x0d, 0x38};

			static constexpr unsigned char binary_3[] = {
			    0x78, 0x01, 0x2b, 0x2a, 0x28, 0x2a, 0x61, 0x60, 0x60, 0x64,
			    0x50, 0x65, 0x60, 0x60, 0xe0, 0x03, 0xe2, 0x09, 0x75, 0x1a,
			    0xa7, 0x57, 0xbc, 0x2c, 0x7a, 0x50, 0xa1, 0xc4, 0xd0, 0x1d,
			    0x7b, 0xa1, 0x63, 0x8b, 0xf3, 0x96, 0x47, 0xd2, 0xcd, 0x0b,
			    0xdb, 0xfe, 0xf1, 0x28, 0xaf, 0x0f, 0x3a, 0xff, 0xce, 0x2d,
			    0xda, 0x28, 0x42, 0xfc, 0xba, 0xfe, 0x9f, 0x1d, 0x1b, 0x8d,
			    0x81, 0x6a, 0x79, 0x81, 0x98, 0x11, 0x88, 0x41, 0x40, 0x55,
			    0x61, 0x63, 0x0a, 0x4c, 0x4c, 0x0e, 0xc8, 0x07, 0xc9, 0x81,
			    0x68, 0x90, 0xfc, 0x4d, 0x55, 0xe1, 0xac, 0x63, 0xbb, 0xc3,
			    0x3c, 0x24, 0xb6, 0x3d, 0xdb, 0xd9, 0x36, 0x6f, 0x7f, 0xbb,
			    0xdc, 0x82, 0xba, 0x74, 0xa0, 0x30, 0x43, 0x13, 0xdb, 0xf9,
			    0x24, 0x6e, 0x20, 0xcd, 0x04, 0xe2, 0x60, 0x03, 0xae, 0x15,
			    0x25, 0xa9, 0x79, 0x29, 0x0a, 0x89, 0x05, 0x05, 0x5c, 0x0c,
			    0x5e, 0xf9, 0x19, 0x79, 0x79, 0x95, 0x0a, 0x8e, 0x05, 0x05,
			    0x39, 0xa9, 0xc5, 0xa9, 0xa9, 0x29, 0x0c, 0x59, 0x60, 0x01,
			    0x07, 0xa0, 0x24, 0x44, 0x40, 0x2f, 0x39, 0x3f, 0x97, 0x21,
			    0x37, 0x31, 0x33, 0x8f, 0xc1, 0x41, 0xee, 0xd3, 0xaa, 0xcc,
			    0x1e, 0x05, 0x4b, 0xeb, 0x6d, 0xd3, 0xe6, 0x96, 0xdb, 0x3c,
			    0x3a, 0xde, 0x38, 0xa9, 0x29, 0x15, 0x64, 0x3c, 0x3e, 0xbb,
			    0x00, 0xd4, 0xba, 0x42, 0x0a};

			static constexpr unsigned char binary_4[] = {
			    0x78, 0x01, 0xcb, 0xc9, 0x2c, 0x2e, 0x61, 0x60, 0x60, 0x64,
			    0x60, 0x65, 0x60, 0x60, 0x60, 0x07, 0x62, 0x1e, 0x20, 0xe6,
			    0x03, 0x62, 0x26, 0x20, 0x2e, 0x2e, 0x4a, 0xd6, 0xcf, 0x4d,
			    0xcc, 0xcc, 0xd3, 0x4b, 0x4e, 0x06, 0xb3, 0xf3, 0x73, 0x52,
			    0x74, 0xf3, 0x12, 0x73, 0x53, 0x41, 0x7c, 0x10, 0x00, 0xe9,
			    0x61, 0x04, 0xb3, 0x18, 0x18, 0x82, 0x8c, 0x9d, 0xa5, 0x79,
			    0x42, 0x55, 0x94, 0x9a, 0x98, 0x5d, 0x82, 0xf2, 0x78, 0xc3,
			    0x18, 0xf6, 0xf4, 0xe4, 0xe6, 0x41, 0xa5, 0x50, 0x28, 0x90,
			    0xf9, 0x6c, 0x40, 0x0c, 0xd3, 0x97, 0x72, 0x69, 0xc6, 0x8e,
			    0xe8, 0xeb, 0xa9, 0x8b, 0x4a, 0x1b, 0xfe, 0x2b, 0x3d, 0x97,
			    0xe2, 0x08, 0xf1, 0x3f, 0xa7, 0x28, 0x8f, 0xa2, 0x01, 0xca,
			    0x01, 0x00, 0x09, 0x1d, 0x1a, 0xaf};

			static constexpr unsigned char binary_5[] = {
			    0x78, 0x01, 0x2b, 0x2a, 0x28, 0x2a, 0x61, 0x60, 0x60, 0x64,
			    0x50, 0x65, 0x60, 0x60, 0xe0, 0x03, 0x62, 0x6c, 0xe0, 0x46,
			    0xea, 0xbc, 0xcb, 0x89, 0x47, 0xd7, 0x2b, 0x4c, 0xde, 0xfa,
			    0x47, 0x7e, 0xed, 0xab, 0x47, 0x12, 0x96, 0x69, 0x86, 0xdc,
			    0xc6, 0x40, 0x85, 0xbc, 0x40, 0xcc, 0x08, 0xd5, 0xa0, 0xaa,
			    0xb0, 0x31, 0xc5, 0x00, 0xc8, 0xe6, 0x02, 0x62, 0x69, 0x24,
			    0x1a, 0x24, 0x7f, 0x22, 0xab, 0xfd, 0xfa, 0xf6, 0x35, 0xcd,
			    0xb9, 0x6e, 0x6d, 0x5f, 0x3e, 0x2d, 0x79, 0x14, 0xb8, 0xc6,
			    0x27, 0x4f, 0x38, 0x0d, 0x28, 0xcc, 0x30, 0x9f, 0xf5, 0x7c,
			    0x12, 0x2b, 0x90, 0x86, 0x99, 0x01, 0x12, 0x43, 0x01, 0x9e,
			    0x79, 0x99, 0x25, 0x99, 0x89, 0x39, 0x5c, 0x0c, 0x5e, 0xf9,
			    0x19, 0x79, 0x79, 0x95, 0x0a, 0x8e, 0x05, 0x05, 0x39, 0xa9,
			    0xc5, 0xa9, 0xa9, 0x29, 0x0c, 0x59, 0x60, 0x01, 0x87, 0x44,
			    0x98, 0x80, 0x5e, 0x72, 0x7e, 0x2e, 0x43, 0x6e, 0x62, 0x66,
			    0x1e, 0x48, 0xff, 0xc9, 0x49, 0x0f, 0x26, 0x6c, 0x7b, 0xb4,
			    0xff, 0x5e, 0xf7, 0x05, 0x8d, 0x95, 0x79, 0x5f, 0x5e, 0xf4,
			    0xa4, 0x3d, 0x9a, 0x58, 0x03, 0x12, 0xc7, 0x67, 0x17, 0x00,
			    0x94, 0x70, 0x3b, 0xa0};

			static constexpr unsigned char binary_6[] = {
			    0x78, 0x01, 0x4b, 0xca, 0x49, 0x51, 0x60, 0x60, 0x60, 0x64,
			    0x10, 0x64, 0x00, 0x91, 0x0c, 0x0c, 0xeb, 0xcf, 0xfc, 0x58,
			    0x35, 0x49, 0x62, 0x46, 0x5e, 0xf3, 0x11, 0xd3, 0x84, 0xa7,
			    0xef, 0xd4, 0xb3, 0x85, 0x38, 0x9e, 0x72, 0x03, 0x85, 0x19,
			    0x54, 0x15, 0x36, 0xa6, 0x80, 0x68, 0x5e, 0x20, 0x66, 0x81,
			    0x62, 0x10, 0x1f, 0x1d, 0x00, 0x00, 0xd8, 0x9c, 0x0c, 0xed};

			static constexpr unsigned char binary_7[] = {
			    0x78, 0x01, 0x2b, 0x2a, 0x28, 0x2a, 0x61, 0x60, 0x60, 0x64,
			    0x50, 0x65, 0x60, 0x60, 0x10, 0x00, 0xe2, 0x55, 0x6c, 0x5d,
			    0x0b, 0x96, 0x7e, 0x3c, 0x5c, 0xb8, 0x2d, 0x5a, 0x65, 0xf9,
			    0xae, 0xf8, 0xde, 0xcc, 0x25, 0x66, 0xf2, 0x51, 0xeb, 0xcf,
			    0xfc, 0x58, 0x35, 0x49, 0x62, 0x46, 0x5e, 0xf3, 0x11, 0xd3,
			    0x84, 0xa7, 0xef, 0xd4, 0xb3, 0x85, 0x38, 0x9e, 0x72, 0x9b,
			    0x02, 0xd5, 0xf2, 0x02, 0x31, 0x23, 0x10, 0x83, 0x80, 0xaa,
			    0xc2, 0xc6, 0x14, 0x2b, 0x20, 0x2d, 0x02, 0x62, 0x23, 0xd1,
			    0x20, 0xf9, 0x79, 0xaf, 0xf7, 0x5e, 0xd4, 0x3b, 0x3e, 0x4f,
			    0xe6, 0x8b, 0xb9, 0xe1, 0xf5, 0x8a, 0x0b, 0xa1, 0x9b, 0x3c,
			    0xcc, 0xe3, 0x6f, 0x01, 0x85, 0x19, 0x38, 0x38, 0xce, 0x27,
			    0x81, 0xcc, 0x60, 0x81, 0x62, 0x90, 0x18, 0x0a, 0x70, 0xcb,
			    0xac, 0x50, 0xc8, 0x4b, 0xcc, 0x4d, 0xd5, 0x51, 0x48, 0xc9,
			    0x57, 0xc8, 0xcd, 0x2f, 0x4a, 0xe5, 0x62, 0xf0, 0xca, 0xcf,
			    0xc8, 0xcb, 0xab, 0x54, 0x70, 0x2c, 0x28, 0xc8, 0x49, 0x2d,
			    0x4e, 0x4d, 0x4d, 0x61, 0xc8, 0x02, 0x0b, 0x38, 0x24, 0xc2,
			    0x04, 0xf4, 0x92, 0xf3, 0x73, 0x19, 0x72, 0x13, 0x33, 0xf3,
			    0x18, 0x18, 0xc2, 0x6d, 0x8e, 0xc5, 0xbf, 0x78, 0x7d, 0xec,
			    0xb0, 0xdf, 0xc1, 0xdd, 0xe5, 0x56, 0xce, 0xc7, 0x3e, 0x0a,
			    0x6c, 0xd0, 0xba, 0x08, 0x32, 0x1f, 0x9f, 0x9d, 0x00, 0x2f,
			    0x3d, 0x45, 0x9a};

			static constexpr unsigned char binary_8[] = {
			    0x78, 0x01, 0xcb, 0xc9, 0x2c, 0x2e, 0x61, 0x60, 0x60, 0x64,
			    0x60, 0x65, 0x60, 0x60, 0xe0, 0x00, 0x62, 0x5e, 0x20, 0xe6,
			    0x03, 0x62, 0x26, 0x20, 0x2e, 0x2e, 0x4a, 0xd6, 0x4f, 0x2f,
			    0x4a, 0x4d, 0x2d, 0xc9, 0xcc, 0x4b, 0x2f, 0xd6, 0x4b, 0x4e,
			    0x06, 0x0b, 0xe4, 0x26, 0x66, 0xe6, 0x81, 0xd8, 0x40, 0x20,
			    0x08, 0xc4, 0x20, 0x7d, 0x8c, 0x50, 0x1c, 0x64, 0xec, 0x2c,
			    0xcd, 0x13, 0xaa, 0xa2, 0xd4, 0xc4, 0xec, 0x12, 0x94, 0xc7,
			    0x1b, 0xc6, 0xb0, 0xa7, 0x27, 0x37, 0x0f, 0x28, 0x85, 0x15,
			    0x80, 0xec, 0x62, 0x86, 0xe2, 0x5f, 0x33, 0xb5, 0x2b, 0xa4,
			    0x57, 0xe8, 0xf6, 0x54, 0xda, 0x6c, 0x7e, 0xbc, 0xc7, 0xfb,
			    0xd5, 0xb2, 0xe7, 0x86, 0x67, 0xd5, 0xb0, 0x69, 0x02, 0x00,
			    0xab, 0x9b, 0x1c, 0x89};

			static constexpr unsigned char binary_9[] = {
			    0x78, 0x01, 0xcb, 0xc9, 0x2c, 0x2e, 0x61, 0x60, 0x60, 0x64,
			    0x60, 0x65, 0x60, 0x60, 0x60, 0x02, 0x62, 0x76, 0x20, 0xe6,
			    0x03, 0x62, 0x46, 0x20, 0xce, 0x4d, 0xcc, 0xcc, 0xd3, 0x4b,
			    0x4e, 0x06, 0xb2, 0x18, 0xc0, 0xf2, 0x20, 0x31, 0x10, 0x58,
			    0xe5, 0x70, 0xea, 0x7a, 0x85, 0xd3, 0x7d, 0xde, 0x87, 0xfc,
			    0xf3, 0x16, 0x26, 0xdd, 0x5e, 0xb7, 0x25, 0xd3, 0x94, 0x6d,
			    0x3d, 0x44, 0x06, 0x95, 0x04, 0x00, 0x86, 0x0c, 0x0e, 0xcc};

			static constexpr unsigned char binary_10[] = {
			    0x78, 0x01, 0x4b, 0xca, 0x49, 0x51, 0x60, 0x60, 0x60, 0x64,
			    0x10, 0x64, 0x00, 0x91, 0x0c, 0x0c, 0xcd, 0x0b, 0xdb, 0xfe,
			    0xf1, 0x28, 0xaf, 0x0f, 0x3a, 0xff, 0xce, 0x2d, 0xda, 0x28,
			    0x42, 0xfc, 0xba, 0xfe, 0x9f, 0x1d, 0x1b, 0x81, 0xc2, 0x0c,
			    0xaa, 0x0a, 0x1b, 0x53, 0x40, 0x34, 0x37, 0x10, 0x33, 0x81,
			    0x18, 0x38, 0x00, 0x00, 0xcf, 0x87, 0x0d, 0x0f};

			static constexpr unsigned char binary_11[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x37, 0x66,
			    0x28, 0x28, 0xca, 0xcf, 0x4a, 0x4d, 0x2e, 0xd1, 0x28, 0x49,
			    0x2d, 0x2e, 0x51, 0x70, 0x8e, 0x88, 0xd0, 0xe4, 0x4a, 0x4c,
			    0x49, 0x89, 0x4f, 0xad, 0x48, 0x4d, 0x2e, 0x2d, 0x49, 0x4c,
			    0xca, 0x49, 0x05, 0x4b, 0x70, 0x29, 0x28, 0x14, 0x17, 0x25,
			    0xeb, 0xe7, 0x26, 0x66, 0xe6, 0xe9, 0x25, 0x27, 0x43, 0x79,
			    0xe9, 0x45, 0xa9, 0xa9, 0x25, 0x99, 0x79, 0xe9, 0xc5, 0x20,
			    0x21, 0x4d, 0x2e, 0x00, 0x65, 0x50, 0x1a, 0xf5};

			static constexpr unsigned char binary_12[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x36, 0x62,
			    0x50, 0x2e, 0x28, 0x4a, 0x4c, 0xcf, 0x4d, 0x54, 0xc8, 0xcf,
			    0x4b, 0x4e, 0xe5, 0xe2, 0x2a, 0xcb, 0xcf, 0x4c, 0x51, 0x48,
			    0x2f, 0x4a, 0x4d, 0x2d, 0xc9, 0xcc, 0x4b, 0x2f, 0xd6, 0xd0,
			    0xb4, 0xe6, 0x02, 0x00, 0x06, 0xb2, 0x0c, 0xc9};

			static constexpr unsigned char binary_13[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x35,
			    0x61, 0x50, 0xce, 0xcc, 0x4b, 0xce, 0x29, 0x4d, 0x49,
			    0x55, 0x50, 0x4a, 0x2f, 0x4a, 0x4d, 0x2d, 0xc9, 0xcc,
			    0x4b, 0x2f, 0xd6, 0xcb, 0xc8, 0x50, 0xe2, 0xe2, 0xca,
			    0xcc, 0x2b, 0x51, 0xc8, 0x4d, 0xcc, 0xcc, 0xd3, 0xd0,
			    0x54, 0xa8, 0xe6, 0x52, 0x50, 0x80, 0xcb, 0x6a, 0x68,
			    0x5a, 0x73, 0xd5, 0x02, 0x00, 0x70, 0x3a, 0x13, 0x8f};

			static constexpr unsigned char binary_14[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30,
			    0x67, 0x48, 0x2a, 0xcd, 0xcc, 0x49, 0xd1, 0xe7,
			    0x02, 0x00, 0x22, 0x3c, 0x04, 0x40};

			static constexpr unsigned char binary_15[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0xb3, 0x64,
			    0x50, 0xce, 0xcc, 0x4b, 0xce, 0x29, 0x4d, 0x49, 0x55, 0xb0,
			    0xc9, 0xcc, 0x2f, 0x2e, 0x29, 0x4a, 0x4d, 0xcc, 0xb5, 0xe3,
			    0xe2, 0xca, 0xcc, 0x2b, 0x51, 0xc8, 0x4d, 0xcc, 0xcc, 0xd3,
			    0xd0, 0x54, 0xa8, 0xe6, 0x52, 0x50, 0x28, 0x2e, 0x49, 0xb1,
			    0xb2, 0x4a, 0xce, 0x2f, 0x2d, 0x51, 0xb0, 0xb1, 0x51, 0x50,
			    0xf2, 0x48, 0xcd, 0xc9, 0xc9, 0xd7, 0x51, 0x28, 0xcf, 0x2f,
			    0xca, 0x49, 0x51, 0x8c, 0xc9, 0x53, 0xb2, 0xe6, 0xaa, 0x05,
			    0x00, 0xb8, 0xe4, 0x18, 0x19};

			static constexpr unsigned char binary_16[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x34, 0x34,
			    0x65, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x48, 0x07,
			    0xf2, 0x4b, 0x32, 0xf3, 0xd2, 0x8b, 0xf5, 0x92, 0x93, 0x19,
			    0x7e, 0xcd, 0xd4, 0xae, 0x90, 0x5e, 0xa1, 0xdb, 0x53, 0x69,
			    0xb3, 0xf9, 0xf1, 0x1e, 0xef, 0x57, 0xcb, 0x9e, 0x1b, 0x9e,
			    0x55, 0xc3, 0x50, 0x97, 0x91, 0xc1, 0xb0, 0x62, 0xcd, 0xf5,
			    0xea, 0x8d, 0x49, 0xea, 0x9b, 0x96, 0x2c, 0x0d, 0xac, 0x68,
			    0x77, 0xd0, 0xa9, 0xe3, 0xd0, 0x8f, 0x68, 0x86, 0xaa, 0xcb,
			    0x4d, 0xcc, 0xcc, 0x03, 0x19, 0x15, 0x64, 0xec, 0x2c, 0xcd,
			    0x13, 0xaa, 0xa2, 0xd4, 0xc4, 0xec, 0x12, 0x94, 0xc7, 0x1b,
			    0xc6, 0xb0, 0xa7, 0x27, 0x37, 0x0f, 0x00, 0xfc, 0x42, 0x2b,
			    0xcf};

			static constexpr unsigned char binary_17[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x34, 0x34,
			    0x60, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0xd0, 0x4b,
			    0xcf, 0x2c, 0xc9, 0x4c, 0xcf, 0xcb, 0x2f, 0x4a, 0x65, 0x08,
			    0x2b, 0xe3, 0xdc, 0xa8, 0xec, 0x35, 0xbb, 0x83, 0xed, 0xe8,
			    0x82, 0xd8, 0x65, 0x27, 0xd2, 0x9e, 0x34, 0xac, 0x12, 0xe9,
			    0x85, 0xaa, 0x72, 0xf6, 0x4d, 0xcc, 0x4e, 0xf5, 0xc9, 0x2c,
			    0x2e, 0x29, 0xd6, 0x2b, 0xa9, 0x28, 0x61, 0x28, 0x3b, 0x78,
			    0x39, 0xaa, 0xd2, 0xce, 0xf5, 0xc5, 0x0f, 0x3e, 0x9b, 0x18,
			    0xbf, 0xd7, 0xf6, 0x6b, 0x15, 0xaa, 0x1d, 0xdf, 0x9b, 0x18,
			    0x00, 0x81, 0x42, 0x71, 0x51, 0x32, 0x43, 0xd1, 0xd1, 0xfe,
			    0xaa, 0xe4, 0xb8, 0xfb, 0x09, 0xcb, 0x0e, 0x07, 0x9d, 0x2a,
			    0x9c, 0x70, 0xea, 0xc5, 0x1a, 0xfb, 0xd4, 0x7f, 0x00, 0xfa,
			    0x9a, 0x2f, 0xae};

			static constexpr unsigned char binary_18[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x31, 0x67,
			    0x28, 0x28, 0xca, 0xcf, 0x4a, 0x4d, 0x2e, 0xd1, 0x28, 0x49,
			    0x2d, 0x2e, 0x51, 0x70, 0x8e, 0x88, 0xd0, 0xe4, 0x4a, 0x4c,
			    0x49, 0x89, 0x4f, 0xad, 0x48, 0x4d, 0x2e, 0x2d, 0x49, 0x4c,
			    0xca, 0x49, 0x85, 0x48, 0xe4, 0x26, 0x66, 0xe6, 0xe9, 0x25,
			    0x27, 0x6b, 0x72, 0x01, 0x00, 0x0f, 0x0c, 0x12, 0xce};

			static constexpr unsigned char binary_19[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x34, 0x34,
			    0x65, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0xd0, 0x4b,
			    0xcf, 0x2c, 0xc9, 0x4c, 0xcf, 0xcb, 0x2f, 0x4a, 0x65, 0x08,
			    0x2b, 0xe3, 0xdc, 0xa8, 0xec, 0x35, 0xbb, 0x83, 0xed, 0xe8,
			    0x82, 0xd8, 0x65, 0x27, 0xd2, 0x9e, 0x34, 0xac, 0x12, 0xe9,
			    0x85, 0xaa, 0x72, 0xf6, 0x4d, 0xcc, 0x4e, 0xf5, 0xc9, 0x2c,
			    0x2e, 0x29, 0xd6, 0x2b, 0xa9, 0x28, 0x61, 0xd0, 0x9d, 0x36,
			    0xf1, 0x48, 0xca, 0xd6, 0x77, 0xcd, 0xf3, 0x36, 0x1f, 0x4f,
			    0xd2, 0xca, 0x94, 0x88, 0x3e, 0xfb, 0x5b, 0xa0, 0x15, 0xaa,
			    0x32, 0x37, 0x31, 0x33, 0x4f, 0x2f, 0x39, 0x99, 0x61, 0x95,
			    0xc3, 0xa9, 0xeb, 0x15, 0x4e, 0xf7, 0x79, 0x1f, 0xf2, 0xcf,
			    0x5b, 0x98, 0x74, 0x7b, 0xdd, 0x96, 0x4c, 0x53, 0xb6, 0xf5,
			    0x00, 0xee, 0x40, 0x30, 0x74};

			static constexpr unsigned char binary_20[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x34, 0x34,
			    0x61, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x48, 0x07,
			    0xf2, 0x4b, 0x32, 0xf3, 0xd2, 0x8b, 0xf5, 0x32, 0x32, 0x18,
			    0x56, 0xac, 0xb9, 0x5e, 0xbd, 0x31, 0x49, 0x7d, 0xd3, 0x92,
			    0xa5, 0x81, 0x15, 0xed, 0x0e, 0x3a, 0x75, 0x1c, 0xfa, 0x11,
			    0xcd, 0x50, 0x75, 0xb9, 0x89, 0x99, 0x79, 0x7a, 0xc9, 0xc9,
			    0x0c, 0x41, 0xc6, 0xce, 0xd2, 0x3c, 0xa1, 0x2a, 0x4a, 0x4d,
			    0xcc, 0x2e, 0x41, 0x79, 0xbc, 0x61, 0x0c, 0x7b, 0x7a, 0x72,
			    0xf3, 0xa0, 0x4a, 0xf2, 0x73, 0x52, 0x74, 0xf3, 0x12, 0x73,
			    0x53, 0x41, 0xca, 0x52, 0x2e, 0xcd, 0xd8, 0x11, 0x7d, 0x3d,
			    0x75, 0x51, 0x69, 0xc3, 0x7f, 0xa5, 0xe7, 0x52, 0x1c, 0x21,
			    0xfe, 0xe7, 0x14, 0xe5, 0x01, 0xc9, 0xf3, 0x2a, 0x03};

			static constexpr unsigned char binary_21[] = {
			    0x78, 0x01, 0xa5, 0xce, 0x41, 0x6a, 0xc3, 0x30, 0x10, 0x40,
			    0xd1, 0xac, 0x75, 0x8a, 0xd9, 0x37, 0x14, 0xcd, 0x58, 0x1a,
			    0xc9, 0x10, 0x42, 0xba, 0xc9, 0xa2, 0xb7, 0x18, 0x49, 0x53,
			    0x9c, 0x10, 0x59, 0xc6, 0x51, 0xa1, 0xb9, 0x7d, 0x4c, 0xa0,
			    0x27, 0xc8, 0xf6, 0x2d, 0x3e, 0x3f, 0xb7, 0x5a, 0x2f, 0x1d,
			    0xc8, 0xb9, 0x5d, 0x5f, 0x55, 0x21, 0x50, 0x46, 0x46, 0xf4,
			    0xe2, 0xac, 0x1f, 0xcb, 0x10, 0x8b, 0x8f, 0x89, 0xc4, 0x95,
			    0x94, 0x90, 0x84, 0x28, 0x93, 0x04, 0xc9, 0x31, 0x0f, 0x66,
			    0x91, 0x55, 0xe7, 0x0e, 0x65, 0x24, 0x8f, 0x03, 0x4b, 0xe6,
			    0x94, 0x3c, 0xbb, 0x88, 0x31, 0xb1, 0x72, 0x1a, 0x23, 0x8f,
			    0x9a, 0x7e, 0x62, 0x40, 0x15, 0x1b, 0x94, 0x83, 0x91, 0xdf,
			    0x3e, 0xb5, 0x15, 0xbe, 0xdb, 0x34, 0xcf, 0x0f, 0xf8, 0x5a,
			    0x96, 0x9b, 0xde, 0x55, 0x0b, 0x1c, 0xae, 0x2f, 0x39, 0xc9,
			    0xbf, 0x7c, 0xe6, 0x56, 0x8f, 0x80, 0xec, 0x43, 0x18, 0x3c,
			    0x06, 0x86, 0x0f, 0x4b, 0xd6, 0x9a, 0x4d, 0xb7, 0xd1, 0xae,
			    0x6f, 0x24, 0xcc, 0xf9, 0xf2, 0x07, 0xb3, 0x54, 0xdd, 0x43,
			    0x69, 0x50, 0xdb, 0xaa, 0xe6, 0x09, 0x94, 0xe4, 0x4e, 0x19};

			static constexpr unsigned char binary_22[] = {
			    0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x34, 0x34,
			    0x60, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0xd0, 0x4b,
			    0xcf, 0x2c, 0xc9, 0x4c, 0xcf, 0xcb, 0x2f, 0x4a, 0x65, 0x08,
			    0x2b, 0xe3, 0xdc, 0xa8, 0xec, 0x35, 0xbb, 0x83, 0xed, 0xe8,
			    0x82, 0xd8, 0x65, 0x27, 0xd2, 0x9e, 0x34, 0xac, 0x12, 0xe9,
			    0x85, 0xaa, 0x72, 0xf6, 0x4d, 0xcc, 0x4e, 0xf5, 0xc9, 0x2c,
			    0x2e, 0x29, 0xd6, 0x2b, 0xa9, 0x28, 0x61, 0xf8, 0x78, 0xc9,
			    0xc6, 0x83, 0x7b, 0x12, 0xff, 0xb6, 0x1d, 0x5d, 0x45, 0xcf,
			    0x4e, 0xb2, 0x47, 0x64, 0x9e, 0xd0, 0xd1, 0x90, 0x37, 0x31,
			    0x00, 0x02, 0x85, 0xe2, 0xa2, 0x64, 0x06, 0x76, 0xeb, 0xb7,
			    0x5d, 0x1c, 0x4c, 0x87, 0xe7, 0xed, 0xc9, 0x7f, 0xbe, 0xea,
			    0x05, 0xbf, 0xec, 0xe3, 0xba, 0x23, 0xf1, 0xd7, 0x00, 0xcc,
			    0x25, 0x2d, 0x6f};

			static constexpr unsigned char binary_23[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x35,
			    0x64, 0x28, 0x28, 0xca, 0xcf, 0x4a, 0x4d, 0x2e, 0xd1,
			    0x28, 0x49, 0x2d, 0x2e, 0x51, 0x70, 0x8e, 0x88, 0xd0,
			    0xe4, 0x4a, 0x4c, 0x49, 0x89, 0x4f, 0xad, 0x48, 0x4d,
			    0x2e, 0x2d, 0x49, 0x4c, 0xca, 0x49, 0x85, 0x48, 0x14,
			    0x17, 0x25, 0xeb, 0xe7, 0x26, 0x66, 0xe6, 0xe9, 0x25,
			    0x27, 0x6b, 0x72, 0x01, 0x00, 0x5f, 0x52, 0x14, 0x40};

			static constexpr unsigned char binary_24[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0xb0, 0xb4, 0x64,
			    0x50, 0xce, 0xcc, 0x4b, 0xce, 0x29, 0x4d, 0x49, 0x55, 0x50,
			    0x4a, 0x2f, 0x4a, 0x4d, 0x2d, 0xc9, 0xcc, 0x4b, 0x2f, 0xd6,
			    0xcb, 0xc8, 0x50, 0xe2, 0x82, 0x8b, 0xdb, 0x64, 0xe6, 0x17,
			    0x97, 0x14, 0xa5, 0x26, 0xe6, 0xda, 0x71, 0x71, 0x95, 0xe5,
			    0x67, 0xa6, 0x28, 0xc0, 0xd5, 0x69, 0x68, 0x2a, 0x54, 0x73,
			    0x29, 0x28, 0x14, 0x97, 0xa4, 0x58, 0x59, 0x25, 0xe7, 0x97,
			    0x96, 0x28, 0xd8, 0xd8, 0x28, 0x28, 0x79, 0xa4, 0xe6, 0xe4,
			    0xe4, 0xeb, 0x28, 0x94, 0xe7, 0x17, 0xe5, 0xa4, 0x28, 0xc6,
			    0xe4, 0x29, 0x59, 0x73, 0xd5, 0x02, 0x00, 0x7b, 0xb7, 0x22,
			    0xe1};

			static constexpr unsigned char binary_25[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x33,
			    0x60, 0x50, 0xce, 0xcc, 0x4b, 0xce, 0x29, 0x4d, 0x49, 0x55,
			    0x50, 0x4a, 0x2f, 0x4a, 0x4d, 0x2d, 0xc9, 0xcc, 0x4b, 0x2f,
			    0xd6, 0xcb, 0xc8, 0x50, 0xe2, 0x82, 0x8b, 0xdb, 0x64, 0xe6,
			    0x17, 0x97, 0x14, 0xa5, 0x26, 0xe6, 0xda, 0x71, 0x71, 0x95,
			    0xe5, 0x67, 0xa6, 0x28, 0xc0, 0xd5, 0x69, 0x68, 0x2a, 0x54,
			    0x73, 0x29, 0x28, 0x14, 0x97, 0xa4, 0x58, 0x59, 0x25, 0xe7,
			    0x97, 0x96, 0x28, 0xd8, 0xd8, 0x28, 0x28, 0x79, 0xa4, 0xe6,
			    0xe4, 0xe4, 0xeb, 0x28, 0x94, 0xe7, 0x17, 0xe5, 0xa4, 0x28,
			    0xc6, 0xe4, 0x29, 0x59, 0x63, 0xa8, 0x08, 0x71, 0x72, 0xc1,
			    0x2e, 0x9e, 0x91, 0x59, 0x94, 0xa2, 0x50, 0x92, 0x01, 0x74,
			    0x82, 0x42, 0x46, 0x6a, 0x51, 0x2a, 0x58, 0x51, 0x2d, 0x00,
			    0x05, 0x20, 0x35, 0x63};

			static constexpr unsigned char binary_26[] = {
			    0x78, 0x01, 0xa5, 0xce, 0x41, 0x6a, 0xc3, 0x30, 0x10, 0x85,
			    0xe1, 0xac, 0x75, 0x8a, 0xd9, 0x07, 0x8a, 0x64, 0x8d, 0x67,
			    0x24, 0x08, 0xa1, 0x59, 0x64, 0xd3, 0x5b, 0x8c, 0x47, 0x63,
			    0x9c, 0x10, 0xcb, 0xc6, 0x55, 0x20, 0xb9, 0x7d, 0x4d, 0xa1,
			    0x27, 0xe8, 0xf6, 0x5b, 0xfc, 0xef, 0xe9, 0x32, 0xcf, 0xb7,
			    0x06, 0x5d, 0xe4, 0x43, 0xdb, 0xcc, 0x00, 0x35, 0x1a, 0x0e,
			    0x3d, 0x93, 0x4a, 0x2e, 0x3e, 0x14, 0x45, 0xc9, 0x9a, 0x63,
			    0xa6, 0x3c, 0xf4, 0x21, 0xea, 0x48, 0xbe, 0x70, 0xce, 0x09,
			    0xdd, 0x2a, 0x9b, 0xd5, 0x06, 0x9a, 0x48, 0x12, 0x17, 0x1e,
			    0x58, 0x34, 0x45, 0x2a, 0x48, 0x89, 0x46, 0x1c, 0x3b, 0x41,
			    0xeb, 0xfa, 0x20, 0x8a, 0x4a, 0x16, 0x22, 0x91, 0x93, 0x67,
			    0x9b, 0x96, 0x0d, 0xbe, 0x96, 0xa9, 0xd6, 0x37, 0x5c, 0xd6,
			    0xf5, 0x61, 0xdf, 0x66, 0x05, 0x4e, 0xf7, 0x5f, 0xf9, 0x94,
			    0x3f, 0xf9, 0xd0, 0x65, 0x3e, 0x43, 0xa0, 0x9e, 0x39, 0x22,
			    0x27, 0x82, 0xa3, 0xef, 0xbc, 0x77, 0xbb, 0xee, 0x47, 0x9b,
			    0xfd, 0x23, 0xe1, 0xae, 0xaf, 0x66, 0xb5, 0xc0, 0x3e, 0xe5,
			    0x7e, 0x00, 0xb0, 0xc2, 0x4c, 0x3c};

			static constexpr unsigned char binary_27[] = {
			    0x78, 0x01, 0xa5, 0xcd, 0xcd, 0x0a, 0x02, 0x21, 0x14, 0x86,
			    0xe1, 0xd6, 0x5e, 0xc5, 0xd9, 0x07, 0xe1, 0xc9, 0x7f, 0x88,
			    0xa8, 0x65, 0xdd, 0x85, 0x3a, 0x47, 0xc6, 0x18, 0x75, 0x98,
			    0x6c, 0xd1, 0xdd, 0x27, 0x41, 0x57, 0xd0, 0xf6, 0x79, 0xe1,
			    0xfb, 0x62, 0x2b, 0x25, 0x77, 0x40, 0xab, 0x77, 0x7d, 0x23,
			    0x02, 0xa2, 0xe4, 0xac, 0xf0, 0x12, 0x09, 0xa3, 0x49, 0x8a,
			    0x07, 0x95, 0x22, 0x97, 0xd6, 0xeb, 0x34, 0x61, 0xe0, 0x06,
			    0x47, 0x51, 0x2e, 0x44, 0xc1, 0xfc, 0xab, 0xcf, 0x6d, 0x83,
			    0x7b, 0x9b, 0x6b, 0x7d, 0xc3, 0x75, 0x5d, 0x17, 0x7a, 0x12,
			    0x4d, 0x70, 0x7a, 0x7c, 0xe5, 0xe2, 0x7f, 0x72, 0x88, 0xad,
			    0x9c, 0x01, 0xb5, 0x32, 0x46, 0x48, 0xa5, 0x1c, 0xec, 0xf9,
			    0x91, 0x73, 0x36, 0x74, 0x1c, 0x77, 0xfa, 0x63, 0x82, 0xdd,
			    0x6a, 0xee, 0xd9, 0x2f, 0xec, 0x03, 0x2d, 0x35, 0x3d, 0xf9};

			static constexpr unsigned char binary_28[] = {
			    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x37, 0x62,
			    0x28, 0x28, 0xca, 0xcf, 0x4a, 0x4d, 0x2e, 0xd1, 0x28, 0x49,
			    0x2d, 0x2e, 0x51, 0x70, 0x8e, 0x88, 0xd0, 0xe4, 0x4a, 0x4c,
			    0x49, 0x89, 0x4f, 0xad, 0x48, 0x4d, 0x2e, 0x2d, 0x49, 0x4c,
			    0xca, 0x49, 0x05, 0x4b, 0x70, 0x29, 0x28, 0x14, 0x17, 0x25,
			    0xeb, 0xe7, 0x26, 0x66, 0xe6, 0xe9, 0x25, 0x27, 0x43, 0x79,
			    0xf9, 0x39, 0x29, 0xba, 0x79, 0x89, 0xb9, 0xa9, 0x20, 0x11,
			    0x4d, 0x2e, 0x00, 0x45, 0xa4, 0x1a, 0x39};
		}  // namespace file

		constexpr std::string_view subdirs[] = {
		    "diffs/.git/.covdata/objects/pack"sv,
		    "diffs/.git/.covdata/refs/tags"sv,
		    "diffs/.git/objects/pack"sv,
		    "diffs/.git/objects/info"sv,
		    "diffs/.git/refs/tags"sv,
		    "diffs/.git/branches"sv,
		};

		constexpr file::text text[] = {
		    {"diffs/.git/.covdata/HEAD"sv, file::text_8},
		    {"diffs/.git/.covdata/config"sv, file::text_9},
		    {"diffs/.git/.covdata/refs/heads/main"sv, file::text_10},
		    {"diffs/.git/HEAD"sv, file::text_6},
		    {"diffs/.git/config"sv, file::text_7},
		    {"diffs/.git/refs/heads/master"sv, file::text_11},
		    {"diffs/.gitignore"sv, file::text_2},
		    {"diffs/CMakeLists.txt"sv, file::text_1},
		    {"diffs/src/greetings.cc"sv, file::text_4},
		    {"diffs/src/greetings.hh"sv, file::text_3},
		    {"diffs/src/main.cc"sv, file::text_5},
		    {"secured.txt"sv, file::safe},
		    {"unsecured.txt"sv, file::safe},
		};

		constexpr file::binary binary[] = {
		    {"diffs/.git/.covdata/objects/coverage/40/1ef2aa698c20393bb6969d773ce2c781928265"sv,
		     span(file::binary_10)},
		    {"diffs/.git/.covdata/objects/coverage/57/3cc65fe8ebc6c34ec1bb773a43c6f110b02ad1"sv,
		     span(file::binary_6)},
		    {"diffs/.git/.covdata/objects/coverage/83/a186fe0c23af52cfee465b325817d72ffcb8b1"sv,
		     span(file::binary_4)},
		    {"diffs/.git/.covdata/objects/coverage/90/7e28cba8e972e07822008b5dd088b443b4e21b"sv,
		     span(file::binary_5)},
		    {"diffs/.git/.covdata/objects/coverage/92/8c96e91cc3b4f2b14038993c9f205a266b1da3"sv,
		     span(file::binary_7)},
		    {"diffs/.git/.covdata/objects/coverage/aa/068aa0a5f1c371b65b24a7ba5f8d69a4361f5a"sv,
		     span(file::binary_3)},
		    {"diffs/.git/.covdata/objects/coverage/af/ccf8aa9218986e83c43560e5ee276b1208e50b"sv,
		     span(file::binary_8)},
		    {"diffs/.git/.covdata/objects/coverage/c9/92e090b6e2bfde8bd028a96ef4e88c66e2917c"sv,
		     span(file::binary_2)},
		    {"diffs/.git/.covdata/objects/coverage/d8/659ed361c5af2093b5fc1fadeae2183966310b"sv,
		     span(file::binary_9)},
		    {"diffs/.git/index"sv, span(file::binary_1)},
		    {"diffs/.git/objects/07/3bed8a0802c39ebc6fe7aae80f1de37ec45fd6"sv,
		     span(file::binary_20)},
		    {"diffs/.git/objects/2d/9691c464b5ee839eb3c7622a69185bcdfb1085"sv,
		     span(file::binary_18)},
		    {"diffs/.git/objects/4c/3e4b576ca9d01dc4a9c93969b513cf60d79984"sv,
		     span(file::binary_22)},
		    {"diffs/.git/objects/52/33431b0c552422820344526e0d5600bc8c6d6e"sv,
		     span(file::binary_13)},
		    {"diffs/.git/objects/56/7609b1234a9b8806c5a05da6c866e480aa148d"sv,
		     span(file::binary_14)},
		    {"diffs/.git/objects/5a/d067ce4c73544ec96bf56b776e4c93b1014d8f"sv,
		     span(file::binary_23)},
		    {"diffs/.git/objects/64/d298b85bd765a27580ff22e71a08544fce211f"sv,
		     span(file::binary_24)},
		    {"diffs/.git/objects/72/c16115a4059d38d58b2a4dbb12a22c2a7ac8c3"sv,
		     span(file::binary_17)},
		    {"diffs/.git/objects/72/c58f7a635edf60a6c352ca7190cae8ac3f65fe"sv,
		     span(file::binary_16)},
		    {"diffs/.git/objects/76/c1d35a793e45e8f80e3c5c4eeb3fad207b41ef"sv,
		     span(file::binary_11)},
		    {"diffs/.git/objects/9e/ebbdd12ec79e1cf43731d778d055b248375fda"sv,
		     span(file::binary_21)},
		    {"diffs/.git/objects/a8/acd77bb16227b2a4a5517887402c7e082f5883"sv,
		     span(file::binary_12)},
		    {"diffs/.git/objects/aa/40cad77842df0de10f9ea162dbaeb4693506af"sv,
		     span(file::binary_15)},
		    {"diffs/.git/objects/c8/6a87d7b7ac836d4686f4f2a4e251ac4c6e1366"sv,
		     span(file::binary_27)},
		    {"diffs/.git/objects/d9/25136ac6bb564818b6e6b9869ebf871ea07e67"sv,
		     span(file::binary_26)},
		    {"diffs/.git/objects/ee/f983a41e1c7f50b5fc048a6fd1b071a4159bc3"sv,
		     span(file::binary_19)},
		    {"diffs/.git/objects/f1/d23c480b920fb6b88a72e6c9075869c82c281f"sv,
		     span(file::binary_28)},
		    {"diffs/.git/objects/fa/992b781ba82d8c793cb3e3bc4beaa6e731cd26"sv,
		     span(file::binary_25)},
		};
	}  // namespace

	void test_globals::setup_test_env() {
		printf("Setting up test environment\n");
		unpack_files(test_dir(), setup::subdirs, setup::text, setup::binary);
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
		remove_all(test_dir(), ignore);
	}
}  // namespace cov::testing::setup
