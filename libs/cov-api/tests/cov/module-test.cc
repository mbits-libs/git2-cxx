// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/errors.h>
#include <git2/signature.h>
#include <gtest/gtest.h>
#include <cov/git2/blob.hh>
#include <cov/git2/commit.hh>
#include <cov/git2/error.hh>
#include <cov/git2/global.hh>
#include <cov/git2/ptr.hh>
#include <cov/git2/tree.hh>
#include <cov/module.hh>
#include <cov/repository.hh>
#include <fstream>
#include "print-view.hh"
#include "setup.hh"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace git {
	GIT_PTR_FREE(git_signature);

	using signature = ptr<git_signature>;
	signature signature_now(char const* name,
	                        char const* email,
	                        std::error_code& ec) {
		return create_handle<signature>(ec, git_signature_now, name, email);
	}
}  // namespace git

namespace cov {
	void PrintTo(module_info const& mod, std::ostream* out) {
		testing::print_view(*out << '{', mod.name) << "s, {";
		auto first = true;
		for (auto const& path : mod.prefixes) {
			if (first)
				first = false;
			else
				*out << ", ";
			testing::print_view(*out, path) << 's';
		}
		*out << "}}";
	}
}  // namespace cov

namespace cov::testing {
	using namespace std::literals;
	using namespace std::filesystem;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

#ifdef _WIN32
	struct cp_utf8 {
		cp_utf8() { SetConsoleOutputCP(CP_UTF8); }
	} init__;
#endif

	static constexpr auto this_project = R"x([module]
    sep = " » "
[module "git2 (c++)"]
    path = libs/git2-c++
[module "cov » API"]
    path = libs/cov
[module "cov » runtime"]
    path = libs/cov-rt
[module "app » library"]
    path = libs/app
[module "app » main"]
    path = apps/cov/
    path = apps/builtins/
[module "hilite"]
    path = libs/hilite/hilite
    path = libs/hilite/lighter
[module "hilite » c++"]
    path = libs/hilite/hilite-cxx
[module "hilite » python"]
    path = libs/hilite/hilite-py3
[module "hilite » type script"]
    path = libs/hilite/hilite-ts
)x"sv;

	static std::vector<module_info> const project_modules{
	    {"git2 (c++)"s, {"libs/git2-c++"s}},
	    {"cov \xc2\xbb API"s, {"libs/cov"s}},
	    {"cov \xc2\xbb runtime"s, {"libs/cov-rt"s}},
	    {"app \xc2\xbb library"s, {"libs/app"s}},
	    {"app \xc2\xbb main"s, {"apps/cov/"s, "apps/builtins/"s}},
	    {"hilite"s, {"libs/hilite/hilite"s, "libs/hilite/lighter"s}},
	    {"hilite \xc2\xbb c++"s, {"libs/hilite/hilite-cxx"s}},
	    {"hilite \xc2\xbb python"s, {"libs/hilite/hilite-py3"s}},
	    {"hilite \xc2\xbb type script"s, {"libs/hilite/hilite-ts"s}},
	};

	static constexpr auto modified_project = R"x([module]
    sep = " :: "
[module "git2 (c++)"]
    path = libs/git2-c++
[module "cov :: API"]
    path = libs/cov
)x"sv;

	static std::vector<module_info> const modified_project_modules{
	    {"git2 (c++)"s, {"libs/git2-c++"s}},
	    {"cov :: API"s, {"libs/cov"s}},
	};

	static std::vector<std::string_view> const all_files{
	    "apps/builtins/builtin_init.cc"sv,
	    "apps/builtins/builtin_config.cc"sv,
	    "apps/builtins/builtin_module.cc"sv,
	    "apps/cov/cov.cc"sv,
	    "libs/app/include/cov/app/args.hh"sv,
	    "libs/app/include/cov/app/tr.hh"sv,
	    "libs/app/src/main.cc"sv,
	    "libs/app/src/args.cc"sv,
	    "libs/app/src/win32.cc"sv,
	    "libs/app/src/tr.cc"sv,
	    "libs/app/src/strings/cov.cc"sv,
	    "libs/app/src/strings/args.cc"sv,
	    "libs/app/src/strings/cov_init.cc"sv,
	    "libs/app/src/strings/cov_config.cc"sv,
	    "libs/app/src/strings/errors.cc"sv,
	    "libs/app/src/strings/cov_module.cc"sv,
	    "libs/cov-rt/include/cov/app/tools.hh"sv,
	    "libs/cov-rt/include/cov/app/config.hh"sv,
	    "libs/cov-rt/src/root_command.cc"sv,
	    "libs/cov-rt/src/config.cc"sv,
	    "libs/cov-rt/src/tools.cc"sv,
	    "libs/cov-rt/src/split_command.cc"sv,
	    "libs/cov/include/cov/branch.hh"sv,
	    "libs/cov-rt/src/win32.cc"sv,
	    "libs/cov/include/cov/db.hh"sv,
	    "libs/cov/include/cov/counted.hh"sv,
	    "libs/cov/include/cov/format.hh"sv,
	    "libs/cov/include/cov/discover.hh"sv,
	    "libs/cov/include/cov/object.hh"sv,
	    "libs/cov/include/cov/init.hh"sv,
	    "libs/cov/include/cov/reference.hh"sv,
	    "libs/cov/include/cov/random.hh"sv,
	    "libs/cov/include/cov/report.hh"sv,
	    "libs/cov/include/cov/repository.hh"sv,
	    "libs/cov/include/cov/streams.hh"sv,
	    "libs/cov/include/cov/tag.hh"sv,
	    "libs/cov/include/cov/zstream.hh"sv,
	    "libs/cov/include/cov/hash/hash.hh"sv,
	    "libs/cov/include/cov/hash/md5.hh"sv,
	    "libs/cov/include/cov/hash/sha1.hh"sv,
	    "libs/cov/include/cov/io/db_object.hh"sv,
	    "libs/cov/include/cov/io/file.hh"sv,
	    "libs/cov/include/cov/io/read_stream.hh"sv,
	    "libs/cov/include/cov/io/safe_stream.hh"sv,
	    "libs/cov/include/cov/io/strings.hh"sv,
	    "libs/cov/include/cov/io/types.hh"sv,
	    "libs/cov/src/branch-tag.hh"sv,
	    "libs/cov/src/branch.cc"sv,
	    "libs/cov/src/counted.cc"sv,
	    "libs/cov/src/db.cc"sv,
	    "libs/cov/src/discover.cc"sv,
	    "libs/cov/src/format.cc"sv,
	    "libs/cov/src/init.cc"sv,
	    "libs/cov/src/path-utils.hh"sv,
	    "libs/cov/src/repository.cc"sv,
	    "libs/cov/src/streams.cc"sv,
	    "libs/cov/src/tag.cc"sv,
	    "libs/cov/src/zstream.cc"sv,
	    "libs/cov/src/hash/md5.cc"sv,
	    "libs/cov/src/hash/sha1.cc"sv,
	    "libs/cov/src/io/db_object-error.cc"sv,
	    "libs/cov/src/io/db_object.cc"sv,
	    "libs/cov/src/io/file.cc"sv,
	    "libs/cov/src/io/line_coverage.cc"sv,
	    "libs/cov/src/io/read_stream.cc"sv,
	    "libs/cov/src/io/report.cc"sv,
	    "libs/cov/src/io/files.cc"sv,
	    "libs/cov/src/io/safe_stream.cc"sv,
	    "libs/cov/src/io/strings.cc"sv,
	    "libs/cov/src/ref/reference.cc"sv,
	    "libs/cov/src/ref/reference_list.cc"sv,
	    "libs/cov/src/ref/references.cc"sv,
	    "libs/git2-c++/include/git2/blob.hh"sv,
	    "libs/git2-c++/include/git2/bytes.hh"sv,
	    "libs/git2-c++/include/git2/commit.hh"sv,
	    "libs/git2-c++/include/git2/config.hh"sv,
	    "libs/git2-c++/include/git2/diff.hh"sv,
	    "libs/git2-c++/include/git2/error.hh"sv,
	    "libs/git2-c++/include/git2/global.hh"sv,
	    "libs/git2-c++/include/git2/object.hh"sv,
	    "libs/git2-c++/include/git2/odb.hh"sv,
	    "libs/git2-c++/include/git2/ptr.hh"sv,
	    "libs/git2-c++/include/git2/repository.hh"sv,
	    "libs/git2-c++/include/git2/tree.hh"sv,
	    "libs/git2-c++/src/blob.cc"sv,
	    "libs/git2-c++/src/commit.cc"sv,
	    "libs/git2-c++/src/config.cc"sv,
	    "libs/git2-c++/src/error.cc"sv,
	    "libs/git2-c++/src/odb.cc"sv,
	    "libs/git2-c++/src/repository.cc"sv,
	    "libs/git2-c++/src/tree.cc"sv,
	    "libs/hilite/hilite-cxx/src/cxx.cc"sv,
	    "libs/hilite/hilite/include/cell/ascii.hh"sv,
	    "libs/hilite/hilite/include/cell/character.hh"sv,
	    "libs/hilite/hilite/include/cell/context.hh"sv,
	    "libs/hilite/hilite/include/cell/operators.hh"sv,
	    "libs/hilite/hilite/include/cell/parser.hh"sv,
	    "libs/hilite/hilite/include/cell/repeat_operators.hh"sv,
	    "libs/hilite/hilite/include/cell/special.hh"sv,
	    "libs/hilite/hilite/include/cell/string.hh"sv,
	    "libs/hilite/hilite/include/cell/tokens.hh"sv,
	    "libs/hilite/hilite/include/hilite/hilite.hh"sv,
	    "libs/hilite/hilite/include/hilite/hilite_impl.hh"sv,
	    "libs/hilite/hilite/src/hilite.cc"sv,
	    "libs/hilite/hilite/src/none.cc"sv,
	    "libs/hilite/lighter/src/lighter.cc"sv,
	    "libs/hilite/lighter/src/hilite/lighter.hh"sv};

	std::error_code commit_dot_modules(git::oid& commit_id,
	                                   git::oid const* parent_id,
	                                   git::repository_handle const& repo,
	                                   std::string_view file_text,
	                                   const char* message) {
		std::error_code ec;

		git_oid modules_id{};
		ec = git::as_error(git_blob_create_from_buffer(
		    &modules_id, repo.get(), file_text.data(), file_text.size()));
		if (ec) return ec;

		auto builder = git::treebuilder::create(ec, repo);
		if (ec) return ec;

		ec = builder.insert(".covmodule", modules_id, GIT_FILEMODE_BLOB);
		if (ec) return ec;

		git::oid tree_id{};
		ec = builder.write(tree_id);
		if (ec) return ec;

		auto tree = repo.lookup<git::tree>(tree_id, ec);
		if (ec) return ec;

		auto johny_appleseed =
		    git::signature_now("Johnny Appleseed", "johnny@appleseed.com", ec);

		git::commit prev_commit{};
		if (parent_id) {
			prev_commit = repo.lookup<git::commit>(*parent_id, ec);
			if (ec) return ec;
		}
		git_commit const* parents[] = {prev_commit.raw()};

		return git::as_error(git_commit_create(
		    &commit_id.id, repo.get(), "HEAD", johny_appleseed.raw(),
		    johny_appleseed.raw(), "UTF-8", message, tree.raw(),
		    prev_commit ? 1 : 0, parents));
	}

	struct simple_view {
		std::string_view name;
		std::vector<std::string_view> items{};
		auto operator<=>(simple_view const&) const noexcept = default;

		friend void PrintTo(simple_view const& view, std::ostream* out) {
			testing::print_view(*out << '{', view.name) << "sv, {";
			for (auto const& path : view.items) {
				testing::print_view(*out, path) << "sv, ";
			}
			*out << "}}";
		}
	};

	struct list {
		std::vector<std::string_view> const& items;

		friend std::ostream& operator<<(std::ostream& out, list const& L) {
			out << '{';
			for (auto const& path : L.items) {
				testing::print_view(out, path) << "sv,";
			}
			return out << "}";
		}
	};

	struct list_ptr {
		std::vector<cov::files::entry const*> const& items;

		friend std::ostream& operator<<(std::ostream& out, list_ptr const& L) {
			for (auto const& ptr : L.items) {
				testing::print_view(out << "\n"
				                           "      ",
				                    ptr->path());
			}
			return out;
		}
	};

	struct message {
		std::error_code ec;

		friend std::ostream& operator<<(std::ostream& out, message const& msg) {
			if (msg.ec.category() == git::category()) {
				auto const error = git_error_last();
				if (error && error->message && *error->message) {
					return out << msg.ec.message() << ": " << error->message;
				}
			}
			return out << msg.ec << ": " << msg.ec.message();
		}
	};

	struct filter_test {
		std::string_view prefix{};
		std::vector<simple_view> expected{};

		friend std::ostream& operator<<(std::ostream& out,
		                                filter_test const& test) {
			testing::print_view(out << "(prefix: ", test.prefix) << ", view: (";
			bool first = true;
			for (auto const& view : test.expected) {
				if (first)
					first = false;
				else
					out << ", ";
				testing::print_view(out, view.name);
			}
			return out << ')';
		}
	};

	struct print_to {
		std::vector<cov::testing::simple_view> const& ref;

		friend std::ostream& operator<<(std::ostream& out,
		                                print_to const& print) {
			::testing::internal::PrintTo(print.ref, &out);
			return out;
		}
	};

	struct modify_op {
		enum op_type { add, remove, remove_all, sep } op{};
		std::string_view mod_name{};
		std::string_view path{};
		mod_errc expected{mod_errc::needs_update};
	};

	struct modify_test {
		std::vector<modify_op> ops{};
		std::string_view expected;
	};

	class module_decl : public TestWithParam<filter_test> {
	protected:
		void setup_files(std::vector<std::string_view> const& files,
		                 ref_ptr<cov::files>& file_list,
		                 ref_ptr<modules>& mods) {
			files::builder builder{};
			for (auto const& file : files) {
				builder.add_nfo({.path = file});
			}

			file_list = builder.extract(git::oid{});
			ASSERT_TRUE(file_list);

			auto const cfg = git::config::create();
			auto ec = cfg.add_memory(this_project);
			ASSERT_FALSE(ec) << message{ec};

			mods = cov::modules::from_config(cfg, ec);
			ASSERT_FALSE(ec) << message{ec};
			ASSERT_TRUE(mods);
		}

		std::vector<simple_view> map(std::vector<module_view> const& view) {
			std::vector<simple_view> result{};
			result.reserve(view.size());
			std::transform(view.begin(), view.end(), std::back_inserter(result),
			               [](module_view const& view) {
				               simple_view group{.name = view.name};
				               group.items.reserve(view.items.size());
				               std::transform(
				                   view.items.begin(), view.items.end(),
				                   std::back_inserter(group.items),
				                   [](cov::files::entry const* entry) {
					                   return entry ? entry->path()
					                                : std::string_view{};
				                   });
				               return group;
			               });
			return result;
		}
	};

	class module_mod : public TestWithParam<modify_test> {};

	TEST_P(module_decl, filter_file_list) {
		auto const& [prefix, expected] = GetParam();

		git::init memory{};

		auto copy{all_files};
		if (!prefix.empty()) {
			std::erase_if(copy, [prefix = prefix](auto path) {
				return !path.starts_with(prefix);
			});
		}

		ref_ptr<files> file_list;
		ref_ptr<modules> mods;
		setup_files(copy, file_list, mods);
		if (::testing::Test::HasFatalFailure()) return;

		auto const actual = map(mods->filter(*file_list));
		ASSERT_EQ(expected, actual)
		    << "    prefix: " << prefix << "\n    files: " << print_to{actual};
	}

	TEST_P(module_decl, filter_pointer) {
		auto const& [prefix, expected] = GetParam();

		git::init memory{};

		ref_ptr<files> file_list;
		ref_ptr<modules> mods;
		setup_files(all_files, file_list, mods);
		if (::testing::Test::HasFatalFailure()) return;

		std::vector<cov::files::entry const*> copy{};
		copy.reserve(file_list->entries().size());
		std::transform(file_list->entries().begin(), file_list->entries().end(),
		               std::back_inserter(copy),
		               [](auto const& ptr) { return ptr.get(); });

		if (!prefix.empty()) {
			std::erase_if(copy, [prefix = prefix](auto ptr) {
				return !ptr->path().starts_with(prefix);
			});
		}

		auto const actual = map(mods->filter(copy));
		ASSERT_EQ(expected, actual)
		    << "    prefix: " << prefix << "\n    files: " << list_ptr{copy};
	}

	TEST(module_decl, from_cfg) {
		git::init memory{};

		auto const cfg = git::config::create();
		auto ec = cfg.add_memory(this_project);
		ASSERT_FALSE(ec) << message{ec};

		auto const mods = cov::modules::from_config(cfg, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);

		auto const obj = ref_ptr<object>{mods};
		ASSERT_TRUE(obj->is_modules());
		ASSERT_EQ(cov::obj_modules, obj->type());
		ASSERT_TRUE(as_a<cov::modules>(obj));

		ASSERT_EQ(project_modules, mods->entries());
		ASSERT_EQ(" \xc2\xbb "sv, mods->separator());
	}

	TEST(module_decl, from_commit) {
		git::init memory{};

		remove_all(setup::test_dir() / "modules.git"sv);

		std::error_code ec{};
		auto repo = git::repository::init(setup::test_dir() / "modules.git"sv,
		                                  true, ec);
		ASSERT_FALSE(ec) << message{ec};

		git::oid commit_id{};
		ec = commit_dot_modules(commit_id, nullptr, repo, this_project,
		                        "Add the cov modules");

		auto const mods = cov::modules::from_commit(commit_id, repo, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);

		ASSERT_EQ(project_modules, mods->entries());
	}

	TEST(module_decl, from_report_at_HEAD) {
		git::init memory{};

		remove_all(setup::test_dir() / "modules-at-HEAD"sv);

		std::error_code ec{};
		auto repo = git::repository::init(
		    setup::test_dir() / "modules-at-HEAD"sv, false, ec);
		ASSERT_FALSE(ec) << message{ec};

		auto cov_repo = cov::repository::init(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "modules-at-HEAD/.git/.covdata"sv,
		    setup::test_dir() / "modules-at-HEAD/.git"sv, ec);
		ASSERT_FALSE(ec) << message{ec};

		git::oid commit_id{};
		ec = commit_dot_modules(commit_id, nullptr, repo, this_project,
		                        "Add the cov modules");
		ASSERT_FALSE(ec) << message{ec};

		git::oid report_id{};
		{
			auto report = cov::report::create(git::oid{}, git::oid{}, commit_id,
			                                  {}, {}, {}, {}, {}, {}, {}, {});
			ASSERT_TRUE(cov_repo.write(report_id, report));
		}

		std::ofstream{setup::test_dir() / "modules-at-HEAD/.covmodule"sv}
		    << modified_project;

		auto const mods = cov::modules::from_report(report_id, cov_repo, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);

		ASSERT_EQ(modified_project_modules, mods->entries());
	}

	TEST(module_decl, from_report_at_HEAD_no_file) {
		git::init memory{};

		remove_all(setup::test_dir() / "modules-at-HEAD"sv);

		std::error_code ec{};
		auto repo = git::repository::init(
		    setup::test_dir() / "modules-at-HEAD"sv, false, ec);
		ASSERT_FALSE(ec) << message{ec};

		auto cov_repo = cov::repository::init(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "modules-at-HEAD/.git/.covdata"sv,
		    setup::test_dir() / "modules-at-HEAD/.git"sv, ec);
		ASSERT_FALSE(ec) << message{ec};

		git::oid commit_id{};
		ec = commit_dot_modules(commit_id, nullptr, repo, this_project,
		                        "Add the cov modules");
		ASSERT_FALSE(ec) << message{ec};

		git::oid report_id{};
		{
			auto report = cov::report::create(git::oid{}, git::oid{}, commit_id,
			                                  {}, {}, {}, {}, {}, {}, {}, {});
			ASSERT_TRUE(cov_repo.write(report_id, report));
		}

		auto const mods = cov::modules::from_report(report_id, cov_repo, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);
		ASSERT_TRUE(mods->entries().empty());
	}

	TEST(module_decl, from_report_in_commit) {
		git::init memory{};

		remove_all(setup::test_dir() / "modules-in-commit"sv);

		std::error_code ec{};
		auto repo = git::repository::init(
		    setup::test_dir() / "modules-in-commit"sv, false, ec);
		ASSERT_FALSE(ec) << message{ec};

		auto cov_repo = cov::repository::init(
		    setup::test_dir() / "sysroot"sv,
		    setup::test_dir() / "modules-in-commit/.git/.covdata"sv,
		    setup::test_dir() / "modules-in-commit/.git"sv, ec);
		ASSERT_FALSE(ec) << message{ec};

		git::oid commit_id{};
		ec = commit_dot_modules(commit_id, nullptr, repo, this_project,
		                        "Add the cov modules");
		ASSERT_FALSE(ec) << message{ec};

		git::oid report_id{};
		{
			auto report = cov::report::create(git::oid{}, git::oid{}, commit_id,
			                                  {}, {}, {}, {}, {}, {}, {}, {});
			ASSERT_TRUE(cov_repo.write(report_id, report));
		}

		std::ofstream{setup::test_dir() / "modules-in-commit/.covmodule"sv}
		    << modified_project;
		git::oid current;
		ec = commit_dot_modules(current, &commit_id, repo, modified_project,
		                        "Update .covmodule filters");
		ASSERT_FALSE(ec) << message{ec};

		auto const mods = cov::modules::from_report(report_id, cov_repo, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);

		ASSERT_EQ(project_modules, mods->entries());
	}

	TEST_P(module_mod, dump) {
		auto const& [ops, expected] = GetParam();

		git::init memory{};

		auto cfg = git::config::create();
		auto ec = cfg.add_memory(this_project);
		ASSERT_FALSE(ec) << message{ec};

		auto const mods = cov::modules::from_config(cfg, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);

		auto const str = [](auto view) {
			return std::string{view.data(), view.size()};
		};

		for (auto const& op : ops) {
			mod_errc actual{};
			switch (op.op) {
				case modify_op::add:
					actual = mods->add(str(op.mod_name), str(op.path));
					break;
				case modify_op::remove:
					actual = mods->remove(str(op.mod_name), str(op.path));
					break;
				case modify_op::remove_all:
					actual = mods->remove_all(str(op.mod_name));
					break;
				case modify_op::sep:
					actual = mods->set_separator(str(op.mod_name));
					break;
			}
			ASSERT_EQ(op.expected, actual);
		}

		auto const path = setup::test_dir() / "module-cfg"sv;
		remove(path);
		cfg = git::config::create();
		ec = cfg.add_file_ondisk(path, GIT_CONFIG_LEVEL_APP);
		ASSERT_FALSE(ec) << message{ec};
		cfg.set_string("module.secondary", "value");

		ec = mods->dump(cfg);
		ASSERT_FALSE(ec) << message{ec};
		cfg = nullptr;

		std::ifstream in{path};
		std::stringstream buffer;
		buffer << in.rdbuf();
		ASSERT_EQ(expected, buffer.str());
	}

	TEST_P(module_mod, dump_preexisting) {
		auto const& [ops, expected] = GetParam();

		git::init memory{};

		auto cfg = git::config::create();
		auto ec = cfg.add_memory(this_project);
		ASSERT_FALSE(ec) << message{ec};

		auto const mods = cov::modules::from_config(cfg, ec);
		ASSERT_FALSE(ec) << message{ec};
		ASSERT_TRUE(mods);

		auto const str = [](auto view) {
			return std::string{view.data(), view.size()};
		};

		for (auto const& op : ops) {
			mod_errc actual{};
			switch (op.op) {
				case modify_op::add:
					actual = mods->add(str(op.mod_name), str(op.path));
					break;
				case modify_op::remove:
					actual = mods->remove(str(op.mod_name), str(op.path));
					break;
				case modify_op::remove_all:
					actual = mods->remove_all(str(op.mod_name));
					break;
				case modify_op::sep:
					actual = mods->set_separator(str(op.mod_name));
					break;
			}
			ASSERT_EQ(op.expected, actual);
		}

		auto const path = setup::test_dir() / "module-cfg"sv;
		remove(path);
		{
			std::ofstream out{path};
			out.write(this_project.data(),
			          static_cast<std::streamsize>(this_project.size()));
		}
		cfg = git::config::create();
		ec = cfg.add_file_ondisk(path, GIT_CONFIG_LEVEL_APP);
		ASSERT_FALSE(ec) << message{ec};
		cfg.set_string("module.secondary", "value");

		ec = mods->dump(cfg);
		ASSERT_FALSE(ec) << message{ec};
		cfg = nullptr;
		modules::cleanup_config(path);

		std::ifstream in{path};
		std::stringstream buffer;
		buffer << in.rdbuf();
		ASSERT_EQ(expected, buffer.str());
	}

	TEST(module_mod, cleanup) {
		static constexpr auto config_text = R"(
; start with a blank line
# carry on:
[group] # this should be deleted
	# with this
    key = value # and this

; but not this...
[group2] # this comment should go away as well
    key = value
    key2 = value2 ;final comment to go
)"sv;
		static constexpr auto expected = R"([group2]
    key = value
    key2 = value2
)"sv;
		git::init memory{};

		auto const path = setup::test_dir() / "module-cfg"sv;
		remove(path);
		{
			std::ofstream out{path};
			out.write(config_text.data(),
			          static_cast<std::streamsize>(config_text.size()));
		}
		auto cfg = git::config::create();
		auto ec = cfg.add_file_ondisk(path, GIT_CONFIG_LEVEL_APP);
		ASSERT_FALSE(ec) << message{ec};
		cfg.delete_entry("group.key");

		cfg = nullptr;

		std::ifstream in0{path};
		std::stringstream buffer0;
		buffer0 << in0.rdbuf();
		in0.close();

		modules::cleanup_config(path);

		std::ifstream in{path};
		std::stringstream buffer;
		buffer << in.rdbuf();
		ASSERT_EQ(expected, buffer.str()) << "Previous: " << buffer0.str();
	}

	static filter_test const decl[] = {
	    {
	        {},
	        {{"git2 (c++)"sv,
	          {
	              "libs/git2-c++/include/git2/blob.hh"sv,
	              "libs/git2-c++/include/git2/bytes.hh"sv,
	              "libs/git2-c++/include/git2/commit.hh"sv,
	              "libs/git2-c++/include/git2/config.hh"sv,
	              "libs/git2-c++/include/git2/diff.hh"sv,
	              "libs/git2-c++/include/git2/error.hh"sv,
	              "libs/git2-c++/include/git2/global.hh"sv,
	              "libs/git2-c++/include/git2/object.hh"sv,
	              "libs/git2-c++/include/git2/odb.hh"sv,
	              "libs/git2-c++/include/git2/ptr.hh"sv,
	              "libs/git2-c++/include/git2/repository.hh"sv,
	              "libs/git2-c++/include/git2/tree.hh"sv,
	              "libs/git2-c++/src/blob.cc"sv,
	              "libs/git2-c++/src/commit.cc"sv,
	              "libs/git2-c++/src/config.cc"sv,
	              "libs/git2-c++/src/error.cc"sv,
	              "libs/git2-c++/src/odb.cc"sv,
	              "libs/git2-c++/src/repository.cc"sv,
	              "libs/git2-c++/src/tree.cc"sv,
	          }},
	         {"cov \xc2\xbb API"sv,
	          {
	              "libs/cov/include/cov/branch.hh"sv,
	              "libs/cov/include/cov/counted.hh"sv,
	              "libs/cov/include/cov/db.hh"sv,
	              "libs/cov/include/cov/discover.hh"sv,
	              "libs/cov/include/cov/format.hh"sv,
	              "libs/cov/include/cov/hash/hash.hh"sv,
	              "libs/cov/include/cov/hash/md5.hh"sv,
	              "libs/cov/include/cov/hash/sha1.hh"sv,
	              "libs/cov/include/cov/init.hh"sv,
	              "libs/cov/include/cov/io/db_object.hh"sv,
	              "libs/cov/include/cov/io/file.hh"sv,
	              "libs/cov/include/cov/io/read_stream.hh"sv,
	              "libs/cov/include/cov/io/safe_stream.hh"sv,
	              "libs/cov/include/cov/io/strings.hh"sv,
	              "libs/cov/include/cov/io/types.hh"sv,
	              "libs/cov/include/cov/object.hh"sv,
	              "libs/cov/include/cov/random.hh"sv,
	              "libs/cov/include/cov/reference.hh"sv,
	              "libs/cov/include/cov/report.hh"sv,
	              "libs/cov/include/cov/repository.hh"sv,
	              "libs/cov/include/cov/streams.hh"sv,
	              "libs/cov/include/cov/tag.hh"sv,
	              "libs/cov/include/cov/zstream.hh"sv,
	              "libs/cov/src/branch-tag.hh"sv,
	              "libs/cov/src/branch.cc"sv,
	              "libs/cov/src/counted.cc"sv,
	              "libs/cov/src/db.cc"sv,
	              "libs/cov/src/discover.cc"sv,
	              "libs/cov/src/format.cc"sv,
	              "libs/cov/src/hash/md5.cc"sv,
	              "libs/cov/src/hash/sha1.cc"sv,
	              "libs/cov/src/init.cc"sv,
	              "libs/cov/src/io/db_object-error.cc"sv,
	              "libs/cov/src/io/db_object.cc"sv,
	              "libs/cov/src/io/file.cc"sv,
	              "libs/cov/src/io/files.cc"sv,
	              "libs/cov/src/io/line_coverage.cc"sv,
	              "libs/cov/src/io/read_stream.cc"sv,
	              "libs/cov/src/io/report.cc"sv,
	              "libs/cov/src/io/safe_stream.cc"sv,
	              "libs/cov/src/io/strings.cc"sv,
	              "libs/cov/src/path-utils.hh"sv,
	              "libs/cov/src/ref/reference.cc"sv,
	              "libs/cov/src/ref/reference_list.cc"sv,
	              "libs/cov/src/ref/references.cc"sv,
	              "libs/cov/src/repository.cc"sv,
	              "libs/cov/src/streams.cc"sv,
	              "libs/cov/src/tag.cc"sv,
	              "libs/cov/src/zstream.cc"sv,
	          }},
	         {"cov \xc2\xbb runtime"sv,
	          {
	              "libs/cov-rt/include/cov/app/config.hh"sv,
	              "libs/cov-rt/include/cov/app/tools.hh"sv,
	              "libs/cov-rt/src/config.cc"sv,
	              "libs/cov-rt/src/root_command.cc"sv,
	              "libs/cov-rt/src/split_command.cc"sv,
	              "libs/cov-rt/src/tools.cc"sv,
	              "libs/cov-rt/src/win32.cc"sv,
	          }},
	         {"app \xc2\xbb library"sv,
	          {
	              "libs/app/include/cov/app/args.hh"sv,
	              "libs/app/include/cov/app/tr.hh"sv,
	              "libs/app/src/args.cc"sv,
	              "libs/app/src/main.cc"sv,
	              "libs/app/src/strings/args.cc"sv,
	              "libs/app/src/strings/cov.cc"sv,
	              "libs/app/src/strings/cov_config.cc"sv,
	              "libs/app/src/strings/cov_init.cc"sv,
	              "libs/app/src/strings/cov_module.cc"sv,
	              "libs/app/src/strings/errors.cc"sv,
	              "libs/app/src/tr.cc"sv,
	              "libs/app/src/win32.cc"sv,
	          }},
	         {"app \xc2\xbb main"sv,
	          {
	              "apps/builtins/builtin_config.cc"sv,
	              "apps/builtins/builtin_init.cc"sv,
	              "apps/builtins/builtin_module.cc"sv,
	              "apps/cov/cov.cc"sv,
	          }},
	         {"hilite"sv,
	          {
	              "libs/hilite/hilite/include/cell/ascii.hh"sv,
	              "libs/hilite/hilite/include/cell/character.hh"sv,
	              "libs/hilite/hilite/include/cell/context.hh"sv,
	              "libs/hilite/hilite/include/cell/operators.hh"sv,
	              "libs/hilite/hilite/include/cell/parser.hh"sv,
	              "libs/hilite/hilite/include/cell/repeat_operators.hh"sv,
	              "libs/hilite/hilite/include/cell/special.hh"sv,
	              "libs/hilite/hilite/include/cell/string.hh"sv,
	              "libs/hilite/hilite/include/cell/tokens.hh"sv,
	              "libs/hilite/hilite/include/hilite/hilite.hh"sv,
	              "libs/hilite/hilite/include/hilite/hilite_impl.hh"sv,
	              "libs/hilite/hilite/src/hilite.cc"sv,
	              "libs/hilite/hilite/src/none.cc"sv,
	              "libs/hilite/lighter/src/hilite/lighter.hh"sv,
	              "libs/hilite/lighter/src/lighter.cc"sv,
	          }},
	         {"hilite \xc2\xbb c++"sv,
	          {
	              "libs/hilite/hilite-cxx/src/cxx.cc"sv,
	          }}},
	    },

	    {
	        "libs/cov"sv,
	        {{"cov \xc2\xbb API"sv,
	          {
	              "libs/cov/include/cov/branch.hh"sv,
	              "libs/cov/include/cov/counted.hh"sv,
	              "libs/cov/include/cov/db.hh"sv,
	              "libs/cov/include/cov/discover.hh"sv,
	              "libs/cov/include/cov/format.hh"sv,
	              "libs/cov/include/cov/hash/hash.hh"sv,
	              "libs/cov/include/cov/hash/md5.hh"sv,
	              "libs/cov/include/cov/hash/sha1.hh"sv,
	              "libs/cov/include/cov/init.hh"sv,
	              "libs/cov/include/cov/io/db_object.hh"sv,
	              "libs/cov/include/cov/io/file.hh"sv,
	              "libs/cov/include/cov/io/read_stream.hh"sv,
	              "libs/cov/include/cov/io/safe_stream.hh"sv,
	              "libs/cov/include/cov/io/strings.hh"sv,
	              "libs/cov/include/cov/io/types.hh"sv,
	              "libs/cov/include/cov/object.hh"sv,
	              "libs/cov/include/cov/random.hh"sv,
	              "libs/cov/include/cov/reference.hh"sv,
	              "libs/cov/include/cov/report.hh"sv,
	              "libs/cov/include/cov/repository.hh"sv,
	              "libs/cov/include/cov/streams.hh"sv,
	              "libs/cov/include/cov/tag.hh"sv,
	              "libs/cov/include/cov/zstream.hh"sv,
	              "libs/cov/src/branch-tag.hh"sv,
	              "libs/cov/src/branch.cc"sv,
	              "libs/cov/src/counted.cc"sv,
	              "libs/cov/src/db.cc"sv,
	              "libs/cov/src/discover.cc"sv,
	              "libs/cov/src/format.cc"sv,
	              "libs/cov/src/hash/md5.cc"sv,
	              "libs/cov/src/hash/sha1.cc"sv,
	              "libs/cov/src/init.cc"sv,
	              "libs/cov/src/io/db_object-error.cc"sv,
	              "libs/cov/src/io/db_object.cc"sv,
	              "libs/cov/src/io/file.cc"sv,
	              "libs/cov/src/io/files.cc"sv,
	              "libs/cov/src/io/line_coverage.cc"sv,
	              "libs/cov/src/io/read_stream.cc"sv,
	              "libs/cov/src/io/report.cc"sv,
	              "libs/cov/src/io/safe_stream.cc"sv,
	              "libs/cov/src/io/strings.cc"sv,
	              "libs/cov/src/path-utils.hh"sv,
	              "libs/cov/src/ref/reference.cc"sv,
	              "libs/cov/src/ref/reference_list.cc"sv,
	              "libs/cov/src/ref/references.cc"sv,
	              "libs/cov/src/repository.cc"sv,
	              "libs/cov/src/streams.cc"sv,
	              "libs/cov/src/tag.cc"sv,
	              "libs/cov/src/zstream.cc"sv,
	          }},
	         {"cov \xc2\xbb runtime"sv,
	          {
	              "libs/cov-rt/include/cov/app/config.hh"sv,
	              "libs/cov-rt/include/cov/app/tools.hh"sv,
	              "libs/cov-rt/src/config.cc"sv,
	              "libs/cov-rt/src/root_command.cc"sv,
	              "libs/cov-rt/src/split_command.cc"sv,
	              "libs/cov-rt/src/tools.cc"sv,
	              "libs/cov-rt/src/win32.cc"sv,
	          }}},
	    },

	    {
	        "libs/hilite/"sv,
	        {{"hilite"sv,
	          {
	              "libs/hilite/hilite/include/cell/ascii.hh"sv,
	              "libs/hilite/hilite/include/cell/character.hh"sv,
	              "libs/hilite/hilite/include/cell/context.hh"sv,
	              "libs/hilite/hilite/include/cell/operators.hh"sv,
	              "libs/hilite/hilite/include/cell/parser.hh"sv,
	              "libs/hilite/hilite/include/cell/repeat_operators.hh"sv,
	              "libs/hilite/hilite/include/cell/special.hh"sv,
	              "libs/hilite/hilite/include/cell/string.hh"sv,
	              "libs/hilite/hilite/include/cell/tokens.hh"sv,
	              "libs/hilite/hilite/include/hilite/hilite.hh"sv,
	              "libs/hilite/hilite/include/hilite/hilite_impl.hh"sv,
	              "libs/hilite/hilite/src/hilite.cc"sv,
	              "libs/hilite/hilite/src/none.cc"sv,
	              "libs/hilite/lighter/src/hilite/lighter.hh"sv,
	              "libs/hilite/lighter/src/lighter.cc"sv,
	          }},
	         {"hilite \xc2\xbb c++"sv,
	          {
	              "libs/hilite/hilite-cxx/src/cxx.cc"sv,
	          }}},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(tests, module_decl, ValuesIn(decl));

	static modify_test const mods[] = {
	    {
	        {},
	        "[module]\n"
	        "\tsecondary = value\n"
	        "\tsep = \" \xC2\xBB \"\n"
	        "[module \"git2 (c++)\"]\n"
	        "\tpath = libs/git2-c++\n"
	        "[module \"cov \xC2\xBB API\"]\n"
	        "\tpath = libs/cov\n"
	        "[module \"cov \xC2\xBB runtime\"]\n"
	        "\tpath = libs/cov-rt\n"
	        "[module \"app \xC2\xBB library\"]\n"
	        "\tpath = libs/app\n"
	        "[module \"app \xC2\xBB main\"]\n"
	        "\tpath = apps/cov/\n"
	        "\tpath = apps/builtins/\n"
	        "[module \"hilite\"]\n"
	        "\tpath = libs/hilite/hilite\n"
	        "\tpath = libs/hilite/lighter\n"
	        "[module \"hilite \xC2\xBB c++\"]\n"
	        "\tpath = libs/hilite/hilite-cxx\n"
	        "[module \"hilite \xC2\xBB python\"]\n"
	        "\tpath = libs/hilite/hilite-py3\n"
	        "[module \"hilite \xC2\xBB type script\"]\n"
	        "\tpath = libs/hilite/hilite-ts\n"sv,
	    },
	    {
	        {
	            {modify_op::remove_all, "git2 (c++)"sv},
	            {modify_op::remove_all, "hilite"sv},
	            {modify_op::remove_all, "hilite \xc2\xbb c++"sv},
	            {modify_op::remove_all, "hilite \xc2\xbb python"sv},
	            {modify_op::remove_all, "hilite \xc2\xbb type script"sv},
	            {modify_op::remove, "cov \xC2\xBB API"sv, "libs/cov"sv},
	            {modify_op::remove, "app \xC2\xBB main"sv, "libs/cov"sv,
	             mod_errc::unmodified},
	            {modify_op::add, "mod1"sv, "path1"sv},
	            {modify_op::add, "mod1"sv, "path2"sv},
	            {modify_op::add, "mod1"sv, "path3"sv},
	            {modify_op::sep, "::"},
	        },
	        "[module]\n"
	        "\tsecondary = value\n"
	        "\tsep = ::\n"
	        "[module \"cov \xC2\xBB runtime\"]\n"
	        "\tpath = libs/cov-rt\n"
	        "[module \"app \xC2\xBB library\"]\n"
	        "\tpath = libs/app\n"
	        "[module \"app \xC2\xBB main\"]\n"
	        "\tpath = apps/cov/\n"
	        "\tpath = apps/builtins/\n"
	        "[module \"mod1\"]\n"
	        "\tpath = path1\n"
	        "\tpath = path2\n"
	        "\tpath = path3\n"
	        ""sv,
	    },
	};
	INSTANTIATE_TEST_SUITE_P(tests, module_mod, ValuesIn(mods));
}  // namespace cov::testing
