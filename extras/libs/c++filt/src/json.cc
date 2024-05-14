// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <c++filt/json.hh>
#include <c++filt/parser.hh>
#include <json/json.hpp>

namespace cxx_filt {
	namespace {
		void spread_map(json::map const& level, Replacements& result);
		void spread_array(json::array const& level, Replacements& result);

		void spread_replacement(std::string_view from,
		                        std::string_view to,
		                        Replacements& result) {
			auto stmt_from = Parser::statement_from(from);
			auto stmt_to = Parser::statement_from(to);
			if (stmt_from.items.size() != 1) {
				fmt::print(stderr,
				           "Expected to get exactly one expression from "
				           "'{}', got {}\n",
				           from, stmt_from.items.size());
				return;
			}
			if (stmt_to.items.size() != 1) {
				fmt::print(stderr,
				           "Expected to get exactly one expression from "
				           "'{}', got {}\n",
				           to, stmt_to.items.size());
				return;
			}
			result.push_back(
			    {std::move(stmt_from.items[0]), std::move(stmt_to.items[0])});
		}

		inline std::string_view from_u8(std::u8string_view u8) {
			return {reinterpret_cast<char const*>(u8.data()), u8.size()};
		}

		void spread_map(json::map const& level, Replacements& result) {
			for (auto const& [key, node] : level.items()) {
				auto map = json::cast<json::map>(node);
				auto array = json::cast<json::array>(node);
				auto string = json::cast<json::string>(node);
				if (map) spread_map(*map, result);
				if (array) spread_array(*array, result);
				if (string) {
					spread_replacement(from_u8(key), from_u8(*string), result);
				}
			}
		}

		void spread_array(json::array const& level, Replacements& result) {
			for (auto const& node : level) {
				auto map = json::cast<json::map>(node);
				auto array = json::cast<json::array>(node);
				if (map) spread_map(*map, result);
				if (array) spread_array(*array, result);
			}
		}
	}  // namespace

	void append_replacements(std::string_view json_text, Replacements& result) {
		auto const root =
		    json::read_json({reinterpret_cast<const char8_t*>(json_text.data()),
		                     json_text.size()});
		auto root_map = json::cast<json::map>(root);
		auto root_array = json::cast<json::array>(root);
		if (root_map) spread_map(*root_map, result);
		if (root_array) spread_array(*root_array, result);
	}
}  // namespace cxx_filt
