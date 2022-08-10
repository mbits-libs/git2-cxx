// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <git2/oid.h>
#include <cov/db.hh>
#include <cov/io/file.hh>
#include <cov/io/line_coverage.hh>
#include <cov/io/read_stream.hh>
#include <cov/io/report.hh>
#include <cov/io/report_files.hh>
#include <cov/io/safe_stream.hh>
#include <cov/zstream.hh>
#include "path-utils.hh"

namespace cov {
	namespace {
		using namespace ::std::literals;

		bool load_zstream(std::filesystem::path const& filename,
		                  std::vector<std::byte>& output) {
			auto const file = io::fopen(filename, "rb");
			if (!file) return false;
			auto const bytes = file.read();
			buffer_zstream z{zstream::inflate};
			if (z.append({bytes.data(), bytes.size()}) != bytes.size())
				return false;

			output = z.close().data;
			return true;
		}
	}  // namespace

	class loose_backend : public counted_impl<backend> {
	public:
		loose_backend(std::filesystem::path const&);
		ref_ptr<object> lookup_object(git_oid const& id) const override;
		ref_ptr<object> lookup_object(git_oid const& id,
		                              size_t character_count) const override;
		bool write(git_oid& id, ref_ptr<object> const& obj) override;

	private:
		std::filesystem::path root_{};
		io::db_object io_{};
	};

	loose_backend::loose_backend(std::filesystem::path const& root)
	    : root_{root} {
		io_.add_handler<io::OBJECT::REPORT, io::handlers::report>();
		io_.add_handler<io::OBJECT::FILES, io::handlers::report_files>();
		io_.add_handler<io::OBJECT::COVERAGE, io::handlers::line_coverage>();
	}

	ref_ptr<object> loose_backend::lookup_object(git_oid const& id) const {
		char buffer[GIT_OID_HEXSZ + 1];
		if (git_oid_pathfmt(buffer, &id)) return {};
		std::vector<std::byte> bytes;
		if (!load_zstream(root_ / std::string_view{buffer, sizeof(buffer)},
		                  bytes))
			return {};

		io::bytes_read_stream stream{{bytes.data(), bytes.size()}};
		std::error_code ec{};
		auto result = io_.load(id, stream, ec);
		if (!result || ec || !result->is_object()) return {};

		return ref_ptr{static_cast<cov::object*>(result.unlink())};
	}

	ref_ptr<object> loose_backend::lookup_object(git_oid const& id_,
	                                             size_t character_count) const {
		if (character_count >= GIT_OID_HEXSZ) return lookup_object(id_);

		if (character_count < GIT_OID_MINPREFIXLEN) return {};
		static_assert(GIT_OID_MINPREFIXLEN > 2);

		char buffer[GIT_OID_HEXSZ];
		if (git_oid_fmt(buffer, &id_)) return {};
		auto view = std::string_view{buffer, character_count};
		auto dir = view.substr(0, 2);
		auto match = view.substr(2);

		std::error_code ec{};
		auto it = std::filesystem::directory_iterator{root_ / dir, ec};
		if (ec) return {};

		std::vector<std::filesystem::path> paths{};
		git_oid id{id_};
		for (auto const& entry : it) {
			auto filename = get_path(entry.path().filename());
			if (!filename.starts_with(match)) continue;
			if (!paths.empty()) return {};
			paths.push_back(entry.path());
			auto const str = fmt::format("{}{}", dir, filename);
			git_oid_fromstr(&id, str.c_str());
		}
		if (paths.empty()) return {};

		std::vector<std::byte> bytes;
		if (!load_zstream(paths.front(), bytes)) return {};

		io::bytes_read_stream stream{{bytes.data(), bytes.size()}};
		auto result = io_.load(id, stream, ec);
		if (!result || ec || !result->is_object()) return {};

		return ref_ptr{static_cast<cov::object*>(result.unlink())};
	}

	bool loose_backend::write(git_oid& id, ref_ptr<object> const& obj) {
		io::safe_z_stream output{root_, "object"sv};
		if (!output.opened()) return false;

		if (!io_.store(obj, output)) {
			output.rollback();
			return false;
		}

		id = output.finish();
		return true;
	}

	ref_ptr<backend> backend::loose_backend(std::filesystem::path const& root) {
		return make_ref<cov::loose_backend>(root);
	}
}  // namespace cov
