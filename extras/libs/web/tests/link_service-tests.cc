// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <string_view>
#include <tuple>
#include <variant>
#include <web/link_service.hh>
#include "setup.hh"

using namespace std::literals;

namespace cov::app::web::testing {
	using namespace projection;
	struct link_for {
		entry_type type;
		std::string_view name{};
	};

	struct resource_link {
		std::string_view link{};
	};

	struct prefix_info {
		std::string_view path{};
		std::string_view relative{};
		std::string_view site{};
	};

	using resource_info = std::variant<link_for, resource_link>;
	struct resource {
		resource_info info;
		struct {
			std::string_view path{};
			std::string_view site{};
		} expected;

		std::string visit_with(link_service const& links) const {
			struct visitor {
				link_service const& links;
				std::string operator()(link_for const& resource) const {
					label name{};
					name.expanded.assign(resource.name);
					return links.link_for(resource.type, name);
				}
				std::string operator()(resource_link const& resource) const {
					return links.resource_link(resource.link);
				}
			};

			return std::visit(visitor{links}, info);
		}
	};

	using link_test = std::tuple<prefix_info, resource>;
}  // namespace cov::app::web::testing

namespace std {
	void PrintTo(cov::app::web::testing::link_test const& test, ostream* outp) {
		auto& out = *outp;
		auto& [setup, resource] = test;

		out << '[' << setup.relative << " :: " << setup.site << "] / ";

		struct visitor {
			std::ostream& out;
			void operator()(
			    cov::app::web::testing::link_for const& resource) const {
				out << '(' << std::to_underlying(resource.type) << ") / "
				    << resource.name;
			}
			void operator()(
			    cov::app::web::testing::resource_link const& resource) const {
				out << resource.link;
			}
		};

		std::visit(visitor{out}, resource.info);
	}
};  // namespace std

namespace cov::app::web::testing {
	class links : public ::testing::TestWithParam<link_test> {};

	std::string join(std::string_view root, std::string_view path) {
		if (!root.empty() && root.back() == '/')
			root = root.substr(0, root.length() - 1);
		std::string result{};
		result.reserve(root.length() + path.length() + 1);
		result.append(root);
		result.push_back('/');
		result.append(path);
		return result;
	}

	TEST(links, root_export_link_for) {
		export_link_service links{};

		auto const expected = "/files/dir/subdir/file.txt.html"sv;
		auto const actual =
		    links.link_for(projection::entry_type::file,
		                   projection::label{
		                       .display = "file.txt"s,
		                       .expanded = "dir/subdir/file.txt"s,
		                   });

		ASSERT_EQ(expected, actual);
	}

	TEST(links, root_export_resource_link) {
		export_link_service links{};

		auto const expected = "/js/app.js"sv;
		auto const actual = links.resource_link("js/app.js");

		ASSERT_EQ(expected, actual);
	}

	TEST(links, export_link_for_standalone) {
		export_link_service links{};
		links.adjust_root("dir/subdir/file.html"sv);

		auto const expected = ""sv;
		auto const actual =
		    links.link_for(projection::entry_type::standalone_file,
		                   projection::label{
		                       .display = "file.txt"s,
		                       .expanded = "dir/subdir/file.txt"s,
		                   });

		ASSERT_EQ(expected, actual);
	}

	TEST(links, server_link_for_standalone) {
		server_link_service links{};
		links.set_app_path("/group/sub-group/app/"sv);

		auto const expected = ""sv;
		auto const actual =
		    links.link_for(projection::entry_type::standalone_file,
		                   projection::label{
		                       .display = "file.txt"s,
		                       .expanded = "dir/subdir/file.txt"s,
		                   });

		ASSERT_EQ(expected, actual);
	}

	TEST_P(links, export) {
		auto const& [setup, res] = GetParam();
		export_link_service links{};
		links.adjust_root(setup::make_u8path(setup.path));

		auto const expected = join(setup.relative, res.expected.path);
		auto const actual = res.visit_with(links);

		ASSERT_EQ(expected, actual);
	}

	TEST_P(links, server) {
		auto const& [setup, res] = GetParam();
		server_link_service links{};
		links.set_app_path(setup.site);

		auto const expected = join(setup.site, res.expected.site);
		auto const actual = res.visit_with(links);

		ASSERT_EQ(expected, actual);
	}

	static constexpr prefix_info prefixes[] = {
	    {.relative = "."sv},
	    {.path = "dir/subdir/file.html",
	     .relative = "../.."sv,
	     .site = "app/"sv},
	    {.path = "file.html", .relative = "."sv, .site = "/app/"sv},
	};

	static constexpr resource resources[] = {
	    {.info = resource_link{.link = {}}, .expected = {}},
	    {
	        .info = resource_link{.link = "styles/dark-theme.css"sv},
	        .expected = {.path = "styles/dark-theme.css"sv,
	                     .site = "styles/dark-theme.css"sv},
	    },
	    {
	        .info =
	            link_for{.type = entry_type::file, .name = "dir/file.ext"sv},
	        .expected = {.path = "files/dir/file.ext.html"sv,
	                     .site = "files/dir/file.ext"sv},
	    },
	    {
	        .info =
	            link_for{.type = entry_type::directory, .name = "dir/subdir"sv},
	        .expected = {.path = "dirs/dir/subdir.html"sv,
	                     .site = "dirs/dir/subdir"sv},
	    },
	    {
	        .info = link_for{.type = entry_type::module, .name = "mod/mod1"sv},
	        .expected = {.path = "mods/mod/mod1.html"sv,
	                     .site = "mods/mod/mod1"sv},
	    },
	    // {
	    //     .info = link_for{.type = entry_type::standalone_file,
	    //                      .name = "dir/file.ext"sv},
	    //     .expected = {.path = ""sv, .site = ""sv},
	    // },
	};

	INSTANTIATE_TEST_SUITE_P(
	    generator,
	    links,
	    ::testing::Combine(::testing::ValuesIn(prefixes),
	                       ::testing::ValuesIn(resources)));

	GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(links);
}  // namespace cov::app::web::testing
