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
			static constexpr auto main =
			    "ed631389fc343f7788bf414c2b3e77749a15deb6"sv;
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

			constexpr unsigned char
			    objects_ed_631389fc343f7788bf414c2b3e77749a15deb6[] = {
			        0x78, 0x01, 0x9d, 0x8d, 0x41, 0x0a, 0xc2, 0x30, 0x10, 0x00,
			        0x3d, 0xe7, 0x15, 0x7b, 0x17, 0x64, 0x93, 0x36, 0xcd, 0x06,
			        0x8a, 0x78, 0xf5, 0xe0, 0x07, 0xbc, 0x65, 0x9b, 0xd4, 0x2e,
			        0x98, 0x14, 0xea, 0xf6, 0xe2, 0xeb, 0xed, 0x1b, 0x3c, 0x0d,
			        0x0c, 0x0c, 0x33, 0xad, 0xb5, 0x8a, 0x82, 0x25, 0x3c, 0xe9,
			        0x56, 0x0a, 0x04, 0xcb, 0x1c, 0x22, 0xa6, 0xbe, 0x50, 0x61,
			        0x74, 0x4c, 0x6e, 0x8e, 0x3d, 0xe5, 0x99, 0x6d, 0x97, 0x63,
			        0x37, 0x44, 0x0e, 0x03, 0x65, 0xec, 0x83, 0x49, 0xbb, 0x2e,
			        0xeb, 0x06, 0x8f, 0xb4, 0x4d, 0xd2, 0xe0, 0x99, 0xf7, 0x06,
			        0x63, 0xfd, 0x1e, 0xb8, 0x55, 0xc9, 0x4d, 0x5e, 0x8b, 0xb2,
			        0xe8, 0xe7, 0x32, 0xad, 0xf5, 0x0a, 0x76, 0xf0, 0xde, 0x07,
			        0x6f, 0x89, 0xe0, 0x8c, 0x0e, 0xd1, 0x1c, 0xf6, 0xb8, 0x6a,
			        0xf9, 0xb7, 0x37, 0xf7, 0x26, 0x2a, 0xe9, 0x6d, 0x7e, 0x47,
			        0xfc, 0x3b, 0x35};

			constexpr unsigned char
			    objects_d8_5c6a23a700aa20fda7cfae8eaa9e80ea22dde0[] = {
			        0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30,
			        0x34, 0x63, 0x50, 0x56, 0x08, 0x49, 0x2d, 0x2e,
			        0xc9, 0xcc, 0x4b, 0x57, 0x28, 0x4a, 0x2d, 0xc8,
			        0x2f, 0xe6, 0x02, 0x00, 0x5b, 0x5a, 0x07, 0x9b};

			constexpr unsigned char
			    objects_71_bb790a4e8eb02b82f948dfb13d9369b768d047[] = {
			        0x78, 0x01, 0x2b, 0x29, 0x4a, 0x4d, 0x55, 0x30, 0x36,
			        0x67, 0x30, 0x34, 0x30, 0x30, 0x33, 0x31, 0x51, 0x08,
			        0x72, 0x75, 0x74, 0xf1, 0x75, 0xd5, 0xcb, 0x4d, 0x61,
			        0xb8, 0x11, 0x93, 0xa5, 0xbc, 0x9c, 0x61, 0x95, 0xc2,
			        0xdf, 0xe5, 0xe7, 0xd7, 0xf5, 0xad, 0x9a, 0xd7, 0xf0,
			        0x4a, 0xe9, 0xee, 0x03, 0x00, 0x3e, 0xdc, 0x11, 0xa9};

			struct type {
				std::string_view path{};
				std::string_view content{};
			};

			struct binary {
				std::string_view path{};
				std::basic_string_view<unsigned char> content{};
			};
		}  // namespace file

		constexpr std::string_view subdirs[] = {
		    "gitdir/.git/objects/info"sv, "gitdir/.git/objects/pack"sv,
		    "gitdir/.git/refs/heads"sv,   "gitdir/.git/refs/tags"sv,
		    "bare.git/objects/info"sv,    "bare.git/objects/pack"sv,
		    "bare.git/refs/heads"sv,      "bare.git/refs/tags"sv,
		};

		constexpr file::type files[] = {
		    {"gitdir/.git/HEAD"sv, file::HEAD},
		    {"gitdir/.git/config"sv, file::config},
		    {"bare.git/HEAD"sv, file::HEAD},
		    {"bare.git/config"sv, file::config_bare},
		    {"bare.git/refs/heads/main"sv, file::main},
		    {"gitdir/subdir/a-file"sv},
		};

		template <size_t Length>
		std::basic_string_view<unsigned char> span(
		    unsigned char const (&v)[Length]) noexcept {
			return {v, Length};
		}

		file::binary binary[] = {
		    {"bare.git/objects/ed/631389fc343f7788bf414c2b3e77749a15deb6"sv,
		     span(file::objects_ed_631389fc343f7788bf414c2b3e77749a15deb6)},
		    {"bare.git/objects/d8/5c6a23a700aa20fda7cfae8eaa9e80ea22dde0"sv,
		     span(file::objects_d8_5c6a23a700aa20fda7cfae8eaa9e80ea22dde0)},
		    {"bare.git/objects/71/bb790a4e8eb02b82f948dfb13d9369b768d047"sv,
		     span(file::objects_71_bb790a4e8eb02b82f948dfb13d9369b768d047)},
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
			printf("+ %s\n", p.string().c_str());
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p};
			out.write(contents.data(), contents.size());
		}

		for (auto const nfo : setup::binary) {
			auto const p = test_dir() / make_path(nfo.path);
			printf("+ %s\n", p.string().c_str());
			create_directories(p.parent_path(), ignore);
			std::ofstream out{p};
			out.write(reinterpret_cast<char const*>(nfo.content.data()),
			          nfo.content.size());
		}
	}

	void test_globals::teardown_test_env() {
		printf("Tearing down test environment\n");
		using namespace std::filesystem;
		std::error_code ignore{};
		// remove_all(test_dir(), ignore);
	}
}  // namespace git::testing::setup

void PrintTo(std::filesystem::path const& path, ::std::ostream* os) {
	*os << git::testing::setup::get_path(path);
}
