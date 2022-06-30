// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <gtest/gtest.h>
#include <cctype>
#include <cov/hash/sha1.hh>
#include <cov/zstream.hh>

namespace cov::testing {
	using namespace ::std::literals;
	using ::testing::TestWithParam;
	using ::testing::ValuesIn;

	void print_view(std::ostream& out, std::string_view text) {
		out << '"';
		for (auto c : text) {
			switch (c) {
				case '"':
					out << "\\\"";
					break;
				case '\n':
					out << "\\n";
					break;
				case '\r':
					out << "\\r";
					break;
				case '\t':
					out << "\\t";
					break;
				case '\v':
					out << "\\v";
					break;
				case '\a':
					out << "\\a";
					break;
				default:
					if (!std::isprint(static_cast<unsigned char>(c))) {
						char buffer[10];
						sprintf(buffer, "\\x%02x", c);
						out << buffer;
					} else
						out << c;
					break;
			}
		}
		out << '"';
	}

	struct z_param {
		std::string_view title{};
		std::string_view text{};
		std::basic_string_view<unsigned char> zipped{};

		friend std::ostream& operator<<(std::ostream& out,
		                                z_param const& param) {
			return out << param.title;
		}
	};

	void format_view(std::ostream& out, std::string_view text) {
		print_view(out, text);
	}

	void format_view(std::ostream& out,
	                 std::basic_string_view<unsigned char> text) {
		out << "static constexpr unsigned char result[] = {"sv;
		bool first = true;
		for (auto c : text) {
			if (first)
				first = false;
			else
				out << ',';
			char buffer[10];
			sprintf(buffer, " 0x%02x", c);
			out << buffer;
		}
		out << " }";
	}

	template <typename Char>
	struct format {
		std::basic_string_view<Char> view;

		friend std::ostream& operator<<(std::ostream& out,
		                                format<Char> const& fmt) {
			format_view(out, fmt.view);
			return out;
		}
	};

	class zstream : public TestWithParam<z_param> {
	protected:
		template <typename From, typename To>
		void run(cov::zstream::direction dir,
		         std::basic_string_view<From> from,
		         std::basic_string_view<To> expected) {
			buffer_zstream z{dir};
			z.append({from.data(), from.size()});
			auto const block = z.close();
			auto const actual = std::basic_string_view<To>{
			    reinterpret_cast<To const*>(block.data.data()),
			    block.data.size(),
			};
			ASSERT_EQ(expected, actual) << format<To>{actual};
		}
	};

	TEST_P(zstream, deflate) {
		auto const [_, text, zipped] = GetParam();
		run(cov::zstream::deflate, text, zipped);
	}

	TEST_P(zstream, inflate) {
		auto const [_, text, zipped] = GetParam();
		run(cov::zstream::inflate, zipped, text);
	}

	namespace {
		template <size_t Length>
		constexpr std::basic_string_view<unsigned char> span(
		    unsigned char const (&v)[Length]) noexcept {
			return {v, Length};
		}

		static constexpr unsigned char empty[] = {0x78, 0x01, 0x03, 0x00,
		                                          0x00, 0x00, 0x00, 0x01};

		static constexpr unsigned char digits[] = {
		    0x78, 0x01, 0x33, 0x30, 0x34, 0x32, 0x36, 0x31, 0x35,
		    0x33, 0xb7, 0xb0, 0x04, 0x00, 0x0a, 0xff, 0x02, 0x0e};

		static constexpr unsigned char letters[] = {
		    0x78, 0x01, 0x4b, 0x4c, 0x4a, 0x4e, 0x49, 0x4d, 0x4b, 0xcf,
		    0xc8, 0xcc, 0xca, 0xce, 0xc9, 0xcd, 0xcb, 0x2f, 0x28, 0x2c,
		    0x2a, 0x2e, 0x29, 0x2d, 0x2b, 0xaf, 0xa8, 0xac, 0x72, 0x74,
		    0x72, 0x76, 0x71, 0x75, 0x73, 0xf7, 0xf0, 0xf4, 0xf2, 0xf6,
		    0xf1, 0xf5, 0xf3, 0x0f, 0x08, 0x0c, 0x0a, 0x0e, 0x09, 0x0d,
		    0x0b, 0x8f, 0x88, 0x8c, 0x02, 0x00, 0x16, 0x70, 0x12, 0xff};

		static constexpr unsigned char README[] = {
		    0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x63, 0x50,
		    0x56, 0x08, 0x49, 0x2d, 0x2e, 0xc9, 0xcc, 0x4b, 0x57, 0x28, 0x4a,
		    0x2d, 0xc8, 0x2f, 0xe6, 0x02, 0x00, 0x5b, 0x5a, 0x07, 0x9b};
	}  // namespace

#define Z(name, content) \
	{ #name##sv, content, span(name) }

	static constexpr z_param streams[] = {
	    Z(empty, {}),
	    Z(digits, "0123456789"sv),
	    Z(letters, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"sv),
	    Z(README, "blob 16\x00# Testing repos\n"sv),
	};
	INSTANTIATE_TEST_SUITE_P(streams, zstream, ValuesIn(streams));
}  // namespace cov::testing
// 35,99 zł
