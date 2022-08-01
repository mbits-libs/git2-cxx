// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/io/db_object.hh>
#include <cov/io/read_stream.hh>
#include <cov/io/report_files.hh>
#include <git2/bytes.hh>
#include "setup.hh"

namespace cov::testing {
	using namespace std::literals;
	class test_stream final : public write_stream {
	public:
		static constexpr size_t infinite = std::numeric_limits<size_t>::max();
		std::vector<std::byte> data{};
		size_t free_size{infinite};

		bool opened() const noexcept final { return true; }
		size_t write(git::bytes block) final {
			if (free_size != infinite) {
				auto chunk = block.size();
				if (chunk > free_size) chunk = free_size;
				free_size -= chunk;
				block = block.subview(0, chunk);
			}
			auto const size = data.size();
			data.insert(data.end(), block.begin(), block.end());
			return data.size() - size;
		}

		std::string_view view() const noexcept {
			return {reinterpret_cast<char const*>(data.data()), data.size()};
		}
	};

	struct report_files_impl : counted_impl<cov::report_files> {
		std::vector<std::unique_ptr<report_entry>> files{};

		std::vector<std::unique_ptr<report_entry>> const& entries()
		    const noexcept override {
			return files;
		}
	};

	TEST(report_files, load) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x0E\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		std::error_code ec{};
		auto const result = dbo.load({}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_report_files, obj->type());
		auto const lines = as_a<cov::report_files>(obj);
		ASSERT_TRUE(lines);
		ASSERT_TRUE(lines->entries().empty());
	}

	TEST(report_files, load_1) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    "\x0E\x00\x00\x00"

		    "file path\0\0\0"

		    "\x00\x00\x00\x00"
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};

		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		std::error_code ec{};
		auto const result = dbo.load({}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';

		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());

		ASSERT_EQ(obj_report_files, obj->type());
		auto const lines = as_a<cov::report_files>(obj);
		ASSERT_TRUE(lines);
		ASSERT_EQ(1u, lines->entries().size());

		auto const& front = lines->entries().front();
		ASSERT_TRUE(front);
		auto const& entry = *front;
		ASSERT_FALSE(entry.in_workdir());
		ASSERT_TRUE(entry.in_index());
		ASSERT_FALSE(entry.is_modified());
		ASSERT_EQ("file path"sv, entry.path());
		ASSERT_EQ(1250u, entry.stats().total);
		ASSERT_EQ(300u, entry.stats().relevant);
		ASSERT_EQ(299u, entry.stats().covered);
	}

	TEST(report_files, load_partial_1_not_enough_entries) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x02\x00\x00\x00"
		    "\x0E\x00\x00\x00"

		    "file path\0\0\0"

		    "\x00\x00\x00\x00"
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		std::error_code ec{};
		auto const result = dbo.load({}, stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::bad_syntax);
	}

	TEST(report_files, load_partial_2_bad_strings) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\xFF\x00\x00\x00"
		    "\xFF\x00\x00\x00"
		    "\x02\x00\x00\x00"
		    "\x0E\x00\x00\x00"

		    "file path\0\0\0"

		    "\x00\x00\x00\x00"
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		std::error_code ec{};
		auto const result = dbo.load({}, stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::bad_syntax);
	}

	TEST(report_files, load_partial_3_entry_too_small) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    "\x0D\x00\x00\x00"

		    "file path\0\0\0"

		    "\x00\x00\x00\x00"
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		std::error_code ec{};
		auto const result = dbo.load({}, stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::bad_syntax);
	}

	TEST(report_files, load_partial_4_entries_beyond_stream) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\xFF\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    "\x0E\x00\x00\x00"

		    "file path\0\0\0"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		std::error_code ec{};
		auto const result = dbo.load({}, stream, ec);
		ASSERT_FALSE(result);
		ASSERT_EQ(ec, io::errc::bad_syntax);
	}

	TEST(report_files, store) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x0E\x00\x00\x00"sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		auto const obj = make_ref<report_files_impl>();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}

	TEST(report_files, store_1) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    "\x00\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    "\x0E\x00\x00\x00"

		    "file path\0\0\0"

		    "\x00\x00\x00\x00"
		    "\xE2\x04\x00\x00"
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::report_files>();

		auto const obj = make_ref<report_files_impl>();
		report_files_builder builder{};
		builder.add_nfo({.path = "file path"sv, .stats = {1250, 300, 299}});
		obj->files = builder.release();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}
}  // namespace cov::testing
