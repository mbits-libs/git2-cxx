// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/hash/sha1.hh>
#include <cov/io/file.hh>
#include <cov/streams.hh>
#include <cov/zstream.hh>

namespace cov::io {
	class safe_base {
	public:
		safe_base(std::filesystem::path filename, std::string_view prefix = {})
		    : path_{std::move(filename)}, tmp_filename_{alt_filename(prefix)} {}

	protected:
		std::filesystem::path path_;
		std::filesystem::path tmp_filename_;

	private:
		std::filesystem::path alt_filename(
		    std::string_view alt_prefix = {}) const;
	};

	class safe_stream final : public tmp_write_stream, private safe_base {
	public:
		safe_stream(std::filesystem::path filename)
		    : safe_base{std::move(filename)} {}

		size_t write(git::bytes data) override;
		bool opened() const noexcept override;
		std::error_code commit() override;

	private:
		io::file out_{io::fopen(tmp_filename_, "wb")};
	};

	class safe_z_stream final : public z_write_stream, private safe_base {
	public:
		safe_z_stream(std::filesystem::path builddir, std::string_view prefix)
		    : safe_base{std::move(builddir), prefix} {}
		~safe_z_stream();

		size_t write(git::bytes data) override;
		bool opened() const noexcept override;
		git_oid finish() override;

	private:
		hash::sha1 id_{};
		outfile_zstream out_{zstream::deflate, io::fopen(tmp_filename_, "wb")};
	};
}  // namespace cov::io
