// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <date/tz.h>
#include <gtest/gtest.h>
#include <cov/branch.hh>
#include <cov/format.hh>
#include <cov/hash/md5.hh>
#include <cov/io/file.hh>
#include <cov/io/strings.hh>
#include <cov/repository.hh>
#include <cov/tag.hh>
#include <fstream>
#include "../setup.hh"
#include "new.hh"

namespace cov::testing {
	using namespace ::std::literals;
	namespace ph = placeholder;

	ref_ptr<cov::report> make_report(git_oid const& id,
	                                 std::string const& message,
	                                 sys_seconds commit,
	                                 sys_seconds add) {
		git_oid parent_id{}, commit_id{}, zero{};
		git_oid_fromstr(&parent_id, "8765432100ffeeddccbbaa998877665544332211");
		git_oid_fromstr(&commit_id, "36109a1c35e0d5cf3e5e68d896c8b1b4be565525");

		io::v1::coverage_stats const default_stats{1250, 300, 299};
		return report_create(id, parent_id, zero, commit_id, "develop"s,
		                     "Johnny Appleseed"s, "johnny@appleseed.com"s,
		                     "Johnny Committer"s, "committer@appleseed.com"s,
		                     message, commit, add, default_stats);
	}

	static constexpr auto message =
	    "Subject, isn't it?\n\nLorem ipsum dolor sit amet, consectetur "
	    "adipiscing elit. Praesent facilisis\nfeugiat nibh in sodales. "
	    "Nullam non velit lectus. Morbi finibus risus vel\nrutrum "
	    "faucibus. Fusce commodo ultrices blandit. Nulla congue accumsan "
	    "risus,\nut tempus lorem facilisis quis. Nunc ultricies metus sed "
	    "lacinia lacinia.\nAenean eu euismod purus, sed pulvinar libero. "
	    "Sed a nibh sed tortor venenatis\nfaucibus ac sit amet massa. Sed "
	    "in fringilla ante. Integer tristique vulputate\nnulla nec "
	    "tristique. Ut finibus leo ut lacinia molestie. Mauris blandit "
	    "tortor\naugue, nec fermentum augue varius in. Nam ut ultrices "
	    "enim. Nulla facilisi.\nPhasellus auctor et arcu vel molestie. "
	    "Proin accumsan rutrum risus, vel\nsollicitudin sapien lobortis "
	    "eget.\n\nUt at odio id nisi malesuada varius ut ac mauris. Orci "
	    "varius natoque penatibus\net magnis dis parturient montes, "
	    "nascetur ridiculus mus. Duis pretium pretium\nfringilla. Nulla "
	    "vitae urna lacinia, viverra ex eu, sollicitudin "
	    "lorem.\nVestibulum ut elit consectetur, placerat turpis "
	    "vulputate, malesuada dui. Nulla\nsagittis nisi ut luctus cursus. "
	    "Quisque sodales sapien quis tempor efficitur.\nPraesent sit amet "
	    "mi ac erat dictum ultrices. Ut arcu nibh, blandit vitae "
	    "nisi\nsed, sollicitudin bibendum ligula.\n"sv;

	TEST(oom, digest) {
		static constexpr auto msg =
		    "The quick brown fox jumps over the lazy dog"sv;
		auto const md5 = hash::md5::once({msg.data(), msg.size()});

		OOM_BEGIN(OOM_STR_THRESHOLD);
		md5.str();
		OOM_END;
	}

	TEST(oom, branch_valid_name) {
		OOM_BEGIN(OOM_STR_THRESHOLD);
		cov::branch::is_valid_name("domain/section"sv);
		OOM_END;
	}

	TEST(oom, tag_valid_name) {
		OOM_BEGIN(OOM_STR_THRESHOLD);
		cov::tag::is_valid_name("v1.2.3"sv);
		OOM_END;
	}

	TEST(oom, format) {
		ph::context ctx = {
		    .now =
		        std::chrono::floor<seconds>(std::chrono::system_clock::now()),
		    .hash_length = 9,
		    .names =
		        {
		            .HEAD = "main"s,
		            .tags = {{
		                         "v1.0.0"s,
		                         "221133445566778899aabbccddeeff0012345678"s,
		                     },
		                     {
		                         "v1.0.1"s,
		                         "112233445566778899aabbccddeeff0012345678"s,
		                     }},
		            .heads = {{
		                          "main"s,
		                          "221144335566778899aabbccddeeff0012345678"s,
		                      },
		                      {
		                          "feat/task-1"s,
		                          "112233445566778899aabbccddeeff0012345678"s,
		                      }},
		            .HEAD_ref = "112233445566778899aabbccddeeff0012345678"s,
		        },
		};

		git_oid id{};
		git_oid_fromstr(&id, "112233445566778899aabbccddeeff0012345678");
		auto const report =
		    make_report(id, {message.data(), message.size()}, ctx.now, ctx.now);

		auto view = ph::report_view::from(*report);
		auto fmt = formatter::from(
		    "%Hr%d %pC/%pR %pP (%pr) - from [%Hc] %s <%an %al %ae>%n%B"sv);

		// load the time zone database
		date::current_zone();

		OOM_BEGIN(1024);
		fmt.format(view, ctx);
		OOM_END;
	}

	TEST(oom, file_read) {
		auto const path = setup::test_dir() / "file_read/file"sv;
		remove_all(path.parent_path());
		create_directories(path.parent_path());
		{
			auto out = io::fopen(path, "wb");
			out.store(message.data(), message.size());
		}
		auto in = io::fopen(path, "rb");
		OOM_BEGIN(OOM_STR_THRESHOLD);
		in.read();
		OOM_END;
	}

	TEST(oom, file_read_line) {
		auto const path = setup::test_dir() / "file_read_line/file"sv;
		remove_all(path.parent_path());
		create_directories(path.parent_path());
		{
			auto out = io::fopen(path, "wb");
			out.store(message.data(), message.size());
		}
		auto in = io::fopen(path, "rb");
		OOM_BEGIN(OOM_STR_THRESHOLD);
		in.read_line();
		OOM_END;
	}

	TEST(oom, strings_reserve_offset) {
		io::strings_block block{};
		ASSERT_TRUE(block.reserve_offsets(12));

		bool reserved = false;
		{
			emulate_oom_as_threshold here{20 * sizeof(size_t)};
			reserved = block.reserve_offsets(21);
		}
		ASSERT_FALSE(reserved);
	}

	TEST(oom, strings_reserve_data) {
		io::strings_block block{};
		ASSERT_TRUE(block.reserve_data(120));

		bool reserved = false;
		{
			emulate_oom_as_threshold here{200};
			reserved = block.reserve_data(201);
		}
		ASSERT_FALSE(reserved);
	}

	TEST(oom, strings_builder_insert) {
		io::strings_builder bld{};
		bld.insert("short_1"sv)
		    .insert("short_2"sv)
		    .insert("short_8"sv)
		    .insert("short_9"sv)
		    .insert("short_10"sv)
		    .insert("short_11"sv)
		    .insert("short_12"sv)
		    .insert("short_13"sv)
		    .insert("short_14"sv)
		    .insert("short_15"sv)
		    .insert("short_16"sv)
		    .insert("short_17"sv)
		    .insert("short_18"sv)
		    .insert("short_19"sv)
		    .insert("short_20"sv);

		{
			emulate_oom_as_threshold here{OOM_STR_THRESHOLD};
			bld.insert("short_21"sv)
			    .insert("short_22"sv)
			    .insert("short_23"sv)
			    .insert("short_24"sv)
			    .insert("short_25"sv);
		}

		auto const result = bld.build();
		ASSERT_NE(result.size(), result.locate_or("short_18"sv, result.size()));
		ASSERT_EQ(result.size(), result.locate_or("short_22"sv, result.size()));
	}

#ifdef _WIN32
	void setpath(char const* name, std::filesystem::path const& value) {
		std::wstring var = std::filesystem::path{name}.native();
		std::filesystem::path path{};
		var.push_back(L'=');
		if (!value.empty()) {
			path = value;
			path.make_preferred();
			var.append(path.native());
		}
		_wputenv(var.c_str());
	}

	inline void unsetenv(char const* name) {
		std::string var = name;
		var.push_back(L'=');
		_putenv(var.c_str());
	}
#else
	void setpath(char const* name, std::filesystem::path const& value) {
		setenv(name, value.c_str(), 1);
	}
#endif

	TEST(oom, open_config) {
		auto const home = setup::test_dir() / "home"sv;
		auto const repo_dir = setup::test_dir() / "repo"sv;

		unsetenv("USERPROFILE");
		unsetenv("ProgramData");
		unsetenv("XDG_CONFIG_HOME");
		setpath("HOME", home.c_str());

		std::error_code ignore{};
		{
			static constexpr auto from_home_xdg_config =
			    "[test]\nvalue=from home/.config config"sv;
			create_directories(home / ".config/dot"sv);
			std::ofstream out{home / ".config/dot/config"sv};
			out.write(
			    from_home_xdg_config.data(),
			    static_cast<std::streamsize>(from_home_xdg_config.size()));
			remove_all(repo_dir);
			create_directories(repo_dir);
			init_repository(repo_dir, {}, ignore);
		}

		OOM_LIMIT(400)
		auto const cfg = cov::repository::open(repo_dir, ignore);
		OOM_END
	}
}  // namespace cov::testing
