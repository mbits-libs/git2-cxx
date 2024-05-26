// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cov/io/file.hh>
#include <fstream>
#include <iostream>
#include <json/json.hpp>
#include <native/path.hh>
#include <set>
#include <unordered_map>
#include <web/mstch_cache.hh>

// #define DEBUG_TEMPLATES 1

#ifdef DEBUG_TEMPLATES
#include "mstch/../../src/template_type.hpp"
#endif

using namespace std::literals;

namespace cov::app::web {
	namespace {
#ifdef DEBUG_TEMPLATES__
		const char* name(mstch::token::type t) {
			switch (t) {
#define CASE(v)                 \
	case mstch::token::type::v: \
		return #v;
				CASE(text);
				CASE(variable);
				CASE(section_open);
				CASE(section_close);
				CASE(inverted_section_open);
				CASE(unescaped_variable);
				CASE(comment);
				CASE(partial);
				CASE(delimiter_change);
#undef CASE
			}
			static thread_local char buf[100];
			sprintf(buf, "unknown(%d)", static_cast<int>(t));
			return buf;
		}
#endif

		std::string from_u8s(std::u8string_view view) {
			return {reinterpret_cast<char const*>(view.data()), view.size()};
		}

		std::tuple<std::filesystem::file_time_type,
		           std::filesystem::path,
		           std::error_code>
		find_partial(std::filesystem::path const& root,
		             std::string const& partial) {
			std::error_code ec1;
			auto path = root / (partial + ".mustache");
			auto time = std::filesystem::last_write_time(path, ec1);
			if (!ec1) return {time, std::move(path), ec1};

			std::error_code ec2;
			path = root / partial;
			time = std::filesystem::last_write_time(path, ec2);
			if (!ec2) return {time, std::move(path), ec2};

			return {std::filesystem::file_time_type::min(),
			        root / (partial + ".mustache"), ec1};
		}

		std::tuple<std::filesystem::file_time_type,
		           std::filesystem::path,
		           std::error_code>
		find_partial(std::vector<std::filesystem::path> const& roots,
		             std::filesystem::path const& res,
		             std::string const& partial) {
			std::vector<std::tuple<std::filesystem::file_time_type,
			                       std::filesystem::path, std::error_code>>
			    candidates{};
			candidates.reserve(roots.size());

			for (auto const& root : roots) {
				auto result = find_partial(root / res, partial);
				if (!std::get<2>(result)) candidates.push_back(result);
			}

			if (candidates.empty())
				return find_partial(roots.front() / res, partial);

			auto max_result = candidates.front();
			for (auto const& result : candidates) {
				if (std::get<0>(result) > std::get<0>(max_result))
					max_result = result;
			}
			return max_result;
		}
	}  // namespace

	std::pair<std::filesystem::file_time_type, std::string> dir_cache::load_ex(
	    std::string const& partial) {
		std::pair<std::filesystem::file_time_type, std::string> result;

		auto [last_write, path, ec] = find_partial(roots_, res_, partial);
#ifdef DEBUG_TEMPLATES
		fmt::print("\n{}: {}\n", partial, get_u8path(path));
#endif

		if (ec) return result;
		result.first = last_write;

		auto size_maxint = std::filesystem::file_size(path, ec);
		if (ec) return result;

		auto handle = io::fopen(path);
		if (!handle) return result;

		if constexpr (!std::is_same_v<decltype(size_maxint), size_t>) {
			if (size_maxint >= std::numeric_limits<size_t>::max())
				return result;
		}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
// It is not useless on systems, where size_t != unimax_t...
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

		result.second.reserve(static_cast<size_t>(size_maxint) + 1);

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

		char buffer[8192];

		while (auto read = handle.load(buffer, sizeof(buffer))) {
			result.second.append(buffer, read);
		}

		cache_[partial] = result.first;
#ifdef DEBUG_TEMPLATES
		mstch::template_type tmplt{result.second};
		std::set<std::string> partials;
		std::set<std::string> vars;
		std::string ctx;
		for (auto& tok : tmplt) {
			switch (tok.token_type()) {
				case mstch::token::type::unescaped_variable:
				case mstch::token::type::variable:
					if (ctx.empty())
						vars.insert(tok.name());
					else
						vars.insert(ctx + "/" + tok.name());
					break;

				case mstch::token::type::section_open:
				case mstch::token::type::inverted_section_open:
					if (!ctx.empty()) ctx.push_back('.');
					ctx += tok.name();
					break;

				case mstch::token::type::section_close: {
					auto pos = ctx.rfind('.');
					if (pos == std::string::npos)
						ctx.clear();
					else
						ctx = ctx.substr(0, pos);
					break;
				}
				case mstch::token::type::partial: {
					partials.insert(tok.name());
				}
				default:
					break;
			}
		}
		for (auto const& var : vars)
			std::cout << "    " << var << "\n";
		for (auto const& partial : partials)
			std::cout << "    > " << partial << "\n";
#endif
		return result;
	}
	std::string dir_cache::load(const std::string& partial) {
		return std::move(load_ex(partial)).second;
	}

	bool dir_cache::need_update(std::string const& partial) const {
		auto [last_write, ignore, ec] = find_partial(roots_, res_, partial);
		if (ec) return false;

		auto it = cache_.find(partial);
		if (it == cache_.end()) return true;

		return it->second < last_write;
	}

	bool dir_cache::is_valid(const std::string&) const { return true; }

	std::map<std::string_view, lng>& strings() {
		using namespace std::literals;
		static std::map<std::string_view, lng> keys = {
#define KNOWN_KEY(val) {"LNG_" #val##sv, lng::val},
		    // clang-format off
			KNOWN_KEY(COVERAGE_BRANDING)
			KNOWN_KEY(COVERAGE_BUILDS)
			KNOWN_KEY(COMMIT_AUTHOR)
			KNOWN_KEY(COMMIT_COMMITTER)
			KNOWN_KEY(COMMIT_ID)
			KNOWN_KEY(COMMIT_BRANCH)
			KNOWN_KEY(FILE_LINES)
			KNOWN_KEY(FILE_NAME)
			KNOWN_KEY(FILE_COVERAGE)
			KNOWN_KEY(FILE_LINES_TOTAL)
			KNOWN_KEY(FILE_RELEVANT)
			KNOWN_KEY(FILE_COVERED)
			KNOWN_KEY(FILE_MISSED)
			KNOWN_KEY(ROW_TOTAL)
#undef KNOWN_KEY
		    // clang-format on
		};
		return keys;
	}

	lng_provider::~lng_provider() = default;

	mstch::node const& lng_callback::at(std::string const& name) const {
		if (provider_) {
			auto const& keys = strings();
			auto it = keys.find(name);
			if (it != keys.end()) {
				auto it_stg = storage_.find(it->second);
				if (it_stg != storage_.end()) {
					return it_stg->second;
				}

				auto view = provider_->get(it->second);
				auto& ret = storage_[it->second];
				ret = std::string{view.data(), view.size()};
				return ret;
			}
		}

		auto& ret = str_storage_[name];
		ret = "\xC2\xAB" + name + "\xC2\xBB";
		return ret;
	}

	bool lng_callback::has(std::string const&) const { return true; }

	struct octicon {
		unsigned size{};
		std::string path{};
	};

	class octicons {
		std::unordered_map<std::string, octicon> icons_;

	public:
		bool has(std::string const& key) const {
			auto it = icons_.find(key);
			return it != icons_.end();
		}

		std::vector<std::string> debug_all_keys() const {
			std::vector<std::string> result{};
			result.reserve(icons_.size());
			for (auto const& [key, _] : icons_) {
				result.push_back(key);
			}
			std::sort(result.begin(), result.end());
			return result;
		}  // GCOV_EXCL_LINE[GCC]

		std::string build(std::string const& key) const {
			auto it = icons_.find(key);
			if (it == icons_.end()) return {};

			auto const& icon = it->second;
			auto const rem_size = 100 * icon.size / 16;
			auto const size =
			    fmt::format("{}.{:02}em", rem_size / 100, rem_size % 100);
			return fmt::format(
			    "<svg xmlns='http://www.w3.org/2000/svg'"
			    " class='octicon'"
			    " width='{0}' height='{0}'"
			    " viewBox='0 0 {1} {1}'"
			    " fill='currentColor'"
			    ">{2}</svg>",
			    size, icon.size, icon.path);
		}

		static octicons& get(std::filesystem::path const& path) {
			static octicons instance = load(path);
			return instance;
		}

		static octicons load(std::filesystem::path data_path) {
			octicons ret;
			auto file = io::fopen(data_path, "rb");
			if (!file) return ret;
			auto const bytes = file.read();

			auto const root = json::read_json(
			    {reinterpret_cast<char8_t const*>(bytes.data()), bytes.size()});

			auto icons = cast<json::map>(root);
			if (icons) {
				for (auto const& [key, json_icon] : icons->items()) {
					auto icon = cast<json::map>(json_icon);
					auto const width = cast<long long>(icon, u8"width"s);
					auto const path = cast<json::string>(icon, u8"path"s);
					if (!width || !path || *width < 1) continue;

					ret.icons_[from_u8s(key)] = {
					    .size = static_cast<unsigned>(*width),
					    .path = from_u8s(*path)};
				}
			}

			return ret;
		}
	};

	mstch::node const& octicon_callback::at(std::string const& name) const {
		auto it = storage_.lower_bound(name);
		if (it == storage_.end() || it->first != name) {
			auto const& icons = octicons::get(json_path_);
			it = storage_.insert(it, {name, icons.build(name)});
		}
		return it->second;
	}

	bool octicon_callback::has(std::string const& name) const {
		return octicons::get(json_path_).has(name);
	}

	std::vector<std::string> octicon_callback::debug_all_keys() const {
		return octicons::get(json_path_).debug_all_keys();
	}
}  // namespace cov::app::web
