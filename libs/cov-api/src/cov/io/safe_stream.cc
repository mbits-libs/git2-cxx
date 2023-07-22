// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <git2/oid.h>
#include <cov/io/safe_stream.hh>
#include <cov/random.hh>

namespace cov::io {
	namespace {
		using namespace std::filesystem;

		path tmp_fname(std::string_view prefix, std::string_view postfix) {
			// ignore ec here, will fail on fopen and propagate to
			// outfile_zstream::output

			constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

			std::uniform_int_distribution<size_t> chars(0,
			                                            sizeof(alphabet) - 1);
			std::uniform_int_distribution<size_t> lengths(6, 10);
			auto engine = cov::seed_sequence::mt();

			auto const length = lengths(engine);
			auto filename = std::string{};
			filename.reserve(prefix.length() + postfix.length() + length + 1);

			filename.append(prefix);
			for (size_t i = 0; i < length; ++i) {
				filename.push_back(alphabet[chars(engine)]);
			}
			filename.append(postfix);

			return filename;
		}

		path tmp_fname(path const& base,
		               std::string_view prefix,
		               std::string_view postfix,
		               std::error_code& ec,
		               bool ignore_mkdir) {
			create_directories(base, ec);
			if (ec && !ignore_mkdir) return {};

			return base / tmp_fname(prefix, postfix);
		}

		path tmp_fname(path const& base,
		               std::string_view prefix,
		               std::string_view postfix,
		               bool ignore_mkdir) {
			std::error_code ec;
			auto tmp = tmp_fname(base, prefix, postfix, ec, ignore_mkdir);
			if (ec && !ignore_mkdir)
				throw filesystem_error("tmp_fname", base, ec);

			return tmp;
		}
	}  // namespace

	std::filesystem::path safe_base::alt_filename(std::string_view alt_prefix,
	                                              bool ignore_mkdir) const {
		return tmp_fname(
		    path_.parent_path(),
		    alt_prefix.empty() ? path_.stem().string() + "_tmp" : alt_prefix,
		    path_.extension().string(), ignore_mkdir);
	}

	size_t safe_stream::write(git::bytes data) {
		return out_.store(data.data(), data.size());
	}

	bool safe_stream::opened() const noexcept { return !!out_; }

	std::error_code safe_stream::commit() {
		out_.close();
		std::error_code ec;
		rename(tmp_filename_, path_, ec);
		return ec;
	}

	void safe_stream::rollback() {
		out_.close();
		std::filesystem::remove(tmp_filename_);
	}

	safe_z_stream::~safe_z_stream() {
		std::error_code ignore{};
		remove(tmp_filename_, ignore);
	}

	size_t safe_z_stream::write(git::bytes data) {
		id_.update(data);
		return out_.append(data);
	}

	bool safe_z_stream::opened() const noexcept { return out_.opened(); }

	git::oid safe_z_stream::finish() {
		out_.close();

		auto const sha_id = id_.finalize();
		git::oid out{};

		static_assert(sizeof(sha_id.data) == sizeof(out.id.id),
		              "git::oid and sha1 digest sizes are mismatched");

		memcpy(&out.id.id, sha_id.data, sizeof(sha_id.data));

		auto const filename = path_ / out.path();

		create_directories(filename.parent_path());
		rename(tmp_filename_, filename);
		return out;
	}

	void safe_z_stream::rollback() {
		out_.close();
		std::filesystem::remove(tmp_filename_);
	}
}  // namespace cov::io
