// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <cxx-filt/expression.hh>

using namespace std::literals;

// #define DEBUG
#ifdef DEBUG
#define dbg_print(...) fmt::print(__VA_ARGS__)
#else
#define dbg_print(...)
#endif

namespace cxx_filt {
	namespace {

		struct all_but_first {
			std::string_view sep{};
			bool first{true};

			std::string_view get() noexcept {
				if (first) {
					first = false;
					return {};
				}
				return sep;
			}
		};

		template <typename Cb>
		std::string to_string(ArgumentList const& args, Cb on_item) {
			std::string result{};
			all_but_first comma{.sep = ", "sv};

			result.append(str_from_token(args.start));
			for (auto const& item : args.items) {
				result.append(comma.get());
				result.append(on_item(item));
			}
			result.append(str_from_token(args.stop));

			return result;
		}  // GCOV_EXCL_LINE[GCC]
	}      // namespace

	std::string ArgumentList::str() const {
		return to_string(*this, [](auto const& item) { return item.str(); });
	}

	std::string ArgumentList::repr() const {
		return to_string(*this, [](auto const& item) { return item.repr(); });
	}

	bool ArgumentList::matches(ArgumentList const& oth,
	                           Refs& refs) const noexcept {
		if (start != oth.start) {
			dbg_print("   > {} != {}\n", dbg_from_token(start),
			          dbg_from_token(oth.start));
			return false;
		}

		if (stop != oth.stop) {
			dbg_print("   > {} != {}\n", dbg_from_token(stop),
			          dbg_from_token(oth.stop));
			return false;
		}

		if (items.size() != oth.items.size()) {
			dbg_print("   > '{}' and '{}' have mismatching lengths: {} vs {}\n",
			          str(), oth.str(), items.size(), oth.items.size());
			return false;
		}

		auto it = oth.items.begin();
		for (auto const& left : items) {
			auto const& right = *it++;
			if (!left.matches(right, refs)) {
				return false;
			}
		}

		return true;
	}

	ArgumentList ArgumentList::replace_with(Refs const& refs) const {
		ArgumentList result{.start = start, .stop = stop};
		result.items.reserve(items.size());
		for (auto const& item : items) {
			result.items.push_back(item.replace_with(refs));
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	ArgumentList ArgumentList::simplified(
	    Replacements const& replacements) const {
		auto result = *this;
		for (auto& item : result.items) {
			item = item.simplified(replacements);
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	std::string Expression::str() const {
		std::string result{};

		for (auto const& item : items) {
			std::visit(
			    overloaded(
			        [&result](std::string const& tok) { result.append(tok); },
			        [&result](Reference const& tok) { result.append(tok.id); },
			        [&result](ArgumentList const& tok) {
				        result.append(tok.str());
			        }),
			    item);
		}
		for (auto const& level : cvrs.levels) {
			if (level.has_const) result.append(" const"sv);
			if (level.has_volatile) result.append(" volatile"sv);
			result.append(str_from_token(level.level_end));
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	std::string Expression::repr() const {
		std::string result{};
		result.append("Expr["sv);
		for (auto const& level : cvrs.levels) {
			if (level.has_const) result.push_back('c');
			if (level.has_volatile) result.push_back('v');
			result.append(str_from_token(level.level_end));
		}
		result.append("]{"sv);

		all_but_first comma{.sep = ", "sv};
		for (auto const& item : items) {
			result.append(comma.get());
			std::visit(
			    overloaded(
			        [&result](std::string const& tok) {
				        result.append(fmt::format("\"{}\"", tok));
			        },
			        [&result](Reference const& tok) { result.append(tok.id); },
			        [&result](ArgumentList const& tok) {
				        result.append(tok.repr());
			        }),
			    item);
		}
		result.push_back('}');
		return result;
	}  // GCOV_EXCL_LINE[GCC]

	bool Expression::matches(Expression const& oth,
	                         Refs& refs,
	                         cvr_eq cvr_eq_policy) const noexcept {
		if (items.size() == 1 &&
		    std::holds_alternative<Reference>(items.front())) {
			auto const& ref_id = std::get<Reference>(items.front()).id;
			auto it = refs.lower_bound(ref_id);
			if (it == refs.end() || it->first != ref_id) {
				dbg_print("   > [{}] = {}\n", ref_id, oth.str());
				refs.insert(it, {ref_id, oth});
				return true;
			}
			auto const matcher = it->second.with(cvrs);
			dbg_print("   > [{}] {} on {}\n", ref_id, matcher.repr(),
			          oth.repr());
			return matcher.matches(oth, refs, cvr_eq_policy);
		}

		if (items.size() != oth.items.size()) {
			dbg_print("   > '{}' and '{}' have mismatching lengths: {} vs {}\n",
			          str(), oth.str(), items.size(), oth.items.size());
			return false;
		}

		if (cvr_eq_policy == cvr_eq::important) {
			if (cvrs != oth.cvrs) {
				dbg_print("   > '{}' and '{}' have mismatching CVRs\n", str(),
				          oth.str());
				return false;
			}
		}

		auto it = oth.items.begin();
		for (auto const& left : items) {
			auto const* left_ptr = &left;
			auto const& right = *it++;

			if (left_ptr->index() != right.index()) {
				return false;
			}

			auto const* left_str = std::get_if<std::string>(left_ptr);
			auto const* left_args = std::get_if<ArgumentList>(left_ptr);

			if (left_str) {
				auto const& right_str = std::get<std::string>(right);
				if (*left_str != right_str) {
					dbg_print("   > \"{}\" != \"{}\"\n", *left_str, right_str);
					return false;
				}
				dbg_print("   > \"{}\" == \"{}\"\n", *left_str, right_str);
				continue;
			}

			if (left_args) {
				auto const& right_args = std::get<ArgumentList>(right);
				if (!left_args->matches(right_args, refs)) {
					dbg_print("   > {} != {}\n", left_args->repr(),
					          right_args.str());
					return false;
				}
				dbg_print("   > {} == {}\n", left_args->repr(),
				          right_args.str());
				continue;
			}

			return false;
		}
		return true;
	}

	std::optional<Refs> Expression::matches(
	    Expression const& oth) const noexcept {
		Refs result{};
		if (!matches(oth, result, cvr_eq::unimportant)) return std::nullopt;
		return {std::move(result)};
	}

	Expression Expression::replace_with(Refs const& refs,
	                                    Cvrs const& original_cvrs) const {
		if (items.size() == 1 &&
		    std::holds_alternative<Reference>(items.front())) {
			auto const& ref_id = std::get<Reference>(items.front()).id;
			refs.find(ref_id);
			auto it = refs.find(ref_id);
			if (it == refs.end()) {
				return {.items = {"?"s}, .cvrs = cvrs};
			}
			return it->second.with(cvrs);
		}

		auto result = Expression{.cvrs = original_cvrs};
		result.items.reserve(items.size());

		for (auto const& item : items) {
			if (std::holds_alternative<ArgumentList>(item)) {
				result.items.push_back(
				    std::get<ArgumentList>(item).replace_with(refs));
			} else {
				result.items.push_back(item);
			}
		}

		return result;
	}

	Expression Expression::simplified(Replacements const& replacements) const {
		auto result = *this;

		for (auto& item : result.items) {
			if (std::holds_alternative<ArgumentList>(item)) {
				item = std::get<ArgumentList>(item).simplified(replacements);
			}
		}

		auto modified = true;
		while (modified) {
			modified = false;

			for (auto const& [matcher, replacement] : replacements) {
				dbg_print("\n - trying '{}' on '{}'\n", matcher.repr(),
				          result.repr());
				auto match = matcher.matches(result);
				if (!match) continue;
				auto next_result =
				    replacement.replace_with(*match, result.cvrs);

				dbg_print("   > matched as {} with [{}]\n", next_result.str(),
				          match->size());

#ifdef DEBUG
				for (auto const& [key, expr] : *match) {
					dbg_print("     - [{}] = {}\n", key, expr.repr());
				}
#endif
				result = next_result;
				modified = true;
				break;
			}
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	std::string Statement::str() const {
		std::string result{};
		all_but_first space{.sep = " "sv};
		for (auto const& expr : items) {
			result.append(space.get());
			result.append(expr.str());
		}
		return result;
	}  // GCOV_EXCL_LINE[GCC]

	std::string Statement::repr() const {
		std::string result{};
		all_but_first comma{.sep = " "sv};
		for (auto const& expr : items) {
			result.append(comma.get());
			result.append(expr.repr());
		}
		return result;
	}  // GCOV_EXCL_LINE[GCC]

	bool Statement::matches(Statement const& oth, Refs& refs) const noexcept {
		if (items.size() != oth.items.size()) {
			dbg_print("   > '{}' and '{}' have mismatching lengths: {} vs {}\n",
			          str(), oth.str(), items.size(), oth.items.size());
			return false;
		}

		auto it = oth.items.begin();
		for (auto const& left : items) {
			auto const& right = *it++;
			if (!left.matches(right, refs, cvr_eq::important)) {
				return false;
			}
		}

		return true;
	}

	Statement Statement::replace_with(Refs const& refs) const {
		auto result = *this;
		if (!result.items.empty()) {
			auto& item = result.items.back();
			item = item.replace_with(refs, item.cvrs);
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

	Statement Statement::simplified(Replacements const& replacements) const {
		auto result = *this;
		for (auto& item : result.items) {
			item = item.simplified(replacements);
		}

		return result;
	}  // GCOV_EXCL_LINE[GCC]

}  // namespace cxx_filt
