// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cov/git2/global.hh>
#include <cov/module.hh>
#include <memory>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <web/components.hh>
#include "setup.hh"

using namespace std::literals;

namespace cov::app::web::testing {
	using ::testing::ConvertGenerator;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	using namespace projection;

	inline std::string str(std::string_view view) {
		return {view.data(), view.size()};
	}

	struct template_context {
		virtual ~template_context() = default;
		virtual bool apply_to(mstch::map& ctx,
		                      link_service const& links) const = 0;
		virtual std::unique_ptr<template_context> clone() const = 0;
	};

	struct component_test {
		std::string_view tmplt;
		std::string_view expected;
		std::unique_ptr<template_context> context;

		component_test() = default;
		component_test(component_test const& oth)
		    : tmplt{oth.tmplt}
		    , expected{oth.expected}
		    , context{oth.context->clone()} {}
		component_test(component_test&& oth) = default;
		component_test& operator=(component_test const& oth) {
			tmplt = oth.tmplt;
			expected = oth.expected;
			context = oth.context->clone();
			return *this;
		}
		component_test& operator=(component_test&& oth) = default;
		component_test(std::string_view tmplt,
		               std::string_view expected,
		               std::unique_ptr<template_context> context)
		    : tmplt{tmplt}, expected{expected}, context{std::move(context)} {}
	};

	class components : public TestWithParam<component_test> {
	protected:
		class mustache : public mstch::cache {
		public:
			explicit mustache(std::string_view tmplt) : template_{tmplt} {}

			std::string load(std::string const&) override {
				return {template_.data(), template_.size()};
			}
			bool need_update(std::string const&) const override {
				return false;
			}
			bool is_valid(std::string const&) const override { return true; }

		private:
			std::string_view template_;
		};
	};

	TEST_P(components, render) {
		auto const& [tmplt, expected, context] = GetParam();
		mustache renderer{tmplt};
		export_link_service links{};
		links.adjust_root("a/path/to/file");

		mstch::map ctx{};
		if (!context->apply_to(ctx, links)) {
			ASSERT_TRUE(false);
		}

		ASSERT_EQ(expected, renderer.render(""s, ctx));

		fmt::print("{}\n", expected);
	}

	namespace wrapper {
		std::string str(std::string_view view) {
			return {view.data(), view.size()};
		}

		template <typename T>
		concept ReturnsBool =
		    requires(T fn, mstch::map& ctx, link_service const& links) {
			    { fn(ctx, links) } -> std::same_as<bool>;
		    };

		template <typename T>
		concept ReturnsVoid =
		    requires(T fn, mstch::map& ctx, link_service const& links) {
			    { fn(ctx, links) } -> std::same_as<void>;
		    };

		template <typename Impl>
		struct template_context_impl;

		template <typename Impl>
		struct template_context_impl_base : template_context {
			Impl impl;
			template_context_impl_base() = default;
			template_context_impl_base(Impl const& impl) : impl{impl} {}
			template_context_impl_base(Impl&& impl) : impl{std::move(impl)} {}
			template_context_impl_base(template_context_impl_base const& impl) =
			    default;
			template_context_impl_base(template_context_impl_base&& impl) =
			    default;
			template_context_impl_base& operator=(
			    template_context_impl_base const& impl) = default;
			template_context_impl_base& operator=(
			    template_context_impl_base&& impl) = default;
		};

		template <ReturnsBool Impl>
		struct template_context_impl<Impl> : template_context_impl_base<Impl> {
			using template_context_impl_base<Impl>::template_context_impl_base;
			bool apply_to(mstch::map& ctx,
			              link_service const& links) const override {
				return this->impl(ctx, links);
			}

			std::unique_ptr<template_context> clone() const override {
				return std::make_unique<template_context_impl<Impl>>(*this);
			}
		};

		template <ReturnsVoid Impl>
		struct template_context_impl<Impl> : template_context_impl_base<Impl> {
			using template_context_impl_base<Impl>::template_context_impl_base;
			bool apply_to(mstch::map& ctx,
			              link_service const& links) const override {
				this->impl(ctx, links);
				return true;
			}

			std::unique_ptr<template_context> clone() const override {
				return std::make_unique<template_context_impl<Impl>>(*this);
			}
		};

		template <typename Impl>
		    requires ReturnsBool<Impl> || ReturnsVoid<Impl>
		std::unique_ptr<template_context> wrap(Impl&& impl) {
			using SimpleImpl = std::remove_cvref_t<Impl>;
			using Context = template_context_impl<SimpleImpl>;
			return std::make_unique<Context>(std::forward<Impl>(impl));
		}

		std::unique_ptr<template_context> wrap_payload(translatable mark) {
			return wrap([mark](mstch::map& ctx, link_service const&) {
				add_mark(ctx, mark);
			});
		}

		struct navigation {
			bool is_root;
			bool is_module;
			bool is_file;
			std::string_view path;
		};

		std::unique_ptr<template_context> wrap_payload(navigation const& nav) {
			std::string path{};
			path.assign(nav.path);
			return wrap([=](mstch::map& ctx, link_service const& links) {
				add_navigation(ctx, nav.is_root, nav.is_module, nav.is_file,
				               path, links);
			});
		}

		struct entry_info {
			entry_type type{};
			std::string_view label{};
			std::string_view expanded{};
			io::v1::coverage_stats stats{coverage_stats::init()};
			io::v1::coverage_stats prev{coverage_stats::init()};
		};

		std::unique_ptr<template_context> wrap_payload(
		    std::span<entry_info const> report) {
			static constexpr placeholder::rating marks{
			    .lines{.incomplete{75, 100}, .passing{9, 10}},
			    .functions{.incomplete{75, 100}, .passing{9, 10}},
			    .branches{.incomplete{75, 100}, .passing{9, 10}}};

			std::vector<entry> entries{};
			entries.reserve(report.size());
			for (auto const& item : report) {
				entries.push_back({
				    .type = item.type,
				    .name = {.display = str(item.label),
				             .expanded = str(item.expanded)},
				    .stats = {.current = item.stats, .previous = item.prev},
				});
			}
			auto const [total, flags] = core::stats::calc_stats(entries);
			auto const table =
			    core::stats::project(marks, entries, total, flags);

			return wrap([=](mstch::map& ctx, link_service const& links) {
				add_table(ctx, table, links);
			});
		}

		struct build_info {
			std::string_view oid;
			std::string_view from{};
		};

		std::unique_ptr<template_context> wrap_payload(
		    build_info const& build) {
			return wrap([=](mstch::map& ctx, link_service const&) -> bool {
				git::init init{};

				auto const [oid_str, from_str] = build;
				auto const oid = git::oid::from(oid_str);
				auto const from =
				    from_str.empty() ? git::oid{} : git::oid::from(from_str);

				std::error_code ec{};
				auto const covdir =
				    cov::repository::discover(setup::test_dir(), ec);
				if (ec) {
					fmt::print("cov::repository::discover: error {}: {}\n",
					           ec.value(), ec.message());
					return false;
				}

				auto repo = cov::repository::open(
				    setup::test_dir() / "sysroot"sv, covdir, ec);
				if (ec) {
					fmt::print("cov::repository::open: error {}: {}\n",
					           ec.value(), ec.message());
					return false;
				}

				auto [commit_ctx, report_ctx] =
				    add_build_info(repo, oid, from, ec);
				if (ec) {
					fmt::print("add_build_info: error {}: {}\n", ec.value(),
					           ec.message());
					return false;
				}

				ctx["commit"] = std::move(commit_ctx);
				ctx["report"] = std::move(report_ctx);
				return true;
			});
		}

		struct file_source {
			std::string_view oid;
			std::string_view path;
		};

		std::unique_ptr<template_context> wrap_payload(
		    file_source const& file) {
			return wrap([=](mstch::map& ctx, link_service const&) -> bool {
				git::init init{};

				auto const [oid_str, path] = file;
				auto const oid = git::oid::from(oid_str);

				std::error_code ec{};
				auto const covdir =
				    cov::repository::discover(setup::test_dir(), ec);
				if (ec) {
					fmt::print("cov::repository::discover: error {}: {}\n",
					           ec.value(), ec.message());
					return false;
				}

				auto repo = cov::repository::open(
				    setup::test_dir() / "sysroot"sv, covdir, ec);
				if (ec) {
					fmt::print("cov::repository::open: error {}: {}\n",
					           ec.value(), ec.message());
					return false;
				}

				cxx_filt::Replacements replacements{};

				add_file_source(ctx, repo, oid, path, replacements, ec);
				if (ec) {
					fmt::print("add_file_source: error {}: {}\n", ec.value(),
					           ec.message());
					return false;
				}
				return true;
			});
		}

		struct page_info {
			std::string_view ref{};
			std::string_view fname_filter{};
			std::string_view module_filter{};
			std::string_view modules{};
			std::span<std::string_view const> files{};
		};

		struct dummy_provider : lng_provider {
#define X(LNG) std::string_view LNG##_ = #LNG ""sv;
			MSTCH_LNG(X)
#undef X

			std::string_view get(lng id) const noexcept override {
				try {
					switch (id) {
#define X(LNG)     \
	case lng::LNG: \
		return LNG##_;
						MSTCH_LNG(X)
#undef X
					}
				} catch (...) {
				}
				return ""sv;
			}
		};

		static dummy_provider provider{};

		ref_ptr<modules> modules_from(std::string_view config) {
			git::config cfg{};
			auto ec = cfg.add_memory(config);
			if (ec) return {};
			return modules::from_config(cfg, ec);
		}

		std::unique_ptr<template_context> wrap_payload(page_info const& page) {
			return wrap([=](mstch::map& ctx,
			                link_service const& links) -> bool {
				git::init init{};

				auto const oid = git::oid::from(page.ref);

				std::error_code ec{};
				auto const covdir =
				    cov::repository::discover(setup::test_dir(), ec);
				if (ec) {
					fmt::print("cov::repository::discover: error {}: {}\n",
					           ec.value(), ec.message());
					return false;
				}

				auto repo = cov::repository::open(
				    setup::test_dir() / "sysroot"sv, covdir, ec);
				if (ec) {
					fmt::print("cov::repository::open: error {}: {}\n",
					           ec.value(), ec.message());
					return false;
				}

				auto mods = modules_from(page.modules);
				auto view = projection::report_filter{mods.get(),
				                                      str(page.module_filter),
				                                      str(page.fname_filter)};
				std::vector<file_stats> diff{};
				diff.reserve(page.files.size());
				for (auto const file : page.files) {
					diff.push_back({.filename = str(file)});
				}

				auto entries = view.project(diff);
				static constexpr placeholder::rating marks{
				    .lines{.incomplete{75, 100}, .passing{9, 10}},
				    .functions{.incomplete{75, 100}, .passing{9, 10}},
				    .branches{.incomplete{75, 100}, .passing{9, 10}}};

				cxx_filt::Replacements replacements{};

				auto const is_standalone = add_page_context(
				    ctx, view, entries, oid, marks, repo, mstch::array{},
				    mstch::array{},
				    mstch::map{{"number", 3.14},
				               {"long-long", 5ll},
				               {"none", nullptr},
				               {"lngs", lng_callback::create(&provider)}},
				    replacements, true, links, ec);
				ctx["is-standalone"] = is_standalone;

				if (ec) {
					fmt::print("is_standalone: error {}: {}\n", ec.value(),
					           ec.message());
					return false;
				}
				return true;
			});
		}

		template <typename Payload>
		struct component_test {
			std::string_view tmplt;
			std::string_view expected;
			Payload context;

			testing::component_test wrap() const {
				return {tmplt, expected, wrap_payload(context)};
			}

			operator testing::component_test() const { return wrap(); }
		};

	}  // namespace wrapper

#define TAG(NAME, TYPE) "{{" TYPE NAME "}}"
#define OPEN_TAG(NAME) TAG(NAME, "# ")
#define OPEN_NOT_TAG(NAME) TAG(NAME, "^ ")
#define CLOSE_TAG(NAME) TAG(NAME, "/ ")
#define PRINT(NAME) TAG(NAME, "")
#define PRINT_RAW(NAME) "{" PRINT(NAME) "}"
#define OPEN(NAME, CONTENT) OPEN_TAG(NAME) CONTENT CLOSE_TAG(NAME)
#define OPEN_NOT(NAME, CONTENT) OPEN_NOT_TAG(NAME) CONTENT CLOSE_TAG(NAME)

#define ICON                 \
	OPEN("is-module", "[M]") \
	OPEN("is-directory", "[D]") OPEN("is-file", "[F]")

#define LINK(VAR) "(" PRINT(VAR "display") ")[" PRINT(VAR "href") "]"

	static constexpr auto mark_template =
	    OPEN("is-passing", "OK") OPEN("is-incomplete", "??")
	        OPEN("is-failing", "!!") " -- " PRINT("mark") ""sv;

	static constexpr auto nav_template =
	    OPEN("breadcrumbs",
	         OPEN("is-root", "[R] " PRINT("heading.display")) OPEN_NOT(
	             "is-root",
	             ICON " " LINK("heading.")
	                 OPEN("nav", " :: " LINK("")) " :: " PRINT("leaf"))) ""sv;

#define ROW_LABEL                                                        \
	ICON OPEN("is-total", "[T]") " " OPEN(                               \
	    "has-href", OPEN("has-prefix", PRINT("prefix") " :: ") LINK("")) \
	    OPEN_NOT("has-href",                                             \
	             OPEN("has-prefix", PRINT("prefix") " :: ") PRINT("display"))
#define ROW ROW_LABEL OPEN("cells", " | " PRINT("value") " " PRINT("change"))
#define PERSON PRINT("name") " <" PRINT("email") "> (" PRINT("hash") ")"
#define EQ_PRINT(name) name ": " PRINT(name)

	static constexpr auto table_template = OPEN(
	    "stats",
	    OPEN("columns",
	         OPEN("is-name", PRINT("display"))
	             OPEN_NOT("is-name",
	                      " | " PRINT("display"))) "\n" OPEN("rows", ROW "\n")
	        OPEN("total", ROW)) ""sv;

	static constexpr auto build_info_template =
	    OPEN("commit",
	         "COMMIT:"
			 "\n - " EQ_PRINT("subject")
	         "\n - " EQ_PRINT("description")
	         "\n - " EQ_PRINT("different_author")
	         "\n - author: " OPEN("author", PERSON)
	         "\n - committer: " OPEN("committer", PERSON)
	         "\n - " EQ_PRINT("branch")
	         "\n - " EQ_PRINT("commit-id")
	         "\n - " EQ_PRINT("date-YmdHM")
		)  //
		OPEN("report",
			"\nREPORT:"
		    "\n - " EQ_PRINT("report-id")
			"\n - " EQ_PRINT("base.present")
			"\n - " EQ_PRINT("base.id")
			"\n - " EQ_PRINT("base.label")
			"\n - " EQ_PRINT("base.is-tag")
			"\n - " EQ_PRINT("base.is-branch")
			OPEN("properties",
				"\n - properties:"
				OPEN("propset",
					"\n   - "
					PRINT("name") ": " PRINT("prop-type") " = "
					OPEN("is-string", "\"" PRINT("value") "\"")
					OPEN_NOT("is-string", PRINT("value")) ";"
					)
			)
		    "\n - date-YmdHM: " PRINT("date-YmdHM")
		)  //
		""sv;

	static constexpr auto file_source_template =
	    "- " EQ_PRINT("file-is-present")  //
	    OPEN("file-is-present",
	         "\n- " EQ_PRINT("last-line")      //
	         "\n- " EQ_PRINT("file-is-empty")  //
	         OPEN_NOT("file-is-empty",
	                  "\n- file-chunks:" OPEN(
	                      "file-chunks",
	                      "\n  - " EQ_PRINT("missing-before")  //
	                      "\n  - " EQ_PRINT("missing-after")   //
	                      "\n  - lines:"                       //
	                      OPEN("lines",
	                           "\n    - " PRINT("line-no") " | "    //
	                           OPEN_NOT("is-null", PRINT("count"))  //
	                           OPEN("is-null", " ") " | "           //
	                           PRINT("class-name") " | "            //
	                           PRINT_RAW("text")                    //
	                           OPEN("functions",
	                                "\n      - "               //
	                                PRINT("count") " | "       //
	                                PRINT("class-name") " | "  //
	                                PRINT_RAW("name")          //
	                                )                          //
	                           )                               //
	                      )                                    //
	                  )                                        //
	         )                                                 //
	    ""sv;

	static constexpr auto page_info_template =  //
	    "- " EQ_PRINT("is-standalone")          //
	    "\n- json: " PRINT_RAW("json")          //
	    ""sv;

	static constexpr wrapper::component_test<cov::translatable> mark_tests[] = {
	    {.tmplt = mark_template,
	     .expected = "OK -- passing"sv,
	     .context = cov::translatable::mark_passing},
	    {.tmplt = mark_template,
	     .expected = "?? -- incomplete"sv,
	     .context = cov::translatable::mark_incomplete},
	    {.tmplt = mark_template,
	     .expected = "!! -- failing"sv,
	     .context = cov::translatable::mark_failing},
	    {.tmplt = mark_template,
	     .expected = " -- "sv,
	     .context = cov::translatable::in_the_future},
	};

	INSTANTIATE_TEST_SUITE_P(mark,
	                         components,
	                         ConvertGenerator(ValuesIn(mark_tests)));

	static constexpr wrapper::component_test<wrapper::navigation> nav_tests[] = {
	    {
	        .tmplt = nav_template,
	        .expected = "[R] repository"sv,
	        .context =
	            {.is_root{true}, .is_module{true}, .is_file{true}, .path{}},
	    },
	    {
	        .tmplt = nav_template,
	        .expected =
	            "[M] (repository)[../../../index.html] :: (mod)[../../../mods/mod.html] :: name"sv,
	        .context = {.is_root{false},
	                    .is_module{true},
	                    .is_file{true},
	                    .path{"mod/name"}},
	    },
	    {
	        .tmplt = nav_template,
	        .expected = "[D] (repository)[../../../index.html] :: src"sv,
	        .context = {.is_root{false},
	                    .is_module{false},
	                    .is_file{false},
	                    .path{"src"}},
	    },
	    {
	        .tmplt = nav_template,
	        .expected =
	            "[F] (repository)[../../../index.html] :: (src)[../../../dirs/src.html] :: file.cc"sv,
	        .context = {.is_root{false},
	                    .is_module{false},
	                    .is_file{true},
	                    .path{"src/file.cc"}},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(nav,
	                         components,
	                         ConvertGenerator(ValuesIn(nav_tests)));

	consteval coverage_stats stats(uint32_t total,
	                               uint32_t relevant,
	                               uint32_t covered) {
		return {total, {relevant, covered}, {0, 0}, {0, 0}};
	}

	consteval coverage_stats stats(uint32_t total,
	                               uint32_t relevant_lines,
	                               uint32_t covered_lines,
	                               uint32_t relevant_funcs,
	                               uint32_t covered_funcs,
	                               uint32_t relevant_branches = 0u,
	                               uint32_t covered_branches = 0u) {
		return {
		    total,
		    {relevant_lines, covered_lines},
		    {relevant_funcs, covered_funcs},
		    {relevant_branches, covered_branches},
		};
	}

	template <typename T, size_t Length>
	consteval std::span<T const> span(T const (&v)[Length]) noexcept {
		return {v, Length};
	}

	static wrapper::component_test<std::vector<wrapper::entry_info>> const
	    table_tests[] = {
	        {
	            .tmplt = table_template,
	            .expected =
	                // clang-format off
					"Name | Lines | Visited | Relevant | Missing | Line count\n"
					"[D] (apps)[../../../dirs/apps.html] | 75.41% +75.41% | 911 +911 | 1208 +1208 | 297 +297 | 1740 +1740\n"
					"[D] (libs)[../../../dirs/libs.html] | 60.94% +60.94% | 21642 +21642 | 35511 +35511 | 13869 +13869 | 46146 +46146\n"
					"[T] Total | 61.42% +61.42% | 22553 +22553 | 36719 +36719 | 14166 +14166 | 47886 +47886"
					""sv,
	            // clang-format on
	            .context =
	                {
	                    {entry_type::directory, "apps"sv, "apps"sv,
	                     stats(1740, 1208, 911)},
	                    {entry_type::directory, "libs"sv, "libs"sv,
	                     stats(46146, 35511, 21642)},
	                },
	        },
	        {
	            .tmplt = table_template,
	            .expected =
	                // clang-format off
					"Name | Functions | Relevant | Missing | Lines | Visited | Relevant | Missing | Line count\n"
					"[F] dir/subdir/ :: (file.ext)[../../../files/current/filter/dir/subdir/file.ext.html] | 0.00% -84.91% | 53 \xC2\xA0 | 53 +45 | 75.41% \xC2\xA0 | 911 \xC2\xA0 | 1208 \xC2\xA0 | 297 \xC2\xA0 | 1740 \xC2\xA0\n"
					""sv,
	            // clang-format on
	            .context =
	                {
	                    {entry_type::file, "dir/subdir/file.ext"sv,
	                     "current/filter/dir/subdir/file.ext"sv,
	                     stats(1740, 1208, 911, 53, 0),
	                     stats(1740, 1208, 911, 53, 45)},
	                },
	        },
	        {
	            .tmplt = table_template,
	            .expected =
	                // clang-format off
					"Name | Branches | Relevant | Missing | Lines | Visited | Relevant | Line count\n"
					"[M] (mod1)[../../../mods/mod1.html] | 80.00% +80.00% | 1000 +1000 | 200 +200 | 100.00% \xC2\xA0 | 0 \xC2\xA0 | 0 \xC2\xA0 | 0 \xC2\xA0\n"
					"[M] (mod2)[../../../mods/mod2.html] | 100.00% \xC2\xA0 | 0 \xC2\xA0 |   | 100.00% \xC2\xA0 | 0 \xC2\xA0 | 0 \xC2\xA0 | 0 \xC2\xA0\n"
					"[T] Total | 80.00% +80.00% | 1000 +1000 | 200 +200 | 100.00% \xC2\xA0 | 0 \xC2\xA0 | 0 \xC2\xA0 | 0 \xC2\xA0"
					""sv,
	            // clang-format on
	            .context =
	                {
	                    {entry_type::module,
	                     "mod1"sv,
	                     "mod1"sv,
	                     {0, {0, 0}, {0, 0}, {1000, 800}}},
	                    {entry_type::module,
	                     "mod2"sv,
	                     "mod2"sv,
	                     {0, {0, 0}, {0, 0}, {0, 0}}},
	                },
	        },
	        {
	            .tmplt = table_template,
	            .expected =
	                // clang-format off
					"Name | Branches | Relevant | Missing | Lines | Visited | Relevant | Missing | Line count\n"
					"[M] (mod1)[../../../mods/mod1.html] | 80.00% \xC2\xA0 | 5 \xC2\xA0 | 1 \xC2\xA0 | 0.00% \xC2\xA0 | 0 \xC2\xA0 | 1 \xC2\xA0 | 1 \xC2\xA0 | 0 \xC2\xA0\n"
					"[M] (mod2)[../../../mods/mod2.html] | 100.00% +100.00% | 1 \xC2\xA0 | 0 -1 | 100.00% +100.00% | 1 +1 | 1 \xC2\xA0 | 0 -1 | 0 \xC2\xA0\n"
					"[T] Total | 83.33% +16.66% | 6 \xC2\xA0 | 1 -1 | 50.00% +50.00% | 1 +1 | 2 \xC2\xA0 | 1 -1 | 0 \xC2\xA0"
					""sv,
	            // clang-format on
	            .context =
	                {
	                    {entry_type::module,
	                     "mod1"sv,
	                     "mod1"sv,
	                     {0, {1, 0}, {0, 0}, {5, 4}},
	                     {0, {1, 0}, {0, 0}, {5, 4}}},
	                    {entry_type::module,
	                     "mod2"sv,
	                     "mod2"sv,
	                     {0, {1, 1}, {0, 0}, {1, 1}},
	                     {0, {1, 0}, {0, 0}, {1, 0}}},
	                },
	        },
	};

	INSTANTIATE_TEST_SUITE_P(table,
	                         components,
	                         ConvertGenerator(ValuesIn(table_tests)));

	static constexpr wrapper::component_test<
	    wrapper::build_info> const build_info_tests[] = {
	    {
	        .tmplt = build_info_template,
	        .expected =
	            // clang-format off
	            "COMMIT:\n"
				" - subject: Initial\n"
				" - description: \n"
				" - different_author: false\n"
				" - author: Johnny Appleseed <johnny@appleseed.com> (8371e7982b4c56cfa544d08389fb1fb5)\n"
				" - committer: Johnny Appleseed <johnny@appleseed.com> (8371e7982b4c56cfa544d08389fb1fb5)\n"
				" - branch: main\n"
				" - commit-id: 43779f75cb730f56f4a8a4cc99ee75e39f363999\n"
				" - date-YmdHM: 2024-05-23 06:30\n"
				"REPORT:\n"
				" - report-id: af2a93f2c862ad474a55ae5a553d521268f1b43e\n"
				" - base.present: false\n"
				" - base.id: 0000000000000000000000000000000000000000\n"
				" - base.label: 000000000\n"
				" - base.is-tag: false\n"
				" - base.is-branch: false\n"
				" - properties:\n"
				"   - arch: string = \"x86_64\";\n"
				"   - build_type: string = \"Debug\";\n"
				"   - compiler: string = \"clang\";\n"
				"   - compiler.libcxx: string = \"stdc++\";\n"
				"   - compiler.version: string = \"18.1.6\";\n"
				"   - host.name: string = \"build-machine\";\n"
				"   - os: string = \"ubuntu\";\n"
				"   - os.version: string = \"22.04\";\n"
				"   - sanitizer: flag = off;\n"
				" - date-YmdHM: 2024-05-23 06:32"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "af2a93f2c862ad474a55ae5a553d521268f1b43e"sv,
	            },
	    },
	    {
	        .tmplt = build_info_template,
	        .expected =
	            // clang-format off
	            "COMMIT:\n"
				" - subject: Not so initial\n"
				" - description: A more description, for the heck of it.\n"
				"Now with more line breaks!\n"
				"\n"
				"Heck yeah!\n"
				" - different_author: false\n"
				" - author: Johnny Appleseed <johnny@appleseed.com> (8371e7982b4c56cfa544d08389fb1fb5)\n"
				" - committer: Johnny Appleseed <johnny@appleseed.com> (8371e7982b4c56cfa544d08389fb1fb5)\n"
				" - branch: main\n"
				" - commit-id: 5363ffcf0c00579c384f4916d92c260f0d7f53d3\n"
				" - date-YmdHM: 2024-05-24 15:41\n"
				"REPORT:\n"
				" - report-id: c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c\n"
				" - base.present: true\n"
				" - base.id: af2a93f2c862ad474a55ae5a553d521268f1b43e\n"
				" - base.label: before\n"
				" - base.is-tag: true\n"
				" - base.is-branch: false\n"
				" - properties:\n"
				"   - arch: string = \"x86_64\";\n"
				"   - build_type: string = \"Debug\";\n"
				"   - compiler: string = \"gcc\";\n"
				"   - compiler.libcxx: string = \"stdc++\";\n"
				"   - compiler.version: string = \"13.1.0\";\n"
				"   - host.name: string = \"build-machine\";\n"
				"   - os: string = \"ubuntu\";\n"
				"   - os.version: string = \"22.04\";\n"
				"   - sanitizer: flag = off;\n"
				" - date-YmdHM: 2024-05-24 16:05"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .from = "af2a93f2c862ad474a55ae5a553d521268f1b43e"sv,
	            },
	    },
	    {
	        .tmplt = build_info_template,
	        .expected =
	            // clang-format off
	            "COMMIT:\n"
				" - subject: Initial\n"
				" - description: \n"
				" - different_author: false\n"
				" - author: Johnny Appleseed <johnny@appleseed.com> (8371e7982b4c56cfa544d08389fb1fb5)\n"
				" - committer: Johnny Appleseed <johnny@appleseed.com> (8371e7982b4c56cfa544d08389fb1fb5)\n"
				" - branch: main\n"
				" - commit-id: 43779f75cb730f56f4a8a4cc99ee75e39f363999\n"
				" - date-YmdHM: 2024-05-23 06:30\n"
				"REPORT:\n"
				" - report-id: af2a93f2c862ad474a55ae5a553d521268f1b43e\n"
				" - base.present: true\n"
				" - base.id: c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c\n"
				" - base.label: main\n"
				" - base.is-tag: false\n"
				" - base.is-branch: true\n"
				" - properties:\n"
				"   - arch: string = \"x86_64\";\n"
				"   - build_type: string = \"Debug\";\n"
				"   - compiler: string = \"clang\";\n"
				"   - compiler.libcxx: string = \"stdc++\";\n"
				"   - compiler.version: string = \"18.1.6\";\n"
				"   - host.name: string = \"build-machine\";\n"
				"   - os: string = \"ubuntu\";\n"
				"   - os.version: string = \"22.04\";\n"
				"   - sanitizer: flag = off;\n"
				" - date-YmdHM: 2024-05-23 06:32"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "af2a93f2c862ad474a55ae5a553d521268f1b43e"sv,
	                .from = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	            },
	    },
	    {
	        .tmplt = build_info_template,
	        .expected = ""sv,
	        .context = {.oid = "9dd1933e47eda7b6613d9bfcd35ca7b3eec3d162"sv},
	    },
	    {
	        .tmplt = build_info_template,
	        .expected = ""sv,
	        .context = {.oid = "f23d04a9dd54870bf016b8ec1b7614e8f38b95da"sv},
	    },
	};

	INSTANTIATE_TEST_SUITE_P(build_info,
	                         components,
	                         ConvertGenerator(ValuesIn(build_info_tests)));

	static constexpr wrapper::component_test<
	    wrapper::file_source> const file_source_tests[] = {
	    {
	        .tmplt = file_source_template,
	        .expected =
	            // clang-format off
	            "- file-is-present: true\n"
				"- last-line: 12\n"
				"- file-is-empty: false\n"
				"- file-chunks:\n"
				"  - missing-before: true\n"
				"  - missing-after: false\n"
				"  - lines:\n"
				"    - 3 |   | none | \n"
				"    - 4 |   | none | <span class=\"hl-meta\"><span class=\"hl-keyword\">import</span> <span class=\"hl-identifier\"><span class=\"hl-module-name\">greetings</span></span><span class=\"hl-punctuator\">;</span></span>\n"
				"    - 5 |   | none | \n"
				"    - 6 | 0 | failing | <span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">main</span><span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">,</span> <span class=\"hl-keyword\">char</span> <span class=\"hl-punctuator\">*</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 0 | failing | main\n"
				"    - 7 | 0 | failing |   <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">vector</span><span class=\"hl-punctuator\">&lt;</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">string</span><span class=\"hl-punctuator\">&gt;</span> <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">{</span><span class=\"hl-punctuator\">}</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 8 | 0 | failing |   <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">reserve</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 9 | 0 | failing |   <span class=\"hl-keyword\">for</span> <span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">=</span> <span class=\"hl-number\">0</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">&lt;</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-punctuator\">++</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">)</span>\n"
				"    - 10 | 0 | failing |     <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">push_back</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 11 | 0 | failing | \n"
				"    - 12 | 0 | failing |   <span class=\"hl-identifier\">print_names</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 13 | 0 | failing | <span class=\"hl-punctuator\">}</span>"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "af2a93f2c862ad474a55ae5a553d521268f1b43e"sv,
	                .path = "src/main.cpp"sv,
	            },
	    },
	    {
	        .tmplt = file_source_template,
	        .expected =
	            // clang-format off
	            "- file-is-present: true\n"
				"- last-line: 12\n"
				"- file-is-empty: false\n"
				"- file-chunks:\n"
				"  - missing-before: true\n"
				"  - missing-after: false\n"
				"  - lines:\n"
				"    - 3 |   | none | \n"
				"    - 4 |   | none | <span class=\"hl-meta\"><span class=\"hl-keyword\">import</span> <span class=\"hl-identifier\"><span class=\"hl-module-name\">greetings</span></span><span class=\"hl-punctuator\">;</span></span>\n"
				"    - 5 |   | none | \n"
				"    - 6 | 0 | failing | <span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">main</span><span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">,</span> <span class=\"hl-keyword\">char</span> <span class=\"hl-punctuator\">*</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 0 | failing | main\n"
				"    - 7 | 0 | failing |   <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">vector</span><span class=\"hl-punctuator\">&lt;</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">string</span><span class=\"hl-punctuator\">&gt;</span> <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">{</span><span class=\"hl-punctuator\">}</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 8 | 0 | failing |   <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">reserve</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 9 | 0 | failing |   <span class=\"hl-keyword\">for</span> <span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">=</span> <span class=\"hl-number\">0</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">&lt;</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-punctuator\">++</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">)</span>\n"
				"    - 10 | 0 | failing |     <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">push_back</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 11 | 0 | failing | \n"
				"    - 12 | 0 | failing |   <span class=\"hl-identifier\">print_names</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 13 | 0 | failing | <span class=\"hl-punctuator\">}</span>"sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "9dd1933e47eda7b6613d9bfcd35ca7b3eec3d162"sv,
	                .path = "src/main.cpp"sv,
	            },
	    },
	    {
	        .tmplt = file_source_template,
	        .expected =
	            // clang-format off
	            "- file-is-present: true\n"
				"- last-line: 12\n"
				"- file-is-empty: false\n"
				"- file-chunks:\n"
				"  - missing-before: true\n"
				"  - missing-after: false\n"
				"  - lines:\n"
				"    - 3 |   | none | \n"
				"    - 4 |   | none | <span class=\"hl-meta\"><span class=\"hl-keyword\">import</span> <span class=\"hl-identifier\"><span class=\"hl-module-name\">greetings</span></span><span class=\"hl-punctuator\">;</span></span>\n"
				"    - 5 |   | none | \n"
				"    - 6 | 0 | failing | <span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">main</span><span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">,</span> <span class=\"hl-keyword\">char</span> <span class=\"hl-punctuator\">*</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 0 | failing | main\n"
				"    - 7 | 0 | failing |   <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">vector</span><span class=\"hl-punctuator\">&lt;</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">string</span><span class=\"hl-punctuator\">&gt;</span> <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">{</span><span class=\"hl-punctuator\">}</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 8 | 0 | failing |   <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">reserve</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 9 | 0 | failing |   <span class=\"hl-keyword\">for</span> <span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">=</span> <span class=\"hl-number\">0</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">&lt;</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-punctuator\">++</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">)</span>\n"
				"    - 10 | 0 | failing |     <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">push_back</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 11 | 0 | failing | \n"
				"    - 12 | 0 | failing |   <span class=\"hl-identifier\">print_names</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 13 | 0 | failing | <span class=\"hl-punctuator\">}</span>"sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "f23d04a9dd54870bf016b8ec1b7614e8f38b95da"sv,
	                .path = "src/main.cpp"sv,
	            },
	    },
	    {
	        .tmplt = file_source_template,
	        .expected =
	            // clang-format off
	            "- file-is-present: true\n"
				"- last-line: 13\n"
				"- file-is-empty: false\n"
				"- file-chunks:\n"
				"  - missing-before: true\n"
				"  - missing-after: false\n"
				"  - lines:\n"
				"    - 6 |   | none | \n"
				"    - 7 |   | none | <span class=\"hl-meta\"><span class=\"hl-keyword\">export</span> <span class=\"hl-keyword\">module</span> <span class=\"hl-identifier\"><span class=\"hl-module-name\">greetings</span></span><span class=\"hl-punctuator\">;</span></span>\n"
				"    - 8 |   | none | \n"
				"    - 9 | 0 | failing | <span class=\"hl-keyword\">export</span> <span class=\"hl-keyword\">void</span> <span class=\"hl-identifier\">print_names</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">vector</span><span class=\"hl-punctuator\">&lt;</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">string</span><span class=\"hl-punctuator\">&gt;</span> <span class=\"hl-keyword\">const</span> <span class=\"hl-punctuator\">&amp;</span><span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 0 | failing | print_names@greetings(std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>>> const&)\n"
				"    - 10 | 0 | failing |   <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-identifier\">printf</span><span class=\"hl-punctuator\">(</span><span class=\"hl-string\"><span class=\"hl-string-delim\">&quot;</span>names:<span class=\"hl-escape-sequence\">\\n"
				"</span><span class=\"hl-string-delim\">&quot;</span></span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 11 | 0 | failing |   <span class=\"hl-keyword\">for</span> <span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">auto</span> <span class=\"hl-keyword\">const</span> <span class=\"hl-punctuator\">&amp;</span><span class=\"hl-identifier\">name</span> <span class=\"hl-punctuator\">:</span> <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"    - 12 | 0 | failing |     <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-identifier\">printf</span><span class=\"hl-punctuator\">(</span><span class=\"hl-string\"><span class=\"hl-string-delim\">&quot;</span> - %s<span class=\"hl-escape-sequence\">\\n"
				"</span><span class=\"hl-string-delim\">&quot;</span></span><span class=\"hl-punctuator\">,</span> <span class=\"hl-identifier\">name</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">c_str</span><span class=\"hl-punctuator\">(</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 13 | 0 | failing |   <span class=\"hl-punctuator\">}</span>\n"
				"    - 14 | 0 | failing | <span class=\"hl-punctuator\">}</span>"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "af2a93f2c862ad474a55ae5a553d521268f1b43e"sv,
	                .path = "src/greetings.cpp"sv,
	            },
	    },
	    {
	        .tmplt = file_source_template,
	        .expected =
	            // clang-format off
	            "- file-is-present: true\n"
				"- last-line: 12\n"
				"- file-is-empty: false\n"
				"- file-chunks:\n"
				"  - missing-before: true\n"
				"  - missing-after: false\n"
				"  - lines:\n"
				"    - 3 |   | none | \n"
				"    - 4 |   | none | <span class=\"hl-meta\"><span class=\"hl-keyword\">import</span> <span class=\"hl-identifier\"><span class=\"hl-module-name\">greetings</span></span><span class=\"hl-punctuator\">;</span></span>\n"
				"    - 5 |   | none | \n"
				"    - 6 | 0 | failing | <span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">main</span><span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">,</span> <span class=\"hl-keyword\">char</span> <span class=\"hl-punctuator\">*</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 1 | passing | main\n"
				"    - 7 | 1 | passing |   <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">vector</span><span class=\"hl-punctuator\">&lt;</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">string</span><span class=\"hl-punctuator\">&gt;</span> <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">{</span><span class=\"hl-punctuator\">}</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 8 | 2 | passing |   <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">reserve</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 9 | 0 | failing |   <span class=\"hl-keyword\">for</span> <span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">int</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">=</span> <span class=\"hl-number\">0</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-identifier\">index</span> <span class=\"hl-punctuator\">&lt;</span> <span class=\"hl-identifier\">argc</span><span class=\"hl-punctuator\">;</span> <span class=\"hl-punctuator\">++</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">)</span>\n"
				"    - 10 | 1 | passing |     <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">push_back</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">argv</span><span class=\"hl-punctuator\">[</span><span class=\"hl-identifier\">index</span><span class=\"hl-punctuator\">]</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 11 | 2 | passing | \n"
				"    - 12 | 0 | failing |   <span class=\"hl-identifier\">print_names</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 13 | 1 | passing | <span class=\"hl-punctuator\">}</span>"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .path = "src/main.cpp"sv,
	            },
	    },
	    {
	        .tmplt = file_source_template,
	        .expected =
	            // clang-format off
	            "- file-is-present: true\n"
				"- last-line: 19\n"
				"- file-is-empty: false\n"
				"- file-chunks:\n"
				"  - missing-before: true\n"
				"  - missing-after: true\n"
				"  - lines:\n"
				"    - 6 |   | none | \n"
				"    - 7 |   | none | <span class=\"hl-meta\"><span class=\"hl-keyword\">export</span> <span class=\"hl-keyword\">module</span> <span class=\"hl-identifier\"><span class=\"hl-module-name\">greetings</span></span><span class=\"hl-punctuator\">;</span></span>\n"
				"    - 8 |   | none | \n"
				"    - 9 | 0 | failing | <span class=\"hl-keyword\">export</span> <span class=\"hl-keyword\">void</span> <span class=\"hl-identifier\">print_names</span><span class=\"hl-punctuator\">(</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">vector</span><span class=\"hl-punctuator\">&lt;</span><span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-known-ident-3\">string</span><span class=\"hl-punctuator\">&gt;</span> <span class=\"hl-keyword\">const</span> <span class=\"hl-punctuator\">&amp;</span><span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 1 | passing | print_names@greetings(std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>>> const&)\n"
				"    - 10 | 1 | passing |   <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-identifier\">printf</span><span class=\"hl-punctuator\">(</span><span class=\"hl-string\"><span class=\"hl-string-delim\">&quot;</span>names:<span class=\"hl-escape-sequence\">\\n"
				"</span><span class=\"hl-string-delim\">&quot;</span></span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 11 | 3 | passing |   <span class=\"hl-keyword\">for</span> <span class=\"hl-punctuator\">(</span><span class=\"hl-keyword\">auto</span> <span class=\"hl-keyword\">const</span> <span class=\"hl-punctuator\">&amp;</span><span class=\"hl-identifier\">name</span> <span class=\"hl-punctuator\">:</span> <span class=\"hl-identifier\">names</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"    - 12 | 4 | passing |     <span class=\"hl-identifier\">std</span><span class=\"hl-punctuator\">::</span><span class=\"hl-identifier\">printf</span><span class=\"hl-punctuator\">(</span><span class=\"hl-string\"><span class=\"hl-string-delim\">&quot;</span> - %s<span class=\"hl-escape-sequence\">\\n"
				"</span><span class=\"hl-string-delim\">&quot;</span></span><span class=\"hl-punctuator\">,</span> <span class=\"hl-identifier\">name</span><span class=\"hl-punctuator\">.</span><span class=\"hl-identifier\">c_str</span><span class=\"hl-punctuator\">(</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"  - missing-before: false\n"
				"  - missing-after: false\n"
				"  - lines:\n"
				"    - 17 |   | none | \n"
				"    - 18 | 1 | passing | <span class=\"hl-keyword\">export</span> <span class=\"hl-keyword\">void</span> <span class=\"hl-identifier\">foo</span><span class=\"hl-punctuator\">(</span><span class=\"hl-punctuator\">)</span> <span class=\"hl-punctuator\">{</span>\n"
				"      - 1 | passing | foo@greetings()\n"
				"    - 19 | 1 | passing |   <span class=\"hl-identifier\">bar</span><span class=\"hl-punctuator\">(</span><span class=\"hl-punctuator\">)</span><span class=\"hl-punctuator\">;</span>\n"
				"    - 20 | 0 | failing | <span class=\"hl-punctuator\">}</span>"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .oid = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .path = "src/greetings.cpp"sv,
	            },
	    },
	    {
	        .tmplt = file_source_template,
	        .expected = "- file-is-present: false"sv,
	        .context =
	            {
	                .oid = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .path = "src/404.cpp"sv,
	            },
	    },
	};

	INSTANTIATE_TEST_SUITE_P(file_source,
	                         components,
	                         ConvertGenerator(ValuesIn(file_source_tests)));

	static constexpr std::string_view files[] = {
	    "src/component1/file1.cc"sv, "src/component1/file2.cc"sv,
	    "src/component1/file3.cc"sv, "src/component1/file4.cc"sv,
	    "src/component2/file1.cc"sv, "src/component2/file2.cc"sv,
	    "src/component2/file3.cc"sv, "src/component2/file4.cc"sv,
	    "src/component3/file1.cc"sv, "src/component3/file2.cc"sv,
	    "src/component3/file3.cc"sv, "src/component3/file4.cc"sv,
	};

	static constexpr auto dot_cov_modules = R"([module "src"]
	path = src
[module "src/c1"]
	path = src/component1
[module "src/c2"]
	path = src/component2
)"sv;

	static constexpr wrapper::component_test<
	    wrapper::page_info> const page_info_tests[] = {
	    {
	        .tmplt = page_info_template,
	        .expected =
	            // clang-format off
	            "- is-standalone: false\n"
				"- json: {\n"
				"    \"breadcrumbs\": {\"heading\": {\"display\": \"repository\", \"href\": \"../../../index.html\"}},\n"
				"    \"build\": {\n"
				"        \"commit\": [],\n"
				"        \"is-failing\": true,\n"
				"        \"is-incomplete\": false,\n"
				"        \"is-passing\": false,\n"
				"        \"mark\": \"failing\",\n"
				"        \"report\": []\n"
				"    },\n"
				"    \"has-copy-path\": false,\n"
				"    \"is-directory\": false,\n"
				"    \"is-file\": false,\n"
				"    \"is-module\": false,\n"
				"    \"is-root\": true,\n"
				"    \"octicons\": {\n"
				"        \"lngs\": {\n"
				"            \"LNG_COVERAGE_BRANDING\": \"COVERAGE_BRANDING\",\n"
				"            \"LNG_COVERAGE_BUILDS\": \"COVERAGE_BUILDS\",\n"
				"            \"LNG_COMMIT_AUTHOR\": \"COMMIT_AUTHOR\",\n"
				"            \"LNG_COMMIT_COMMITTER\": \"COMMIT_COMMITTER\",\n"
				"            \"LNG_COMMIT_ID\": \"COMMIT_ID\",\n"
				"            \"LNG_COMMIT_BRANCH\": \"COMMIT_BRANCH\",\n"
				"            \"LNG_FILE_LINES\": \"FILE_LINES\",\n"
				"            \"LNG_FILE_NAME\": \"FILE_NAME\",\n"
				"            \"LNG_FILE_COVERAGE\": \"FILE_COVERAGE\",\n"
				"            \"LNG_FILE_LINES_TOTAL\": \"FILE_LINES_TOTAL\",\n"
				"            \"LNG_FILE_RELEVANT\": \"FILE_RELEVANT\",\n"
				"            \"LNG_FILE_COVERED\": \"FILE_COVERED\",\n"
				"            \"LNG_FILE_MISSED\": \"FILE_MISSED\",\n"
				"            \"LNG_ROW_TOTAL\": \"ROW_TOTAL\"\n"
				"        },\n"
				"        \"long-long\": 5,\n"
				"        \"none\": null,\n"
				"        \"number\": 3.14\n"
				"    },\n"
				"    \"site-root\": \"../../../\",\n"
				"    \"title\": \"repository\"\n"
				"}"
				""sv,
	        // clang-format on
	        .context =
	            {

	            },
	    },
	    {
	        .tmplt = page_info_template,
	        .expected =
	            // clang-format off
	            "- is-standalone: false\n"
				"- json: {\n"
				"    \"breadcrumbs\": {\n"
				"        \"heading\": {\"display\": \"repository\", \"href\": \"../../../index.html\"},\n"
				"        \"leaf\": \"component1\",\n"
				"        \"nav\": [{\"display\": \"src\", \"href\": \"../../../dirs/src.html\"}]\n"
				"    },\n"
				"    \"build\": {\n"
				"        \"commit\": [],\n"
				"        \"is-failing\": true,\n"
				"        \"is-incomplete\": false,\n"
				"        \"is-passing\": false,\n"
				"        \"mark\": \"failing\",\n"
				"        \"report\": []\n"
				"    },\n"
				"    \"has-copy-path\": false,\n"
				"    \"is-branches-covered\": false,\n"
				"    \"is-branches-missing\": false,\n"
				"    \"is-branches-relevant\": false,\n"
				"    \"is-directory\": true,\n"
				"    \"is-file\": false,\n"
				"    \"is-functions-covered\": false,\n"
				"    \"is-functions-missing\": false,\n"
				"    \"is-functions-relevant\": false,\n"
				"    \"is-lines-covered\": false,\n"
				"    \"is-lines-missing\": false,\n"
				"    \"is-lines-relevant\": false,\n"
				"    \"is-lines-total\": true,\n"
				"    \"is-lines-visited\": false,\n"
				"    \"is-module\": false,\n"
				"    \"is-name\": false,\n"
				"    \"is-root\": false,\n"
				"    \"octicons\": {\n"
				"        \"lngs\": {\n"
				"            \"LNG_COVERAGE_BRANDING\": \"COVERAGE_BRANDING\",\n"
				"            \"LNG_COVERAGE_BUILDS\": \"COVERAGE_BUILDS\",\n"
				"            \"LNG_COMMIT_AUTHOR\": \"COMMIT_AUTHOR\",\n"
				"            \"LNG_COMMIT_COMMITTER\": \"COMMIT_COMMITTER\",\n"
				"            \"LNG_COMMIT_ID\": \"COMMIT_ID\",\n"
				"            \"LNG_COMMIT_BRANCH\": \"COMMIT_BRANCH\",\n"
				"            \"LNG_FILE_LINES\": \"FILE_LINES\",\n"
				"            \"LNG_FILE_NAME\": \"FILE_NAME\",\n"
				"            \"LNG_FILE_COVERAGE\": \"FILE_COVERAGE\",\n"
				"            \"LNG_FILE_LINES_TOTAL\": \"FILE_LINES_TOTAL\",\n"
				"            \"LNG_FILE_RELEVANT\": \"FILE_RELEVANT\",\n"
				"            \"LNG_FILE_COVERED\": \"FILE_COVERED\",\n"
				"            \"LNG_FILE_MISSED\": \"FILE_MISSED\",\n"
				"            \"LNG_ROW_TOTAL\": \"ROW_TOTAL\"\n"
				"        },\n"
				"        \"long-long\": 5,\n"
				"        \"none\": null,\n"
				"        \"number\": 3.14\n"
				"    },\n"
				"    \"site-root\": \"../../../\",\n"
				"    \"stats\": {\n"
				"        \"columns\": [\n"
				"            {\"display\": \"Name\", \"is-name\": true},\n"
				"            {\"display\": \"Lines\", \"priority\": \"high\"},\n"
				"            {\"display\": \"Visited\", \"priority\": \"key\"},\n"
				"            {\"display\": \"Relevant\", \"priority\": \"supplemental\"},\n"
				"            {\"display\": \"Line count\", \"priority\": \"supplemental\"}\n"
				"        ],\n"
				"        \"rows\": [\n"
				"            {\n"
				"                \"cells\": [\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff passing-bg priority-high\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"100.00%\",\n"
				"                        \"value-classes\": \"value bold passing passing-bg priority-high\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-key\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-key\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    }\n"
				"                ],\n"
				"                \"class-name\": \"file\",\n"
				"                \"display\": \"file1.cc\",\n"
				"                \"has-href\": true,\n"
				"                \"has-prefix\": false,\n"
				"                \"href\": \"../../../files/src/component1/file1.cc.html\",\n"
				"                \"is-directory\": false,\n"
				"                \"is-file\": true,\n"
				"                \"is-module\": false,\n"
				"                \"is-total\": false\n"
				"            },\n"
				"            {\n"
				"                \"cells\": [\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff passing-bg priority-high\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"100.00%\",\n"
				"                        \"value-classes\": \"value bold passing passing-bg priority-high\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-key\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-key\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    }\n"
				"                ],\n"
				"                \"class-name\": \"file\",\n"
				"                \"display\": \"file2.cc\",\n"
				"                \"has-href\": true,\n"
				"                \"has-prefix\": false,\n"
				"                \"href\": \"../../../files/src/component1/file2.cc.html\",\n"
				"                \"is-directory\": false,\n"
				"                \"is-file\": true,\n"
				"                \"is-module\": false,\n"
				"                \"is-total\": false\n"
				"            },\n"
				"            {\n"
				"                \"cells\": [\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff passing-bg priority-high\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"100.00%\",\n"
				"                        \"value-classes\": \"value bold passing passing-bg priority-high\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-key\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-key\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    }\n"
				"                ],\n"
				"                \"class-name\": \"file\",\n"
				"                \"display\": \"file3.cc\",\n"
				"                \"has-href\": true,\n"
				"                \"has-prefix\": false,\n"
				"                \"href\": \"../../../files/src/component1/file3.cc.html\",\n"
				"                \"is-directory\": false,\n"
				"                \"is-file\": true,\n"
				"                \"is-module\": false,\n"
				"                \"is-total\": false\n"
				"            },\n"
				"            {\n"
				"                \"cells\": [\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff passing-bg priority-high\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"100.00%\",\n"
				"                        \"value-classes\": \"value bold passing passing-bg priority-high\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-key\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-key\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    },\n"
				"                    {\n"
				"                        \"change\": \"\xC2\xA0\",\n"
				"                        \"change-classes\": \"diff priority-supplemental\",\n"
				"                        \"is-simple\": false,\n"
				"                        \"value\": \"0\",\n"
				"                        \"value-classes\": \"value priority-supplemental\"\n"
				"                    }\n"
				"                ],\n"
				"                \"class-name\": \"file\",\n"
				"                \"display\": \"file4.cc\",\n"
				"                \"has-href\": true,\n"
				"                \"has-prefix\": false,\n"
				"                \"href\": \"../../../files/src/component1/file4.cc.html\",\n"
				"                \"is-directory\": false,\n"
				"                \"is-file\": true,\n"
				"                \"is-module\": false,\n"
				"                \"is-total\": false\n"
				"            }\n"
				"        ],\n"
				"        \"total\": {\n"
				"            \"cells\": [\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff passing-bg priority-high\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"100.00%\",\n"
				"                    \"value-classes\": \"value bold passing passing-bg priority-high\"\n"
				"                },\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff priority-key\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"0\",\n"
				"                    \"value-classes\": \"value priority-key\"\n"
				"                },\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff priority-supplemental\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"0\",\n"
				"                    \"value-classes\": \"value priority-supplemental\"\n"
				"                },\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff priority-supplemental\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"0\",\n"
				"                    \"value-classes\": \"value priority-supplemental\"\n"
				"                }\n"
				"            ],\n"
				"            \"class-name\": \"total\",\n"
				"            \"display\": \"Total\",\n"
				"            \"has-href\": false,\n"
				"            \"has-prefix\": false,\n"
				"            \"is-directory\": false,\n"
				"            \"is-file\": false,\n"
				"            \"is-module\": false,\n"
				"            \"is-total\": true\n"
				"        }\n"
				"    },\n"
				"    \"title\": \"src/component1\"\n"
				"}"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .ref = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .fname_filter = "src/component1",
	                .modules = dot_cov_modules,
	                .files = span(files),
	            },
	    },
	    {
	        .tmplt = page_info_template,
	        .expected =
	            // clang-format off
				"- is-standalone: false\n"
				"- json: {\n"
				"    \"breadcrumbs\": {\n"
				"        \"heading\": {\"display\": \"repository\", \"href\": \"../../../index.html\"},\n"
				"        \"leaf\": \"c1\",\n"
				"        \"nav\": [{\"display\": \"src\", \"href\": \"../../../mods/src.html\"}]\n"
				"    },\n"
				"    \"build\": {\n"
				"        \"commit\": [],\n"
				"        \"is-failing\": true,\n"
				"        \"is-incomplete\": false,\n"
				"        \"is-passing\": false,\n"
				"        \"mark\": \"failing\",\n"
				"        \"report\": []\n"
				"    },\n"
				"    \"has-copy-path\": false,\n"
				"    \"is-directory\": false,\n"
				"    \"is-file\": false,\n"
				"    \"is-module\": true,\n"
				"    \"is-root\": false,\n"
				"    \"octicons\": {\n"
				"        \"lngs\": {\n"
				"            \"LNG_COVERAGE_BRANDING\": \"COVERAGE_BRANDING\",\n"
				"            \"LNG_COVERAGE_BUILDS\": \"COVERAGE_BUILDS\",\n"
				"            \"LNG_COMMIT_AUTHOR\": \"COMMIT_AUTHOR\",\n"
				"            \"LNG_COMMIT_COMMITTER\": \"COMMIT_COMMITTER\",\n"
				"            \"LNG_COMMIT_ID\": \"COMMIT_ID\",\n"
				"            \"LNG_COMMIT_BRANCH\": \"COMMIT_BRANCH\",\n"
				"            \"LNG_FILE_LINES\": \"FILE_LINES\",\n"
				"            \"LNG_FILE_NAME\": \"FILE_NAME\",\n"
				"            \"LNG_FILE_COVERAGE\": \"FILE_COVERAGE\",\n"
				"            \"LNG_FILE_LINES_TOTAL\": \"FILE_LINES_TOTAL\",\n"
				"            \"LNG_FILE_RELEVANT\": \"FILE_RELEVANT\",\n"
				"            \"LNG_FILE_COVERED\": \"FILE_COVERED\",\n"
				"            \"LNG_FILE_MISSED\": \"FILE_MISSED\",\n"
				"            \"LNG_ROW_TOTAL\": \"ROW_TOTAL\"\n"
				"        },\n"
				"        \"long-long\": 5,\n"
				"        \"none\": null,\n"
				"        \"number\": 3.14\n"
				"    },\n"
				"    \"site-root\": \"../../../\",\n"
				"    \"title\": \"[src/c1]\"\n"
				"}"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .ref = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .module_filter = "src/c1",
	                .modules = dot_cov_modules,
	                .files = span(files),
	            },
	    },
	    {
	        .tmplt = page_info_template,
	        .expected =
	            // clang-format off
				"- is-standalone: true\n"
				"- json: {\n"
				"    \"breadcrumbs\": {\n"
				"        \"heading\": {\"display\": \"repository\", \"href\": \"../../../index.html\"},\n"
				"        \"leaf\": \"file4.cc\",\n"
				"        \"nav\": [\n"
				"            {\"display\": \"src\", \"href\": \"../../../dirs/src.html\"},\n"
				"            {\"display\": \"component3\", \"href\": \"../../../dirs/src/component3.html\"}\n"
				"        ]\n"
				"    },\n"
				"    \"build\": {\n"
				"        \"commit\": [],\n"
				"        \"is-failing\": true,\n"
				"        \"is-incomplete\": false,\n"
				"        \"is-passing\": false,\n"
				"        \"mark\": \"failing\",\n"
				"        \"report\": []\n"
				"    },\n"
				"    \"copy-path\": \"src/component3/file4.cc\",\n"
				"    \"file-is-present\": false,\n"
				"    \"has-copy-path\": true,\n"
				"    \"is-branches-covered\": false,\n"
				"    \"is-branches-missing\": false,\n"
				"    \"is-branches-relevant\": false,\n"
				"    \"is-directory\": false,\n"
				"    \"is-file\": true,\n"
				"    \"is-functions-covered\": false,\n"
				"    \"is-functions-missing\": false,\n"
				"    \"is-functions-relevant\": false,\n"
				"    \"is-lines-covered\": false,\n"
				"    \"is-lines-missing\": false,\n"
				"    \"is-lines-relevant\": false,\n"
				"    \"is-lines-total\": true,\n"
				"    \"is-lines-visited\": false,\n"
				"    \"is-module\": false,\n"
				"    \"is-name\": false,\n"
				"    \"is-root\": false,\n"
				"    \"octicons\": {\n"
				"        \"lngs\": {\n"
				"            \"LNG_COVERAGE_BRANDING\": \"COVERAGE_BRANDING\",\n"
				"            \"LNG_COVERAGE_BUILDS\": \"COVERAGE_BUILDS\",\n"
				"            \"LNG_COMMIT_AUTHOR\": \"COMMIT_AUTHOR\",\n"
				"            \"LNG_COMMIT_COMMITTER\": \"COMMIT_COMMITTER\",\n"
				"            \"LNG_COMMIT_ID\": \"COMMIT_ID\",\n"
				"            \"LNG_COMMIT_BRANCH\": \"COMMIT_BRANCH\",\n"
				"            \"LNG_FILE_LINES\": \"FILE_LINES\",\n"
				"            \"LNG_FILE_NAME\": \"FILE_NAME\",\n"
				"            \"LNG_FILE_COVERAGE\": \"FILE_COVERAGE\",\n"
				"            \"LNG_FILE_LINES_TOTAL\": \"FILE_LINES_TOTAL\",\n"
				"            \"LNG_FILE_RELEVANT\": \"FILE_RELEVANT\",\n"
				"            \"LNG_FILE_COVERED\": \"FILE_COVERED\",\n"
				"            \"LNG_FILE_MISSED\": \"FILE_MISSED\",\n"
				"            \"LNG_ROW_TOTAL\": \"ROW_TOTAL\"\n"
				"        },\n"
				"        \"long-long\": 5,\n"
				"        \"none\": null,\n"
				"        \"number\": 3.14\n"
				"    },\n"
				"    \"site-root\": \"../../../\",\n"
				"    \"stats\": {\n"
				"        \"columns\": [\n"
				"            {\"display\": \"Name\", \"is-name\": true},\n"
				"            {\"display\": \"Lines\", \"priority\": \"high\"},\n"
				"            {\"display\": \"Visited\", \"priority\": \"key\"},\n"
				"            {\"display\": \"Relevant\", \"priority\": \"supplemental\"},\n"
				"            {\"display\": \"Line count\", \"priority\": \"supplemental\"}\n"
				"        ],\n"
				"        \"rows\": [{\n"
				"            \"cells\": [\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff passing-bg priority-high\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"100.00%\",\n"
				"                    \"value-classes\": \"value bold passing passing-bg priority-high\"\n"
				"                },\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff priority-key\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"0\",\n"
				"                    \"value-classes\": \"value priority-key\"\n"
				"                },\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff priority-supplemental\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"0\",\n"
				"                    \"value-classes\": \"value priority-supplemental\"\n"
				"                },\n"
				"                {\n"
				"                    \"change\": \"\xC2\xA0\",\n"
				"                    \"change-classes\": \"diff priority-supplemental\",\n"
				"                    \"is-simple\": false,\n"
				"                    \"value\": \"0\",\n"
				"                    \"value-classes\": \"value priority-supplemental\"\n"
				"                }\n"
				"            ],\n"
				"            \"class-name\": \"file\",\n"
				"            \"display\": \"file4.cc\",\n"
				"            \"has-href\": false,\n"
				"            \"has-prefix\": false,\n"
				"            \"is-directory\": false,\n"
				"            \"is-file\": true,\n"
				"            \"is-module\": false,\n"
				"            \"is-total\": false\n"
				"        }],\n"
				"        \"total\": false\n"
				"    },\n"
				"    \"title\": \"src/component3/file4.cc\"\n"
				"}"
				""sv,
	        // clang-format on
	        .context =
	            {
	                .ref = "c924e9d5ee8655b8d2af9f0c4b5ca3458ca9361c"sv,
	                .fname_filter = "src/component3/file4.cc",
	                .modules = dot_cov_modules,
	                .files = span(files),
	            },
	    },
	};

	INSTANTIATE_TEST_SUITE_P(page_info,
	                         components,
	                         ConvertGenerator(ValuesIn(page_info_tests)));
}  // namespace cov::app::web::testing
