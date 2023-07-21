// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cov/git2/bytes.hh>
#include <cov/io/db_object.hh>
#include <cov/io/function_coverage.hh>
#include <cov/io/read_stream.hh>
#include "setup.hh"
#include "test_stream.hh"

namespace cov::testing {
	namespace {
		using namespace std::literals;
		using namespace git::literals;

		struct function_coverage_impl : counted_impl<cov::function_coverage> {
			std::vector<std::unique_ptr<cov::function_coverage::entry>>
			    functions{};

			std::span<std::unique_ptr<cov::function_coverage::entry> const>
			entries() const noexcept override {
				return functions;
			}
		};
	}  // namespace

	TEST(function_coverage, load) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_function_coverage, obj->type());
		auto const lines = as_a<cov::function_coverage>(obj);
		ASSERT_TRUE(lines);
		ASSERT_TRUE(lines->entries().empty());
	}

	TEST(function_coverage, load_one_item) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // entries
		    "\x07\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "name\x00"  // 0 + 5 | 0x00
		    "\x00"      // 5 + 1 | 0x05
		    "\x00\x00"  // 6 + 2 -> 8

		    // name
		    "\x00\x00\x00\x00"
		    // demangled_name
		    "\x05\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x10\x00\x00\x00"
		    "\x05\x00\x00\x00"
		    // end
		    "\x20\x00\x00\x00"
		    "\x14\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_FALSE(ec) << "   Error: " << ec.message() << " ("
		                 << ec.category().name() << ')';
		ASSERT_TRUE(result);
		ASSERT_TRUE(result->is_object());
		auto const obj = static_cast<object const*>(result.get());
		ASSERT_EQ(obj_function_coverage, obj->type());
		auto const lines = as_a<cov::function_coverage>(obj);
		ASSERT_TRUE(lines);
		ASSERT_EQ(1, lines->entries().size());
		auto const& entry = lines->entries()[0];
		ASSERT_EQ("name"sv, entry->name());
		ASSERT_EQ(""sv, entry->demangled_name());
		ASSERT_EQ(1250, entry->count());
		ASSERT_EQ(16, entry->start().line);
		ASSERT_EQ(5, entry->start().column);
		ASSERT_EQ(32, entry->end().line);
		ASSERT_EQ(20, entry->end().column);
	}

	TEST(function_coverage, partial_load_no_header) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x07\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_entry_too_small) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x01\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec) << sizeof(io::v1::function_coverage::entry) /
		                       sizeof(uint32_t);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_string_inside_header) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x02\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_entries_inside_strings) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x10\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_no_space_for_strings) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x10\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x10\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_cant_load_string) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x04\x00\x00\x00"

		    // entries
		    "\x09\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_no_space_for_entries) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x07\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_cant_load_entry) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x00\x00\x00\x00"

		    // entries
		    "\x05\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"
		    "\x00\x00\x00\x00"sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_name_is_bad) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // entries
		    "\x07\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "name\x00"  // 0 + 5 | 0x00
		    "\x00"      // 5 + 1 | 0x05
		    "\x00\x00"  // 6 + 2 -> 8

		    // name
		    "\x10\x00\x00\x00"
		    // demangled_name
		    "\x05\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x10\x00\x00\x00"
		    "\x05\x00\x00\x00"
		    // end
		    "\x20\x00\x00\x00"
		    "\x14\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_demangled_name_is_bad) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // entries
		    "\x07\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "name\x00"  // 0 + 5 | 0x00
		    "\x00"      // 5 + 1 | 0x05
		    "\x00\x00"  // 6 + 2 -> 8

		    // name
		    "\x00\x00\x00\x00"
		    // demangled_name
		    "\x15\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x10\x00\x00\x00"
		    "\x05\x00\x00\x00"
		    // end
		    "\x20\x00\x00\x00"
		    "\x14\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_line_end_before_start) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // entries
		    "\x07\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "name\x00"  // 0 + 5 | 0x00
		    "\x00"      // 5 + 1 | 0x05
		    "\x00\x00"  // 6 + 2 -> 8

		    // name
		    "\x00\x00\x00\x00"
		    // demangled_name
		    "\x05\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x20\x00\x00\x00"
		    "\x05\x00\x00\x00"
		    // end
		    "\x10\x00\x00\x00"
		    "\x14\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, partial_load_column_end_before_start) {
		static constexpr auto s =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x02\x00\x00\x00"

		    // entries
		    "\x07\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "name\x00"  // 0 + 5 | 0x00
		    "\x00"      // 5 + 1 | 0x05
		    "\x00\x00"  // 6 + 2 -> 8

		    // name
		    "\x00\x00\x00\x00"
		    // demangled_name
		    "\x05\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x10\x00\x00\x00"
		    "\x15\x00\x00\x00"
		    // end
		    "\x10\x00\x00\x00"
		    "\x04\x00\x00\x00"
		    ""sv;
		io::bytes_read_stream stream{git::bytes{s.data(), s.size()}};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		std::error_code ec{};
		auto const result = dbo.load(git::oid{}, stream, ec);
		ASSERT_TRUE(ec);
		ASSERT_FALSE(result);
	}

	TEST(function_coverage, store) {
		static constexpr auto expected =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x04\x00\x00\x00"

		    // entries
		    "\x09\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "_main\x00"
		    "main()\x00"
		    "\x00\x00\x00"

		    // name
		    "\x00\x00\x00\x00"
		    // demangled_name
		    "\x06\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x10\x00\x00\x00"
		    "\x05\x00\x00\x00"
		    // end
		    "\x20\x00\x00\x00"
		    "\x14\x00\x00\x00"
		    ""sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		auto const obj = make_ref<function_coverage_impl>();
		cov::function_coverage::builder builder{};
		builder.add_nfo({
		    .name = "_main"sv,
		    .demangled_name = "main()"sv,
		    .count = 1250,
		    .start = {.line = 16, .column = 5},
		    .end = {.line = 32, .column = 20},
		});
		obj->functions = builder.release();
		auto const result = dbo.store(obj, stream);
		ASSERT_TRUE(result);
		ASSERT_EQ(expected, stream.view());
	}

	TEST(function_coverage, store_limited) {
		static constexpr auto expected =
		    "fnct\x00\x00\x01\x00"

		    // strings
		    "\x05\x00\x00\x00"
		    "\x04\x00\x00\x00"

		    // entries
		    "\x09\x00\x00\x00"
		    "\x07\x00\x00\x00"
		    "\x01\x00\x00\x00"

		    "_main\x00"
		    "main()\x00"
		    "\x00\x00\x00"

		    // name
		    "\x00\x00\x00\x00"
		    // demangled_name
		    "\x06\x00\x00\x00"
		    // count
		    "\xE2\x04\x00\x00"
		    // start
		    "\x10\x00\x00\x00"
		    "\x05\x00\x00\x00"
		    // end
		    "\x20\x00\x00\x00"
		    "\x14\x00\x00\x00"
		    ""sv;
		test_stream stream{};

		io::db_object dbo{};
		dbo.add_handler<io::OBJECT::FUNCTIONS,
		                io::handlers::function_coverage>();

		auto const obj = make_ref<function_coverage_impl>();
		cov::function_coverage::builder builder{};
		builder.add_nfo({
		    .name = "_main"sv,
		    .demangled_name = "main()"sv,
		    .count = 1250,
		    .start = {.line = 16, .column = 5},
		    .end = {.line = 32, .column = 20},
		});
		obj->functions = builder.release();

		for (size_t limit = 4 * sizeof(uint32_t); limit < expected.size();
		     limit += sizeof(uint32_t)) {
			test_stream stream{};
			stream.free_size = limit;

			auto const result = dbo.store(obj, stream);
			ASSERT_FALSE(result);
			ASSERT_EQ(expected.substr(0, limit), stream.view());
		}
	}

	TEST(function_coverage, merge_aliases) {
		auto const obj = cov::function_coverage::builder{}
		                     .add_nfo({
		                         .name = "name_v1"sv,
		                         .demangled_name = "name()"sv,
		                         .count{0},
		                         .start{.line = 5, .column = 0},
		                         .end{.line = 15, .column = 0},
		                     })
		                     .add_nfo({
		                         .name = "name_v2"sv,
		                         .demangled_name = "name()"sv,
		                         .count{50},
		                         .start{.line = 5, .column = 0},
		                         .end{.line = 15, .column = 0},
		                     })
		                     .add_nfo({
		                         .name = "name_v1"sv,
		                         .demangled_name = ""sv,
		                         .count{100},
		                         .start{.line = 0, .column = 0},
		                         .end{.line = 2, .column = 0},
		                     })
		                     .extract();
		auto const actual = obj->merge_aliases();
		decltype(actual) expected = {
		    {
		        .label = {.start = {.line = 0, .column = 0},
		                  .end = {.line = 2, .column = 0},
		                  .name = "name_v1"s},
		        .count = 100,
		    },
		    {
		        .label = {.start = {.line = 5, .column = 0},
		                  .end = {.line = 15, .column = 0},
		                  .name = "name()"s},
		        .count = 50,
		    },
		};
		ASSERT_EQ(expected.size(), actual.size());

		for (size_t index = 0; index < expected.size(); ++index) {
			auto const& exp = expected[index];
			auto const& act = actual[index];
			ASSERT_EQ(exp.count, act.count) << "Index: " << index;
			ASSERT_EQ(exp.label.name, act.label.name) << "Index: " << index;
			ASSERT_EQ(exp.label.start.line, act.label.start.line)
			    << "Index: " << index;
			ASSERT_EQ(exp.label.start.column, act.label.start.column)
			    << "Index: " << index;
			ASSERT_EQ(exp.label.end.line, act.label.end.line)
			    << "Index: " << index;
			ASSERT_EQ(exp.label.end.column, act.label.end.column)
			    << "Index: " << index;
		}
	}
}  // namespace cov::testing
