// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <charconv>
#include <chrono>
#include <cov/app/report.hh>
#include <json/json.hpp>

namespace cov::app::report {
	namespace {
		std::u8string_view to_json(std::string_view str) {
			return {reinterpret_cast<char8_t const*>(str.data()), str.size()};
		}
		std::string_view from_json(std::u8string_view str) {
			return {reinterpret_cast<char const*>(str.data()), str.size()};
		}

		bool conv(std::u8string_view key, unsigned& out) {
			auto const view = from_json(key);
			auto const first = view.data();
			auto const last = first + view.size();
			auto const [ptr, ec] = std::from_chars(first, last, out);
			return ptr == last && ec == std::errc{};
		}

		template <typename To, typename From>
		inline To unsigned_cast(From hits) {
			return static_cast<To>(hits);
		}

		template <typename To>
		inline To unsigned_cast(To hits) {
			return hits;
		}

		inline unsigned unsigned_from(long long hits) {
			if (hits < 0) return 0;
			static constexpr auto max_hits =
			    std::numeric_limits<unsigned>::max();
			auto const u_hits =
			    std::min(unsigned_cast<unsigned long long>(hits),
			             unsigned_cast<unsigned long long>(max_hits));
			return unsigned_cast<unsigned>(u_hits);
		}
	}  // namespace

	bool report_info::load_from_text(std::string_view u8_encoded) {
		git = {};
		files.clear();

		auto const node = json::read_json(to_json(u8_encoded));

		auto const json_git = cast_from_json<json::map>(node, u8"git"sv);
		auto const json_files = cast_from_json<json::array>(node, u8"files"sv);

		auto const json_git_branch =
		    cast_from_json<json::string>(json_git, u8"branch"sv);
		auto const json_git_head =
		    cast_from_json<json::string>(json_git, u8"head"sv);

		if (!json_git_branch || !json_git_head || !json_files) return false;

		files.reserve(json_files->size());

		for (auto const& file_node : *json_files) {
			auto const json_file_name =
			    cast_from_json<json::string>(file_node, u8"name"sv);
			auto const json_file_digest =
			    cast_from_json<json::string>(file_node, u8"digest"sv);
			auto const json_file_line_coverage =
			    cast_from_json<json::map>(file_node, u8"line_coverage"sv);

			if (!json_file_name || !json_file_digest ||
			    !json_file_line_coverage) {
				[[unlikely]];
				files.clear();
				return false;
			}

			auto digest = from_json(*json_file_digest);
			auto const pos = digest.find(':');
			if (pos == std::string_view::npos) {
				[[unlikely]];
				files.clear();
				return false;
			}

			files.push_back({});
			auto& dst = files.back();

			dst.name.assign(from_json(*json_file_name));
			dst.algorithm.assign(digest.substr(0, pos));
			dst.digest.assign(digest.substr(pos + 1));

			for (auto const& [node_key, node_value] :
			     *json_file_line_coverage) {
				auto const hits = cast<long long>(node_value);

				unsigned line{};
				if (!hits || !conv(node_key, line)) {
					[[unlikely]];
					files.clear();
					return false;
				}
				dst.line_coverage[line] = unsigned_from(*hits);
			}
		}

		git.branch.assign(from_json(*json_git_branch));
		git.head.assign(from_json(*json_git_head));
		return true;
	}
}  // namespace cov::app::report
