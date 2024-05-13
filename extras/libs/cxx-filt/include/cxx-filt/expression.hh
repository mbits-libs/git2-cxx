// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cxx-filt/types.hh>
#include <map>
#include <optional>
#include <vector>

namespace cxx_filt {

	struct CvrLevel {
		bool has_const{false};
		bool has_volatile{false};
		Token level_end{};  // *, &, &&

		bool active() const noexcept {
			return has_const || has_volatile || level_end != Token::NONE;
		}

		bool operator==(CvrLevel const&) const noexcept = default;
	};

	struct Cvrs {
		std::vector<CvrLevel> levels{};

		void merge(Cvrs const& external) {
			levels.insert(levels.end(), external.levels.begin(),
			              external.levels.end());
			collapse();
		}

		void collapse() {}

		bool operator==(Cvrs const&) const noexcept = default;
	};

	struct Statement;
	struct Expression;
	struct ArgumentList;

	using Refs = std::map<std::string, Expression>;
	using Replacements = std::vector<std::pair<Expression, Expression>>;

	struct ArgumentList {
		Token start{}, stop{};
		std::vector<Statement> items{};

		std::string str() const;
		std::string repr() const;
		bool matches(ArgumentList const& oth, Refs& refs) const noexcept;
		ArgumentList replace_with(Refs const& refs) const;
		ArgumentList simplified(Replacements const& replacements) const;
	};

	enum class cvr_eq : bool {
		important = true,
		unimportant = false,
	};

	struct Expression {
		using Union = std::variant<std::string, Reference, ArgumentList>;

		std::vector<Union> items{};
		Cvrs cvrs{};

		Expression with(Cvrs const& external) const {
			auto copy = *this;
			copy.cvrs.merge(external);
			return copy;
		}  // GCOV_EXCL_LINE[GCC]

		std::string str() const;
		std::string repr() const;
		bool matches(Expression const& oth,
		             Refs& refs,
		             cvr_eq cvr_eq_policy) const noexcept;
		std::optional<Refs> matches(Expression const& oth) const noexcept;
		Expression replace_with(Refs const& refs, Cvrs const& cvrs) const;
		Expression simplified(Replacements const& replacements) const;
	};

	struct Statement {
		std::vector<Expression> items{};
		Cvrs const& cvrs() const { return items.front().cvrs; };

		Statement with(Cvrs const& external) const {
			auto copy = *this;
			copy.items.back().with(external);
			return copy;
		}

		std::string str() const;
		std::string repr() const;
		bool matches(Statement const& oth, Refs& refs) const noexcept;
		Statement replace_with(Refs const& refs) const;
		Statement simplified(Replacements const& replacements) const;
	};

}  // namespace cxx_filt
