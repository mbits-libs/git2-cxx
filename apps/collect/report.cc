// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "report.hh"
#include <fmt/format.h>
#include <cov/app/path.hh>
#include <cov/hash/sha1.hh>
#include <cov/io/file.hh>
#include <cov/version.hh>
#include <native/str.hh>
#include <string>

using namespace std::literals;

namespace cov::app::collect {
	coverage_file* report::get_file(std::string_view path) {
		if (!path.starts_with(src_prefix_)) return nullptr;
		path = path.substr(src_prefix_.length());
		for (auto const& excl_path : cfg_.exclude) {
			auto excl = get_u8path(excl_path);
			if (!path.starts_with(excl)) continue;
			auto rest = path.substr(excl.size());
			if (rest.empty() || rest.front() == '/') return nullptr;
		}
		for (auto const& incl_path : cfg_.include) {
			auto incl = get_u8path(incl_path);
			if (!path.starts_with(incl)) continue;
			auto rest = path.substr(incl.size());
			if (rest.empty() || rest.front() == '/') {
				auto key = std::string{path.data(), path.size()};
				auto it = files_.find(key);
				if (it == files_.end()) {
					bool ignore = false;
					std::tie(it, ignore) =
					    files_.insert({key, std::make_unique<file>(key)});
				}
				return it->second.get();
			}
		}
		return nullptr;
		{}
	}

	std::unique_ptr<coverage_file> report::get_file_mt(std::string_view path) {
		if (!path.starts_with(src_prefix_)) return nullptr;
		path = path.substr(src_prefix_.length());
		for (auto const& excl_path : cfg_.exclude) {
			auto excl = get_u8path(excl_path);
			if (!path.starts_with(excl)) continue;
			auto rest = path.substr(excl.size());
			if (rest.empty() || rest.front() == '/') return nullptr;
		}
		for (auto const& incl_path : cfg_.include) {
			auto incl = get_u8path(incl_path);
			if (!path.starts_with(incl)) continue;
			auto rest = path.substr(incl.size());
			if (rest.empty() || rest.front() == '/') {
				return std::make_unique<file>(
				    std::string{path.data(), path.size()});
			}
		}
		return nullptr;
	}

	void report::handover_mt(std::unique_ptr<coverage_file>&& returned) {
		std::lock_guard lock{m_};
		std::unique_ptr<file> src{static_cast<file*>(returned.release())};

		auto it = files_.find(src->filename());
		if (it == files_.end()) {
			bool ignore = false;
			std::tie(it, ignore) =
			    files_.insert({src->filename(), std::move(src)});
			return;
		}
		it->second->merge(*src);
	}

	void report::post_process() {
		for (auto& [name, data] : files_) {
			data->post_process(cfg_.src_dir / make_u8path(name));
		}
	}

	io::v1::coverage_stats report::summary() const {
		auto result = io::v1::coverage_stats::init();
		result.lines_total = static_cast<unsigned>(files_.size());  // reuse
		for (auto& [name, data] : files_) {
			result.lines.relevant +=
			    static_cast<unsigned>(data->lines().size());
			for (auto const& [_, count] : data->lines()) {
				if (count) ++result.lines.visited;
			}

			result.functions.relevant +=
			    static_cast<unsigned>(data->functions().size());
			for (auto const& fun : data->functions()) {
				if (fun.count) ++result.functions.visited;
			}
		}
		return result;
	}

	json::node report::get_json(json::string const& branch, git::oid_view ref) {
		json::array json_files{};
		json_files.reserve(files_.size());
		for (auto const& [filename, file] : files_) {
			json_files.push_back(file->get_json(filename));
		}

		return json::map{
		    {u8"$schema"s,
		     to_u8s(fmt::format("https://raw.githubusercontent.com/mzdun/cov/"
		                        "v{}/apps/report-schema.json",
		                        cov::version::string))},
		    {
		        u8"git"s,
		        json::map{{u8"branch"s, branch},
		                  {u8"head"s, to_u8s(ref.str())}},
		    },
		    {u8"files"s, std::move(json_files)},
		};
	}

	std::string report::build_prefix() const {
		return fmt::format("{}/", get_u8path(cfg_.src_dir));
	}

	json::node report::function_cvg::get_json() const {
		json::map item{
		    {u8"name"s, to_u8s(name)},
		    {u8"count"s, count},
		    {u8"start_line"s, start.line},
		};
		if (start.col) item[u8"start_column"s] = start.col;
		if (stop.line) item[u8"end_line"s] = stop.line;
		if (stop.col) item[u8"end_column"s] = stop.col;
		if (!demangled.empty()) item[u8"demangled"s] = to_u8s(demangled);
		return {std::move(item)};
	}

	void report::file::on_visited(unsigned line, unsigned count) {
		lines_[line] += count;
	}

	void report::file::on_function(text_pos const& start,
	                               text_pos const& stop,
	                               unsigned count,
	                               std::string const& name,
	                               std::string const& demangled) {
		functions_.push_back({.start = start,
		                      .stop = stop,
		                      .count = count,
		                      .name = name,
		                      .demangled = demangled});
	}

	void report::file::merge(file& src) {
		for (auto const& [line, count] : src.lines_)
			lines_[line] += count;
		functions_.insert(functions_.end(),
		                  std::make_move_iterator(src.functions_.begin()),
		                  std::make_move_iterator(src.functions_.end()));
	}

	void report::file::post_process(std::filesystem::path const& path) {
		auto input = cov::io::fopen(path, "rb");
		if (!input) {
			hash_.clear();
			return;
		}

		std::byte buffer[8192];
		hash::sha1 m{};

		while (auto size = input.load(buffer, sizeof(buffer))) {
			m.update({buffer, size});
		}
		hash_ = fmt::format("sha1:{}", m.finalize().str());

		std::sort(functions_.begin(), functions_.end());
	}

	json::node report::file::get_json(std::string_view filename) const {
		json::map result{{u8"name"s, to_u8s(filename)},
		                 {u8"digest"s, to_u8s(hash_)},
		                 {u8"line_coverage"s, get_lines()}};
		if (!functions_.empty()) {
			result[u8"functions"s] = get_functions();
		}

		return {std::move(result)};
	}

	json::node report::file::get_lines() const {
		json::map result{};
		for (auto const& [line, count] : lines_) {
			result[to_u8s(fmt::format("{}", line))] = count;
		}
		return {std::move(result)};
	}

	json::node report::file::get_functions() const {
		json::array result{};
		result.reserve(functions_.size());

		for (auto const& function : functions_) {
			result.emplace_back(function.get_json());
		}
		return {result};
	}
}  // namespace cov::app::collect
