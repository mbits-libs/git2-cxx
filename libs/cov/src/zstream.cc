// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "cov/zstream.hh"
#include <algorithm>
#include <cstring>

namespace cov {
	zstream::zstream(direction dir) : deflating_{dir} {
		if (deflating_)
			deflateInit_(&z_, Z_BEST_SPEED, ZLIB_VERSION, sizeof(z_stream));
		else
			inflateInit_(&z_, ZLIB_VERSION, sizeof(z_stream));
	}

	zstream::~zstream() {
		if (deflating_)
			deflateEnd(&z_);
		else
			inflateEnd(&z_);
	}

	size_t zstream::append(git::bytes data) {
		Byte input[buffer_size];
		Byte output[buffer_size];

		auto ptr = data.data();
		auto size = data.size();

		int ret = Z_OK;
		while (size && ret == Z_OK) {
			z_.avail_in = static_cast<uInt>(std::min(sizeof(input), size));
			z_.next_in = input;

			std::memcpy(input, ptr, z_.avail_in);
			ptr += z_.avail_in;
			size -= z_.avail_in;

			auto const output_size =
			    std::min(std::max(z_.avail_in, buffer_min_extra), buffer_size);
			while (z_.avail_in && ret == Z_OK) {
				z_.avail_out = output_size;
				z_.next_out = output;
				ret = process(Z_NO_FLUSH);
				if (ret != Z_OK) 
					size += z_.avail_in;
			}
		}

		return data.size() - size;
	}

	void zstream::finish() {
		Byte input;
		Byte output[buffer_size];

		z_.avail_in = 0;
		z_.next_in = &input;

		auto ret = Z_OK;
		while (ret != Z_STREAM_END) {
			z_.avail_out = buffer_size;
			z_.next_out = output;
			ret = process(Z_FINISH);
		}
	}

	int zstream::process(int flush) {
		auto const ready = z_.avail_out;
		auto const data = z_.next_out;

		int ret = deflating_ ? ::deflate(&z_, flush) : ::inflate(&z_, flush);

		if (ret == Z_OK || ret == Z_STREAM_END) {
			auto const used = ready - z_.avail_out;
			if (output({data, used}) != used) ret = Z_STREAM_ERROR;
		}

		return ret;
	}
}  // namespace cov
