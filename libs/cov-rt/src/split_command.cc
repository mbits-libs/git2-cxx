// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cctype>
#include <cov/app/split_command.hh>

namespace cov::app {
	namespace {
		bool isspace(char c) {
			return std::isspace(static_cast<unsigned char>(c));
		}
	}  // namespace

	std::vector<std::string> split_command(std::string_view command) {
		std::vector<std::string> result{};
		std::vector<char> buffer(command.size());

		size_t out_index{};
		char in_quote = 0;
		bool escaped = false;
		for (auto c : command) {
			if (escaped) {
				buffer[out_index++] = c;
				escaped = false;
				continue;
			}
			if (!in_quote) {
				if (isspace(c)) {
					if (out_index) {
						result.emplace_back(buffer.data(), out_index);
						out_index = 0;
					}
					continue;
				}
				if (c == '\'' || c == '"') {
					in_quote = c;
					continue;
				}
			}
			if (c == in_quote) {
				result.emplace_back(buffer.data(), out_index);
				out_index = 0;
				in_quote = 0;
				continue;
			}

			if (c == '\\' && in_quote != '\'') {
				escaped = true;
				continue;
			}
			buffer[out_index++] = c;
		}

		if (out_index) result.emplace_back(buffer.data(), out_index);

		return result;
	}
}  // namespace cov::app
