// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/git2/bytes.hh>
#include <cov/io/db_object.hh>
#include <cov/io/files.hh>
#include <cov/io/read_stream.hh>
#include "setup.hh"

namespace cov::testing {
	namespace {
		using namespace std::literals;
		using namespace git::literals;
		class test_stream final : public write_stream {
		public:
			static constexpr size_t infinite =
			    std::numeric_limits<size_t>::max();
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
				return {reinterpret_cast<char const*>(data.data()),
				        data.size()};
			}
		};

		struct files_impl : counted_impl<cov::files> {
			std::vector<std::unique_ptr<cov::files::entry>> files{};

			std::span<std::unique_ptr<cov::files::entry> const> entries()
			    const noexcept override {
				return files;
			}

			cov::files::entry const* by_path(
			    std::string_view path) const noexcept override {
				for (auto const& entry : files) {
					if (entry->path() == path) return entry.get();
				}
				return nullptr;
			}
		};
	}  // namespace

	TEST(files, load) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_files, obj->type());
		auto const lines = as_a<cov::files>(obj);
		ASSERT_TRUE(lines);
		ASSERT_TRUE(lines->entries().empty());
	}

	TEST(files, load_basic) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - line_total
		    "\xE2\x04\x00\x00"

		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"

		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_files, obj->type());
		auto const files = as_a<cov::files>(obj);
		ASSERT_TRUE(files);
		ASSERT_EQ(1, files->entries().size());

		ASSERT_TRUE(files->by_path("file path"sv));
		ASSERT_FALSE(files->by_path("another path"sv));
	}

	TEST(files, load_ext) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x1C\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - line_total
		    "\xE2\x04\x00\x00"
		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - functions
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - branches
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_files, obj->type());
		auto const files = as_a<cov::files>(obj);
		ASSERT_TRUE(files);
		ASSERT_EQ(1, files->entries().size());
		ASSERT_TRUE(files->by_path("file path"sv));
		ASSERT_FALSE(files->by_path("another path"sv));
	}

	TEST(files, partial_load_1_no_header) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_2_A_strings_inside_header) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_2_B_entry_too_small) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_2_C_entries_before_strings) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x0C\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_3_strings_too_far) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x08\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x0B\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_4_not_enough_data_for_strings) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // strings
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_5_entries_too_far) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x09\x00\x00\x00"
		    "\x1C\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // strings
		    "file path\0\0\0"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_6_not_enough_data_for_entry) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x1C\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // strings
		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - total
		    "\xE2\x04\x00\x00"
		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, partial_load_7_entry_path_invalid) {
		static constexpr auto s =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    // strings
		    "file path\0\0\0"

		    // - path
		    "\x00\x01\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - total
		    "\xE2\x04\x00\x00"
		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(files, store) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x05\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		auto const obj = make_ref<files_impl>();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}

	TEST(files, store_1) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - total
		    "\xE2\x04\x00\x00"

		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12"
		    "\x34\x56\x78\x90"sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		auto const obj = make_ref<files_impl>();
		files::builder builder{};
		builder.add_nfo(
		    {.path = "file path"sv,
		     .stats = {1250, {300, 299}, {0, 0}, {0, 0}},
		     .line_coverage = "1234567890123456789012345678901234567890"_oid});
		obj->files = builder.release();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}

	TEST(files, store_with_newer_data) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    // entries
		    "\x08\x00\x00\x00"
		    "\x1C\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - total
		    "\xE2\x04\x00\x00"

		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // - functions
		    "\x0F\x00\x00\x00"
		    "\x0A\x00\x00\x00"
		    "\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12"
		    "\x34\x56\x78\x90"

		    // - branches
		    "\xE6\x00\x00\x00"
		    "\xDE\x00\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		auto const obj = make_ref<files_impl>();
		files::builder builder{};
		builder.add_nfo({.path = "file path"sv,
		                 .stats = {1250, {300, 299}, {15, 10}, {230, 222}},
		                 .function_coverage =
		                     "1234567890123456789012345678901234567890"_oid});
		obj->files = builder.release();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}

	TEST(files, store_limited_1) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"

		    // entries
		    "\x08\x00\x00\x00"
		    "\x0E\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - total
		    "\xE2\x04\x00\x00"
		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    ""sv;
		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		files::builder builder{};
		builder.add_nfo({.path = "file path"sv,
		                 .stats = {1250, {300, 299}, {0, 0}, {0, 0}}});
		auto const obj = make_ref<files_impl>();
		obj->files = builder.release();

		for (size_t limit = 4 * sizeof(uint32_t); limit < expected.size();
		     limit += sizeof(uint32_t)) {
			test_stream stream{};
			stream.free_size = limit;

			auto const result = dbo.store(obj, stream);
			ASSERT_FALSE(result);
			ASSERT_EQ(expected.substr(0, limit), stream.view());
		}
	}

	TEST(files, store_limited_2) {
		static constexpr auto expected =
		    "list\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x03\x00\x00\x00"
		    // entries
		    "\x08\x00\x00\x00"
		    "\x1C\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "file path\0\0\0"

		    // - path
		    "\x00\x00\x00\x00"
		    // - contents
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    // - total
		    "\xE2\x04\x00\x00"

		    // - lines
		    "\x2C\x01\x00\x00"
		    "\x2B\x01\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // - functions
		    "\x0F\x00\x00\x00"
		    "\x0A\x00\x00\x00"
		    "\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12\x34\x56\x78\x90\x12"
		    "\x34\x56\x78\x90"

		    // - branches
		    "\xE6\x00\x00\x00"
		    "\xDE\x00\x00\x00"
		    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FILES, io::handlers::files>();

		files::builder builder{};
		builder.add_nfo({.path = "file path"sv,
		                 .stats = {1250, {300, 299}, {15, 10}, {230, 222}},
		                 .function_coverage =
		                     "1234567890123456789012345678901234567890"_oid});
		auto const obj = make_ref<files_impl>();
		obj->files = builder.release();

		for (size_t limit = 4 * sizeof(uint32_t); limit < expected.size();
		     limit += sizeof(uint32_t)) {
			test_stream stream{};
			stream.free_size = limit;

			auto const result = dbo.store(obj, stream);
			ASSERT_FALSE(result);
			ASSERT_EQ(expected.substr(0, limit), stream.view());
		}
	}

	TEST(files, remove) {
		files::builder builder{};
		builder.add_nfo({.path = "Alpha"})
		    .add_nfo({.path = "Beta"})
		    .add_nfo({.path = "Gamma"})
		    .add_nfo({.path = "Delta"});
		ASSERT_TRUE(builder.remove("Beta"));
		ASSERT_FALSE(builder.remove("Epsilon"));
		std::vector<std::string_view> expected = {"Alpha", "Delta", "Gamma"};
		auto result = builder.release();
		std::vector<std::string_view> actual{};
		actual.reserve(result.size());
		for (auto const& ptr : result)
			actual.push_back(ptr->path());
		ASSERT_EQ(expected, actual);
	}
}  // namespace cov::testing
