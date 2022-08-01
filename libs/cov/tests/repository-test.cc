// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/branch.hh>
#include <cov/io/file.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
#include <git2/blob.hh>
#include <git2/config.hh>
#include <git2/global.hh>
#include "db-helper.hh"
#include "path-utils.hh"
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	// using namespace std::filesystem;

	class repository : public ::testing::Test {
	public:
		void run_setup(std::vector<path_info> const& steps) {
			std::error_code ec{};
			path_info::op(steps, ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}
	};

	TEST_F(repository, bad_discovery) {
		git::init init{};

		run_setup(
		    make_setup(remove_all("repository"sv),
		               create_directories("repository/discover/subdir"sv)));

		std::error_code ec{};
		auto const covdir = cov::repository::discover(
		    setup::test_dir() / "repository/discover/subdir"sv, ec);
		ASSERT_TRUE(ec);
		ASSERT_TRUE(covdir.empty());
	}

	TEST_F(repository, good_discovery) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_repo("repository/actual"sv),
		    touch("repository/discover/.covdata"sv,
		          "covdata: \t ../actual \t \n"sv),
		    create_directories("repository/discover/subdir"sv)));

		std::error_code ec{};
		auto const covdir = cov::repository::discover(
		    setup::test_dir() / "repository/discover/subdir"sv, ec);
		ASSERT_FALSE(ec);
		ASSERT_EQ(setup::get_path(
		              canonical(setup::test_dir() / "repository/actual"sv)),
		          setup::get_path(covdir));
	}

	TEST_F(repository, good_init) {
		git::init init{};

		run_setup(make_setup(remove_all("inside_git"sv),
		                     init_git_workspace("inside_git"sv)));

		std::error_code ec{};
		auto const repo = cov::repository::init(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "inside_git/.git/.covdata"sv,
		    setup::test_dir() / "inside_git/.git"sv, ec);
		ASSERT_FALSE(ec);

		auto const& commondir = repo.commondir();
		std::vector<entry_t> entries;
		for (auto const& entry : recursive_directory_iterator{commondir}) {
			entries.push_back(
			    {setup::get_path(proximate(entry.path(), commondir)),
			     entry.status().type()});
		}
		std::sort(entries.begin(), entries.end());

		std::vector<entry_t> const expected_tree{
		    {"HEAD"s, file_type::regular},
		    {"config"s, file_type::regular},
		    {"objects"s, file_type::directory},
		    {"objects/coverage"s, file_type::directory},
		    {"objects/pack"s, file_type::directory},
		    {"refs"s, file_type::directory},
		    {"refs/heads"s, file_type::directory},
		    {"refs/tags"s, file_type::directory},
		};

		ASSERT_EQ(expected_tree, entries);

		static constexpr auto gitdir = ".."sv;
		auto dir = repo.config().get_string("core.gitdir");
		ASSERT_TRUE(dir);
		ASSERT_EQ(gitdir, *dir);
	}

	TEST_F(repository, init_existing) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("inside_git"sv), init_git_workspace("inside_git"sv),
		    init_repo("inside_git/.git/.covdata"sv, "inside_git/.git"sv)));

		std::error_code ec{};
		auto const repo = cov::repository::init(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "inside_git/.git/.covdata"sv,
		    setup::test_dir() / "inside_git/.git"sv, ec);
		ASSERT_TRUE(ec);
	}

	TEST_F(repository, open_no_git) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("inside_git"sv), init_git_workspace("inside_git"sv),
		    init_repo("inside_git/.git/.covdata"sv, "inside_git/.git"sv)));
		{
			auto f = io::fopen(
			    setup::test_dir() / "inside_git/.git/.covdata/config"sv, "w");
		}

		std::error_code ec{};
		auto const repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "inside_git/.git/.covdata"sv, ec);
		ASSERT_TRUE(ec);
	}

	TEST_F(repository, git_missing) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), create_directories("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		std::error_code ec{};
		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv, ec);
		ASSERT_FALSE(ec);
		ASSERT_EQ(setup::get_path(
		              canonical(setup::test_dir() / "repository/.covdata"sv)),
		          setup::get_path(covdir));

		auto const repo =
		    cov::repository::open(setup::test_dir() / "sysroot"sv, covdir, ec);
		ASSERT_TRUE(ec);
	}

	TEST_F(repository, no_reports) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		std::error_code ec{};
		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv, ec);
		ASSERT_FALSE(ec);
		ASSERT_EQ(setup::get_path(
		              canonical(setup::test_dir() / "repository/.covdata"sv)),
		          setup::get_path(covdir));

		auto const repo =
		    cov::repository::open(setup::test_dir() / "sysroot"sv, covdir, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo.refs());

		auto const HEAD = repo.refs()->lookup("HEAD"sv);
		ASSERT_TRUE(HEAD);
		ASSERT_EQ(reference_type::symbolic, HEAD->reference_type());

		auto const empty_repo = !HEAD->peel_target();
		ASSERT_TRUE(empty_repo);
	}

	TEST_F(repository, detached) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv),
		    touch("repository/.covdata/HEAD"sv,
		          "f8e21b2735744fa148e8c6e0bb59d7629857d2a7\n"sv)));

		std::error_code ec{};
		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv, ec);
		ASSERT_FALSE(ec);
		ASSERT_EQ(setup::get_path(
		              canonical(setup::test_dir() / "repository/.covdata"sv)),
		          setup::get_path(covdir));

		auto const repo =
		    cov::repository::open(setup::test_dir() / "sysroot"sv, covdir, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo.refs());

		auto const HEAD = repo.refs()->lookup("HEAD"sv);
		ASSERT_TRUE(HEAD);
		ASSERT_EQ(reference_type::direct, HEAD->reference_type());
	}

	TEST_F(repository, dwim) {
		git::init init{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv),
		    touch("repository/.covdata/refs/heads/main"sv,
		          "f8e21b2735744fa148e8c6e0bb59d7629857d2a7\n"sv),
		    touch("repository/.covdata/refs/tags/v1.0.0"sv,
		          "cb62947f3ebcaf2869b0a7c1c76673db3641b72a\n"sv)));

		std::error_code ec{};
		auto const covdir =
		    cov::repository::discover(setup::test_dir() / "repository"sv, ec);
		ASSERT_FALSE(ec);
		ASSERT_EQ(setup::get_path(
		              canonical(setup::test_dir() / "repository/.covdata"sv)),
		          setup::get_path(covdir));

		auto const repo =
		    cov::repository::open(setup::test_dir() / "sysroot"sv, covdir, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(repo.refs());

		auto const HEAD = as_a<cov::reference>(repo.dwim("HEAD"sv));
		ASSERT_TRUE(HEAD);
		auto const peeled = HEAD->peel_target();
		ASSERT_TRUE(peeled);
		ASSERT_EQ("main"sv, peeled->shorthand());
		ASSERT_TRUE(peeled->references_branch());

		auto const tag = as_a<cov::tag>(repo.dwim("tags/v1.0.0"sv));
		ASSERT_TRUE(tag);
		ASSERT_EQ("v1.0.0"sv, tag->name());

		auto const branch = as_a<cov::branch>(repo.dwim("heads/main"sv));
		ASSERT_TRUE(branch);
		ASSERT_EQ("main"sv, branch->name());
	}

	TEST_F(repository, lookup_report) {
		git::init init{};
		git_oid report_id{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		report rprt = {
		    .add_time_utc = sys_seconds{0x11223344556677s},
		    .head = {.branch = "develop"s,
		             .author_name = "Johnny Appleseed"s,
		             .author_email = "johnny@appleseed.com"s,
		             .committer_name = "Johnny Appleseed"s,
		             .committer_email = "johnny@appleseed.com"s,
		             .message = "Initial commit"s,
		             .commit_time_utc = sys_seconds{0x11223344556677s}},
		    .files = {{.name = "main.cpp"sv,
		               .dirty = false,
		               .modified = false,
		               .lines = {{10, 1},
		                         {11, 1},
		                         {12, 1},
		                         {15, 0},
		                         {16, 0},
		                         {18, 0},
		                         {19, 0}}},
		              {.name = "module.cpp"sv,
		               .dirty = false,
		               .modified = false,
		               .lines = {{10, 15},
		                         {11, 15},
		                         {12, 10},
		                         {13, 5},
		                         {14, 15},
		                         {100, 15},
		                         {101, 15},
		                         {102, 10},
		                         {103, 5},
		                         {104, 15}},
		               .finish = 20}}};

		// write
		{
			auto backend = loose_backend_create(
			    setup::test_dir() / "repository/.covdata/objects/coverage"sv);
			ASSERT_TRUE(backend);

			auto total = io::v1::coverage_stats::init();
			report_files_builder builder{};
			for (auto const& file : rprt.files) {
				auto cvg_object = from_lines(file.lines, file.finish);
				ASSERT_TRUE(cvg_object);
				git_oid line_cvg_id{};
				ASSERT_TRUE(backend->write(line_cvg_id, cvg_object));

				auto file_stats = stats(cvg_object->coverage());
				total += file_stats;

				file.add_to(builder, file_stats, line_cvg_id);
			}

			auto cvg_files = builder.extract();
			ASSERT_TRUE(cvg_files);
			git_oid files_id{};
			ASSERT_TRUE(backend->write(files_id, cvg_files));

			auto cvg_report = report_create(
			    {}, rprt.parent, files_id, rprt.head.commit, rprt.head.branch,
			    rprt.head.author_name, rprt.head.author_email,
			    rprt.head.committer_name, rprt.head.committer_email,
			    rprt.head.message, rprt.head.commit_time_utc, rprt.add_time_utc,
			    total);
			ASSERT_TRUE(cvg_report);
			ASSERT_TRUE(backend->write(report_id, cvg_report));
		}
		ASSERT_EQ("6ae885c09be27020a24aacbd9dd96d11c8305692"sv,
		          setup::get_oid(report_id));

		// read
		std::error_code ec{};
		auto repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "repository/.covdata"sv, ec);
		ASSERT_FALSE(ec);

		auto cvg_report = repo.lookup<cov::report>(report_id, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(cvg_report);

		ASSERT_EQ(rprt.head.branch, cvg_report->branch());
		ASSERT_EQ(rprt.head.author_name, cvg_report->author_name());
		ASSERT_EQ(rprt.head.author_email, cvg_report->author_email());
		ASSERT_EQ(rprt.head.committer_name, cvg_report->committer_name());
		ASSERT_EQ(rprt.head.committer_email, cvg_report->committer_email());
		ASSERT_EQ(rprt.head.message, cvg_report->message());
		ASSERT_EQ(rprt.head.commit_time_utc, cvg_report->commit_time_utc());
		ASSERT_EQ(rprt.add_time_utc, cvg_report->add_time_utc());

		auto cvg_files =
		    repo.lookup<cov::report_files>(cvg_report->file_list(), ec);
		ASSERT_TRUE(cvg_files);
		auto& entries = cvg_files->entries();
		ASSERT_EQ(rprt.files.size(), entries.size());

		auto const count = entries.size();
		for (auto index = 0u; index < count; ++index) {
			auto& entry = entries[index];
			auto& file = rprt.files[index];
			ASSERT_EQ(file.name, entry->path());
			ASSERT_EQ(file.dirty, entry->is_dirty());
			ASSERT_EQ(file.modified, entry->is_modified());

			auto cvg_lines =
			    repo.lookup<cov::line_coverage>(entry->line_coverage(), ec);
			ASSERT_TRUE(cvg_files);
			unsigned finish{0};
			auto const cvg = from_coverage(cvg_lines->coverage(), finish);
			ASSERT_EQ(file.finish, finish);
			ASSERT_EQ(file.lines, cvg);
		}
	}

	TEST_F(repository, write_report) {
		git::init init{};
		git_oid report_id{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		report rprt = {
		    .add_time_utc = sys_seconds{0x11223344556677s},
		    .head = {.branch = "develop"s,
		             .author_name = "Johnny Appleseed"s,
		             .author_email = "johnny@appleseed.com"s,
		             .committer_name = "Johnny Appleseed"s,
		             .committer_email = "johnny@appleseed.com"s,
		             .message = "Initial commit"s,
		             .commit_time_utc = sys_seconds{0x11223344556677s}},
		    .files = {{.name = "main.cpp"sv,
		               .dirty = false,
		               .modified = false,
		               .lines = {{10, 1},
		                         {11, 1},
		                         {12, 1},
		                         {15, 0},
		                         {16, 0},
		                         {18, 0},
		                         {19, 0}}},
		              {.name = "module.cpp"sv,
		               .dirty = false,
		               .modified = false,
		               .lines = {{10, 15},
		                         {11, 15},
		                         {12, 10},
		                         {13, 5},
		                         {14, 15},
		                         {100, 15},
		                         {101, 15},
		                         {102, 10},
		                         {103, 5},
		                         {104, 15}},
		               .finish = 20}}};

		std::error_code ec{};
		auto repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "repository/.covdata"sv, ec);
		ASSERT_FALSE(ec);

		// write
		{
			auto total = io::v1::coverage_stats::init();
			report_files_builder builder{};
			for (auto const& file : rprt.files) {
				auto cvg_object = from_lines(file.lines, file.finish);
				ASSERT_TRUE(cvg_object);
				git_oid line_cvg_id{};
				ASSERT_TRUE(repo.write(line_cvg_id, cvg_object));

				auto file_stats = stats(cvg_object->coverage());
				total += file_stats;

				file.add_to(builder, file_stats, line_cvg_id);
			}

			auto cvg_files = builder.extract();
			ASSERT_TRUE(cvg_files);
			git_oid files_id{};
			ASSERT_TRUE(repo.write(files_id, cvg_files));

			auto cvg_report = report_create(
			    {}, rprt.parent, files_id, rprt.head.commit, rprt.head.branch,
			    rprt.head.author_name, rprt.head.author_email,
			    rprt.head.committer_name, rprt.head.committer_email,
			    rprt.head.message, rprt.head.commit_time_utc, rprt.add_time_utc,
			    total);
			ASSERT_TRUE(cvg_report);
			ASSERT_TRUE(repo.write(report_id, cvg_report));
		}
		ASSERT_EQ("6ae885c09be27020a24aacbd9dd96d11c8305692"sv,
		          setup::get_oid(report_id));

		// read
		auto cvg_report = repo.lookup<cov::report>(report_id, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(cvg_report);

		ASSERT_EQ(rprt.head.branch, cvg_report->branch());
		ASSERT_EQ(rprt.head.author_name, cvg_report->author_name());
		ASSERT_EQ(rprt.head.author_email, cvg_report->author_email());
		ASSERT_EQ(rprt.head.committer_name, cvg_report->committer_name());
		ASSERT_EQ(rprt.head.committer_email, cvg_report->committer_email());
		ASSERT_EQ(rprt.head.message, cvg_report->message());
		ASSERT_EQ(rprt.head.commit_time_utc, cvg_report->commit_time_utc());
		ASSERT_EQ(rprt.add_time_utc, cvg_report->add_time_utc());

		auto cvg_files =
		    repo.lookup<cov::report_files>(cvg_report->file_list(), ec);
		ASSERT_TRUE(cvg_files);
		auto& entries = cvg_files->entries();
		ASSERT_EQ(rprt.files.size(), entries.size());

		auto const count = entries.size();
		for (auto index = 0u; index < count; ++index) {
			auto& entry = entries[index];
			auto& file = rprt.files[index];
			ASSERT_EQ(file.name, entry->path());
			ASSERT_EQ(file.dirty, entry->is_dirty());
			ASSERT_EQ(file.modified, entry->is_modified());

			auto cvg_lines =
			    repo.lookup<cov::line_coverage>(entry->line_coverage(), ec);
			ASSERT_TRUE(cvg_files);
			unsigned finish{0};
			auto const cvg = from_coverage(cvg_lines->coverage(), finish);
			ASSERT_EQ(file.finish, finish);
			ASSERT_EQ(file.lines, cvg);
		}
	}

	TEST_F(repository, blob) {
		git::init init{};
		git_oid file_id{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		std::error_code ec{};
		auto repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "repository/.covdata"sv, ec);
		ASSERT_FALSE(ec);

		static constexpr auto text = R"(This is text of the file.
It has two lines.
)"sv;
		ASSERT_TRUE(repo.write(file_id, {text.data(), text.size()}));
		ASSERT_EQ("750229e9588ba0f8a2d4c15d64bb089fe135734d"sv,
		          setup::get_oid(file_id));

		auto blob_obj = repo.lookup<cov::object>(file_id, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(blob_obj);
		ASSERT_TRUE(is_a<cov::blob>(blob_obj));
		ASSERT_EQ(obj_blob, blob_obj->type());
		auto blob = as_a<cov::blob>(blob_obj);
		ASSERT_TRUE(blob);
		ASSERT_EQ(cov::origin::cov, blob->origin());
		ASSERT_TRUE(blob->peek());

		auto data = blob->peek().raw();
		auto view = std::string_view{reinterpret_cast<char const*>(data.data()),
		                             data.size()};
		ASSERT_EQ(text, view);

		git_oid new_one{};
		ASSERT_FALSE(repo.write(new_one, blob));

		auto unlinked = blob->peel_and_unlink();
		ASSERT_TRUE(unlinked);
		ASSERT_FALSE(blob->peek());
	}

	TEST_F(repository, blob_in_git) {
		git::init init{};
		git_oid file_id{};

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		std::error_code ec{};
		static constexpr auto text = R"(This is text of the file.
It has two lines.
)"sv;

		{
			auto const odb = git::odb::open(
			    setup::test_dir() / "repository/.git/objects"sv, ec);
			ASSERT_FALSE(ec);
			ASSERT_TRUE(odb);
			ec = odb.write(&file_id, {text.data(), text.size()},
			               GIT_OBJECT_BLOB);
			ASSERT_EQ("750229e9588ba0f8a2d4c15d64bb089fe135734d"sv,
			          setup::get_oid(file_id));
		}

		auto repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "repository/.covdata"sv, ec);
		ASSERT_FALSE(ec);

		auto blob = repo.lookup<cov::blob>(file_id, ec);
		ASSERT_FALSE(ec);
		ASSERT_TRUE(blob);
		ASSERT_EQ(cov::origin::git, blob->origin());
		ASSERT_TRUE(blob->peek());

		auto data = blob->peek().raw();
		auto view = std::string_view{reinterpret_cast<char const*>(data.data()),
		                             data.size()};
		ASSERT_EQ(text, view);
	}

	TEST_F(repository, nonexistent_blob) {
		git::init init{};
		git_oid file_id{};
		git_oid_fromstr(&file_id, "750229e9588ba0f8a2d4c15d64bb089fe135734d");

		run_setup(make_setup(
		    remove_all("repository"sv), init_git_workspace("repository"sv),
		    init_repo("repository/.covdata"sv, "repository/.git"sv)));

		std::error_code ec{};
		auto repo = cov::repository::open(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "repository/.covdata"sv, ec);
		ASSERT_FALSE(ec);

		auto blob = repo.lookup<cov::blob>(file_id, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(blob);
	}
}  // namespace cov::testing
