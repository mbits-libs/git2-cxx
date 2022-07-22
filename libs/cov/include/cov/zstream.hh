// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <zlib.h>
#include <cov/io/file.hh>
#include <git2/bytes.hh>
#include <string>
#include <utility>
#include <vector>

namespace cov {
	struct zstream {
		enum direction : bool {
			inflate = false,
			deflate = true,
		};

		zstream(direction dir);
		virtual ~zstream();
		size_t append(git::bytes data);

	protected:
		void finish();

	private:
		int process(int flush);
		virtual size_t output(git::bytes) = 0;

		z_stream z_{};
		direction deflating_{};
		static constexpr uInt buffer_min_extra = 8, buffer_size = 8 * 1024;
	};

	struct byte_block {
		std::vector<std::byte> data;

		git::bytes bytes() const { return {data.data(), data.size()}; }
		explicit operator git::bytes() const { return bytes(); }

		size_t append(git::bytes bytes) {
			auto const before = data.size();
			data.insert(data.end(), bytes.begin(), bytes.end());
			return data.size() - before;
		}
	};

	struct buffer_zstream : zstream {
		byte_block buffer;

		buffer_zstream(direction dir) : zstream{dir} {}

		size_t output(git::bytes data) override { return buffer.append(data); }

		byte_block close() {
			finish();
			return std::move(buffer);
		}
	};

	class outfile_zstream : public zstream {
		io::file f_{};

	public:
		outfile_zstream(direction dir, io::file f)
		    : zstream{dir}, f_{std::move(f)} {}

		bool opened() const noexcept { return !!f_; }

		size_t output(git::bytes data) override {
			if (!f_) return 0;
			return f_.store(data.data(), data.size());
		}

		void close() {
			finish();
			f_.close();
		}
	};
}  // namespace cov
