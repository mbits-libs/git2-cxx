// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <charconv>
#include <chrono>
#include <cov/app/path.hh>
#include <cov/app/report.hh>
#include <cov/git2/commit.hh>
#include <cov/hash/md5.hh>
#include <cov/hash/sha1.hh>
#include <cov/io/file.hh>
#include <json/json.hpp>

namespace cov::app::report {
	namespace {
		std::u8string_view to_json(std::string_view str) {
			return {reinterpret_cast<char8_t const*>(str.data()), str.size()};
		}

		std::string_view from_json(std::u8string_view str) {
			return {reinterpret_cast<char const*>(str.data()), str.size()};
		}

		struct digest_info {
			std::string_view id;
			digest result;
		};

		digest lookup_digest(std::string_view algorithm) {
			static constexpr digest_info algorithms[] = {
			    {"md5"sv, digest::md5},
			    {"sha"sv, digest::sha1},
			    {"sha1"sv, digest::sha1},
			};
			// too short for binary search
			for (auto const& [id, result] : algorithms) {
				if (id == algorithm) return result;
			}
			return digest::unknown;
		}

		bool conv_rebase(std::u8string_view key, unsigned& out) {
			auto const view = from_json(key);
			auto const first = view.data();
			auto const last = first + view.size();
			auto const [ptr, ec] = std::from_chars(first, last, out);
			auto const ok = ptr == last && ec == std::errc{};
			if (ok && out) --out;
			return ok;
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

		inline uint32_t u32_from(long long value) {
			if (value < 0) return 0;
			static constexpr auto max_value =
			    std::numeric_limits<uint32_t>::max();
			auto const u_hits =
			    std::min(unsigned_cast<unsigned long long>(value),
			             unsigned_cast<unsigned long long>(max_value));
			return unsigned_cast<uint32_t>(u_hits);
		}

		inline std::string stored(std::string_view view) {
			return {view.data(), view.size()};
		}

		inline std::string_view rstrip(std::string_view view) {
			while (!view.empty() &&
			       std::isspace(static_cast<unsigned char>(view.back())))
				view = view.substr(0, view.size() - 1);
			return view;
		}

		git_signature signature(git::commit::signature const& sign) {
			return {.name = stored(sign.name), .mail = stored(sign.email)};
		}

		enum class matching {
			none,
			exactly,
			with_different_newlines,
		};

		unsigned nybble(char c) {
			switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					return static_cast<unsigned>(c - '0');
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					return static_cast<unsigned>(c - 'a' + 10);
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					return static_cast<unsigned>(c - 'A' + 10);
				default:
					return 16;
			}
		}

		template <typename Digest>
		matching match_(std::string_view hash, git::bytes data) {
			using digest_type = typename Digest::digest_type;
			static constexpr auto digest_byte_size = Digest::digest_byte_size;

			if (hash.size() != digest_byte_size * 2) return matching::none;
			digest_type expected{};
			for (size_t index = 0; index < digest_byte_size; ++index) {
				auto const upper = nybble(hash[index * 2]);
				auto const lower = nybble(hash[index * 2 + 1]);
				if (upper > 15 || lower > 15) return matching::none;
				expected.data[index] =
				    static_cast<std::byte>((upper << 4) | lower);
			}

			if (Digest::once(data) == expected) return matching::exactly;

			{
				// 2. replace \n with \r\n
				Digest calc{};
				auto chars = std::string_view{
				    reinterpret_cast<char const*>(data.data()), data.size()};
				auto pos = chars.find('\n');
				while (pos != std::string_view::npos) {
					if (!pos || chars[pos - 1] != '\r') {
						auto const chunk = chars.substr(0, pos);

						if (chunk.size()) calc.update(git::bytes{chunk});
						calc.update(git::bytes{"\r\n"sv});

						chars = chars.substr(pos + 1);
						pos = chars.find('\n');
						continue;
					}
					pos = chars.find('\n', pos + 1);
				}
				if (chars.size()) calc.update(git::bytes{chars});
				if (calc.finalize() == expected)
					return matching::with_different_newlines;
			}

			{
				// 3. replace \r\n with \n
				Digest calc{};
				auto chars = std::string_view{
				    reinterpret_cast<char const*>(data.data()), data.size()};
				auto pos = chars.find("\r\n"sv);
				while (pos != std::string_view::npos) {
					auto const chunk = chars.substr(0, pos);

					if (chunk.size()) calc.update(git::bytes{chunk});
					calc.update(git::bytes{"\n"sv});

					chars = chars.substr(pos + 2);
					pos = chars.find("\r\n"sv);
				}
				if (chars.size()) calc.update(git::bytes{chars});
				if (calc.finalize() == expected)
					return matching::with_different_newlines;
			}

			return matching::none;
		}

		matching match(digest type, std::string_view hash, git::bytes data) {
			switch (type) {
				case digest::md5:
					return match_<hash::md5>(hash, data);
				case digest::sha1:
					return match_<hash::sha1>(hash, data);
				default:
					break;
			}
			return matching::none;
		}

		size_t lines_in(git::bytes data) {
			static constexpr auto LN = std::byte{'\n'};
			size_t lines{};
			for (auto b : data) {
				if (b == LN) ++lines;
			}
			if (data.size() == 0 || data.data()[data.size() - 1] != LN) ++lines;
			return lines;
		}

		unsigned visit_lines(std::map<unsigned, unsigned> const& line_coverage,
		                     size_t line_count,
		                     auto visitor) {
			static constexpr auto max_u31 = (1u << 31) - 1;

			unsigned prev_line = 0;
			for (auto [line, hits] : line_coverage) {
				auto null_lines = line - prev_line - 1;
				while (null_lines) {
					auto const chunk = std::min(max_u31, null_lines);
					null_lines -= chunk;
					visitor(chunk, true);
				}

				visitor(std::min(max_u31, hits), false);
				prev_line = line;
			}

			auto const result =
			    std::max(prev_line, static_cast<unsigned>(line_count));
			if (prev_line < result) {
				auto null_lines = result - prev_line - 1;
				while (null_lines) {
					auto const chunk = std::min(max_u31, null_lines);
					null_lines -= chunk;
					visitor(chunk, true);
				}
			}

			return result;
		}
	}  // namespace

	file_info::coverage_info file_info::expand_coverage(
	    size_t line_count) const {
		coverage_info result{std::vector<io::v1::coverage>{},
		                     io::v1::coverage_stats::init()};
		auto& [coverage, stats] = result;

		size_t size{};
		stats.lines_total = visit_lines(line_coverage, line_count,
		                                [&](unsigned, bool) { ++size; });
		coverage.reserve(size);
		visit_lines(line_coverage, line_count,
		            [&coverage = coverage, &stats = stats](unsigned count,
		                                                   bool is_null) {
			            coverage.push_back(
			                io::v1::coverage{.value = count & ((1u << 31) - 1),
			                                 .is_null = is_null ? 1u : 0u});
			            if (!is_null) {
				            ++stats.lines.relevant;
				            if (count) ++stats.lines.visited;
			            }
		            });

		return result;
	}  // GCOV_EXCL_LINE[GCC]

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
			auto const json_file_functions_coverage =
			    cast_from_json<json::array>(file_node, u8"functions"sv);

			if (!json_file_name || !json_file_digest ||
			    !json_file_line_coverage) {
				[[unlikely]];
				files.clear();
				return false;
			}

			auto digest_view = from_json(*json_file_digest);
			auto const pos = digest_view.find(':');
			auto const algorithm = lookup_digest(digest_view.substr(0, pos));
			if (pos == std::string_view::npos || algorithm == digest::unknown) {
				[[unlikely]];
				files.clear();
				return false;
			}

			files.push_back({});
			auto& dst = files.back();

			dst.name.assign(from_json(*json_file_name));
			dst.algorithm = algorithm;
			dst.digest.assign(digest_view.substr(pos + 1));

			for (auto const& [node_key, node_value] :
			     json_file_line_coverage->items()) {
				auto const hits = cast<long long>(node_value);

				unsigned line{};
				if (!hits || !conv_rebase(node_key, line)) {
					[[unlikely]];
					files.clear();
					return false;
				}
				dst.line_coverage[line] = unsigned_from(*hits);
			}

			if (json_file_functions_coverage) {
				auto& cvg = dst.function_coverage;
				cvg.reserve(json_file_functions_coverage->size());
				for (auto const& node_file : *json_file_functions_coverage) {
					auto const name =
					    cast_from_json<json::string>(node_file, u8"name"sv);
					auto const count =
					    cast_from_json<long long>(node_file, u8"count"sv);
					auto const start_line =
					    cast_from_json<long long>(node_file, u8"start_line"sv);

					if (!name || !count || !start_line) continue;

					auto const demangled = cast_from_json<json::string>(
					    node_file, u8"demangled"sv);
					auto const start_column = cast_from_json<long long>(
					    node_file, u8"start_column"sv);
					auto const end_line =
					    cast_from_json<long long>(node_file, u8"end_line"sv);
					auto const end_column =
					    cast_from_json<long long>(node_file, u8"end_column"sv);

					cvg.push_back({});
					auto& nfo = cvg.back();
					nfo.name.assign(from_json(*name));
					if (demangled)
						nfo.demangled_name.assign(from_json(*demangled));
					nfo.count = u32_from(*count);
					nfo.start.line = u32_from(*start_line);
					nfo.start.column =
					    u32_from(start_column ? *start_column : 0);
					nfo.end.line = u32_from(end_line ? *end_line : *start_line);
					nfo.end.column = u32_from(end_column ? *end_column : 0);

					// rebase line indexes
					if (nfo.start.line > 0) --nfo.start.line;
					if (nfo.end.line > 0) --nfo.end.line;
				}

				std::sort(cvg.begin(), cvg.end());
			}
		}

		git.branch.assign(from_json(*json_git_branch));
		git.head.assign(from_json(*json_git_head));
		return true;
	}

	git_commit git_commit::load(git::repository_handle repo,
	                            std::string_view commit_id,
	                            std::error_code& ec) {
		auto const commit = repo.lookup<git::commit>(commit_id, ec);
		if (ec) return {};
		auto tree = commit.tree(ec);
		if (ec) return {};
		return {
		    .author = signature(commit.author()),
		    .committer = signature(commit.committer()),  // GCOV_EXCL_LINE[GCC]
		    .committed = commit.committer().when,        // GCOV_EXCL_LINE[GCC]
		    .message = stored(rstrip(commit.message_raw())),
		    .tree = std::move(tree),
		};
	}  // GCOV_EXCL_LINE[WIN32]

	blob_info git_commit::verify(file_info const& file) const {
		std::error_code ec{};
		auto const entry = tree.blob_bypath(file.name.c_str(), ec);
		auto flags = text::missing;

		if (entry && !ec) {
			auto const bytes = entry.raw();
			auto const result = match(file.algorithm, file.digest, bytes);
			if (result != matching::none) {
				return {.flags = result == matching::exactly
				                     ? text::in_repo
				                     : text::in_repo | text::different_newline,
				        .existing = entry.oid().oid(),
				        .lines = lines_in(bytes)};
			}
			flags = text::in_repo;
		}

		auto raw = git_object_owner(reinterpret_cast<git_object*>(tree.raw()));
		auto workdir = git::repository_handle{raw}.workdir();
		if (!workdir) {
			// there is no source to compare against...
			return {};
		}

		auto const opened =
		    io::fopen(make_u8path(*workdir) / make_u8path(file.name), "rb");
		if (!opened) return {};

		auto stg = opened.read();
		auto const result =
		    match(file.algorithm, file.digest, {stg.data(), stg.size()});

		flags = result == matching::none
		            ? flags | text::in_fs | text::mismatched
		        : result == matching::exactly
		            ? text::in_fs
		            : text::in_fs | text::different_newline;
		auto const lines = result == matching::none
		                       ? size_t{}
		                       : lines_in({stg.data(), stg.size()});
		return {.flags = flags, .lines = lines};
	}  // GCOV_EXCL_LINE[WIN32]

}  // namespace cov::app::report
