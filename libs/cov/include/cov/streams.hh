// Copyright (c) 2022 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <git2/oid.h>
#include <concepts>
#include <filesystem>
#include <git2/bytes.hh>
#include <system_error>
#include <type_traits>
#include <vector>

namespace cov {
	template <typename Type>
	concept Simple = std::is_standard_layout_v<Type> && std::is_trivial_v<Type>;

	struct write_stream {
		virtual ~write_stream();
		virtual bool opened() const noexcept = 0;

		template <Simple T>
		bool store(T const& datum) {
			return write(&datum, sizeof(datum)) == sizeof(datum);
		}

		template <Simple T, size_t Length>
		bool store(T const (&data)[Length]) {
			return write(data, sizeof(data) * Length) == sizeof(data) * Length;
		}

		template <Simple T>
		bool store(std::vector<T> const& data) {
			return write(data.data(), sizeof(T) * data.size()) ==
			       sizeof(T) * data.size();
		}

		bool store(git::bytes data) { return write(data) == data.size(); }

	private:
		size_t write(void const* data, size_t size) {
			return write({reinterpret_cast<std::byte const*>(data), size});
		}
		virtual size_t write(git::bytes) = 0;
	};

	struct z_write_stream : write_stream {
		virtual git_oid finish() = 0;
	};

	struct tmp_write_stream : write_stream {
		[[nodiscard]] virtual std::error_code commit() = 0;
	};

	struct read_stream {
		virtual ~read_stream();

		template <Simple T>
		bool load(T& datum) {
			return read(&datum, sizeof(datum)) == sizeof(datum);
		}

		template <Simple T>
		bool load(std::vector<T>& data, size_t count) {
			data.resize(count);
			if (read(data.data(), sizeof(T) * data.size()) ==
			    sizeof(T) * data.size())
				return true;
			data.clear();
			return false;
		}

		virtual bool skip(size_t length) = 0;

	private:
		virtual size_t read(void* ptr, size_t length) = 0;
	};
}  // namespace cov
