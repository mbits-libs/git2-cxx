// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/file.hh>
#include <cov/streams.hh>

namespace cov::io {
	class direct_read_stream : public read_stream {
		io::file in_{};

	public:
		explicit direct_read_stream(io::file in) : in_{std::move(in)} {}
		explicit operator bool() const noexcept { return !!in_; }

		bool skip(size_t length) override;

	private:
		size_t read(void* ptr, size_t length) override;
	};

	class bytes_read_stream : public read_stream {
		git::bytes in_{};

	public:
		explicit bytes_read_stream(git::bytes in) : in_{in} {}
		explicit operator bool() const noexcept { return true; }
		bool skip(size_t length) override;

	private:
		size_t read(void* ptr, size_t length) override;
	};
}  // namespace cov::io
