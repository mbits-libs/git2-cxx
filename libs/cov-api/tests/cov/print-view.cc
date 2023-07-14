// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "print-view.hh"

namespace cov::testing {
	std::ostream& single_char(std::ostream& out, char c) {
		switch (c) {
			case '"':
				return out << "\\\"";
			case '\n':
				return out << "\\n";
			case '\r':
				return out << "\\r";
			case '\t':
				return out << "\\t";
			case '\v':
				return out << "\\v";
			case '\a':
				return out << "\\a";
			default:
				if (!std::isprint(static_cast<unsigned char>(c))) {
					char buffer[10];
					snprintf(buffer, sizeof(buffer), "\\x%02x",
					         static_cast<unsigned char>(c));
					return out << buffer;
				} else {
					return out << c;
				}
		}
	}

	std::ostream& print_view(std::ostream& out, std::string_view text) {
		out << '"';
		for (auto c : text) {
			single_char(out, c);
		}
		return out << '"';
	}

	std::ostream& print_char(std::ostream& out, char c) {
		return single_char(out << '\'', c) << '\'';
	}
}  // namespace cov::testing
