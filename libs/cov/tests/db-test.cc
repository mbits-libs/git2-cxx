// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/db.hh>
#include <cov/hash/sha1.hh>
#include <cov/io/types.hh>
#include <cov/report.hh>
#include <filesystem>
#include <map>
#include "db-helper.hh"
#include "path-utils.hh"

namespace cov::testing {
	using namespace std::literals;

	TEST(db, full_report) {
		git_oid report_id{};

		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("full_report"sv),
			                         create_directories("full_report"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto backend =
		    loose_backend_create(setup::test_dir() / "full_report"sv);
		ASSERT_TRUE(backend);

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
		               .lines = {{10, 1},
		                         {11, 1},
		                         {12, 1},
		                         {15, 0},
		                         {16, 0},
		                         {18, 0},
		                         {19, 0}}},
		              {.name = "module.cpp"sv,
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
			    git_oid{}, rprt.parent, files_id, rprt.head.commit,
			    rprt.head.branch, rprt.head.author_name, rprt.head.author_email,
			    rprt.head.committer_name, rprt.head.committer_email,
			    rprt.head.message, rprt.head.commit_time_utc, rprt.add_time_utc,
			    total);
			ASSERT_TRUE(cvg_report);
			ASSERT_TRUE(backend->write(report_id, cvg_report));
		}
		ASSERT_EQ("6ae885c09be27020a24aacbd9dd96d11c8305692"sv,
		          setup::get_oid(report_id));

		// read
		{
			auto cvg_report = backend->lookup<cov::report>(report_id);
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
			    backend->lookup<cov::report_files>(cvg_report->file_list());
			ASSERT_TRUE(cvg_files);
			auto& entries = cvg_files->entries();
			ASSERT_EQ(rprt.files.size(), entries.size());

			auto const count = entries.size();
			for (auto index = 0u; index < count; ++index) {
				auto& entry = entries[index];
				auto& file = rprt.files[index];
				ASSERT_EQ(file.name, entry->path());

				auto cvg_lines =
				    backend->lookup<cov::line_coverage>(entry->line_coverage());
				ASSERT_TRUE(cvg_files);
				unsigned finish{0};
				auto const cvg = from_coverage(cvg_lines->coverage(), finish);
				ASSERT_EQ(file.finish, finish);
				ASSERT_EQ(file.lines, cvg);
			}
		}
	}

	TEST(db, read_nonexisting) {
		git_oid report_id{};

		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("read_nonexisting"sv),
			                         create_directories("read_nonexisting"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';

			ASSERT_EQ(
			    0, git_oid_fromstr(&report_id,
			                       "6ae885c09be27020a24aacbd9dd96d11c8305692"));
		}

		auto backend =
		    loose_backend_create(setup::test_dir() / "read_nonexisting"sv);
		ASSERT_TRUE(backend);

		auto cvg_report = backend->lookup<cov::report>(report_id);
		ASSERT_FALSE(cvg_report);
	}

	struct none : counted_impl<object> {
		obj_type type() const noexcept override {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
// warning: the result of the conversion is unspecified because
// ‘std::numeric_limits<unsigned int>::max()’ is outside the range of type
// ‘cov::obj_type’
//
// Which is kinda the point...
#endif
			return static_cast<obj_type>(
			    std::numeric_limits<std::underlying_type_t<obj_type>>::max());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
		};
	};

}  // namespace cov::testing

namespace cov {
	template <>
	inline bool object::is_a<cov::testing::none>() const noexcept {
		return false;
	}
}  // namespace cov

namespace cov::testing {
	TEST(db, read_unknown) {
		git_oid report_id{};

		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("read_unknown"sv),
			                         create_directories("read_unknown"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto backend =
		    loose_backend_create(setup::test_dir() / "read_unknown"sv);
		ASSERT_TRUE(backend);

		{
			auto cvg_report = report_create({}, {}, {}, {}, {}, {}, {}, {}, {},
			                                {}, {}, {}, {});
			ASSERT_TRUE(cvg_report);
			ASSERT_TRUE(backend->write(report_id, cvg_report));
		}

		auto cvg_report = backend->lookup<none>(report_id);
		ASSERT_FALSE(cvg_report);
	}

	TEST(db, write_unknown) {
		git_oid report_id{};

		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("write_unknown"sv),
			                         create_directories("write_unknown"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';
		}

		auto backend =
		    loose_backend_create(setup::test_dir() / "write_unknown"sv);
		ASSERT_TRUE(backend);

		auto none_ref = make_ref<none>();
		ASSERT_TRUE(none_ref);
		ASSERT_FALSE(backend->write(report_id, none_ref));
	}

	TEST(db, failed_z_stream) {
		git_oid oid{};

		static constexpr auto text = "not a z-stream"sv;

		auto sha = hash::sha1::once({text.data(), text.size()}).str();
		git_oid_fromstr(&oid, sha.c_str());
		sha.insert(2, 1, '/');

		{
			std::error_code ec{};
			path_info::op(make_setup(remove_all("failed_z_stream"sv),
			                         create_directories("failed_z_stream"sv)),
			              ec);
			ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
			                 << ec.category().name() << ')';

			auto const path = setup::test_dir() / ("failed_z_stream/" + sha);
			create_directories(path.parent_path());
			auto f = io::fopen(path, "w");
			f.store(text.data(), text.size());
		}

		auto backend =
		    loose_backend_create(setup::test_dir() / "failed_z_stream"sv);
		ASSERT_TRUE(backend);

		auto obj = backend->lookup<none>(oid);
		ASSERT_FALSE(obj);
	}
}  // namespace cov::testing
