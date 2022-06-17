#include <gtest/gtest.h>
#include <filesystem>
#include <git2-c++/global.hh>
#include <git2-c++/repository.hh>
#include <fstream>

extern std::filesystem::path TESTING_data_path;

namespace git::testing {
	using namespace ::std::literals;
	using std::filesystem::path;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	namespace setup {
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
	}  // namespace setup

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

	std::string append(path const& root, std::string_view utf8) {
		return get_path(root / conv<char8_t>(utf8));
	}

	std::string make_path(std::string_view utf8) {
		return get_path(path{conv<char8_t>(utf8)});
	}
#else
	std::string get_path(path const& p) { return p.generic_u8string(); }
	std::string append(path const& root, std::string_view utf8) {
		return get_path(root / std::filesystem::u8path(utf8));
	}

	std::string make_path(std::string_view utf8) {
		return get_path(std::filesystem::u8path(utf8));
	}
#endif

	std::string make_absolute(std::string_view utf8, bool absolute) {
		if (absolute) return make_path(utf8);

		return get_path(append(TESTING_data_path, utf8));
	}

	struct repo_param {
		std::string_view start_path{};
		std::string_view expected{};
		bool absolute{false};
	};

	class repository : public TestWithParam<repo_param> {
		static git::init thread_;

	public:
		static void SetUpTestSuite() {
			using namespace std::filesystem;
			std::error_code ignore{};
			remove_all(TESTING_data_path, ignore);
			for (auto const subdir : setup::subdirs) {
				create_directories(path{append(TESTING_data_path, subdir)}, ignore);
			}
			for (auto const [filename, contents] : setup::files) {
				auto const p = path{append(TESTING_data_path, filename)};
				create_directories(p.parent_path(), ignore);
				std::ofstream out{p};
				out.write(contents.data(), contents.size());
			}
		}
		static void TearDownTestSuite() {
			using namespace std::filesystem;
			std::error_code ignore{};
			remove_all(TESTING_data_path, ignore);
		}
	};

	git::init repository::thread_{};

	TEST_P(repository, discover) {
		auto [start_path, expected, absolute] = GetParam();
		auto const start = make_absolute(start_path, absolute);
		auto const result = git::repository::discover(start.c_str());
		if (expected == std::string_view{}) {
			ASSERT_EQ(result, std::string{});
			return;
		}

		ASSERT_EQ(result, make_absolute(expected, absolute));
	}

	TEST_P(repository, open) {
		auto [start_path, expected, absolute] = GetParam();
		auto const start = make_absolute(start_path, absolute);
		auto const result = git::repository::discover(start.c_str());
		if (expected == std::string_view{}) {
			ASSERT_EQ(result, std::string{});
		} else {
			ASSERT_EQ(result, make_absolute(expected, absolute));
		}

		if (!result.empty()) {
			auto const repo = git::repository::open(result.c_str());
			ASSERT_TRUE(repo);
		}
	}

	TEST_P(repository, wrap) {
		auto [start_path, expected, absolute] = GetParam();
		auto const start = make_absolute(start_path, absolute);
		auto const result = git::repository::discover(start.c_str());
		if (expected == std::string_view{}) {
			ASSERT_EQ(result, std::string{});
		} else {
			ASSERT_EQ(result, make_absolute(expected, absolute));
		}

		if (!result.empty()) {
			auto const db = git::odb::open(result.c_str());
			auto const repo = git::repository::wrap(db);
			ASSERT_TRUE(db);
			ASSERT_TRUE(repo);
		}
	}

	constexpr repo_param dirs[] = {
	    {"/", {}, true},
	    {"c:/", {}, true},
	    {"bare.git/", "bare.git/"},
	    {"bare.git", "bare.git/"},
	    {"gitdir/subdir/", "gitdir/.git/"},
	    {"gitdir/", "gitdir/.git/"},
	    {"gitdir", "gitdir/.git/"},
	};

	INSTANTIATE_TEST_SUITE_P(dirs, repository, ValuesIn(dirs));
}  // namespace git::testing
