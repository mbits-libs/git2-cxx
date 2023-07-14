// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <fmt/format.h>
#include <git2/oid.h>
#include <iostream>
#include <string>

namespace git {
	template <typename Holder>
	struct basic_oid {
		std::string str() const {
			char buffer[GIT_OID_HEXSZ];
			git_oid_fmt(buffer, id_());
			return {buffer, buffer + GIT_OID_HEXSZ};
		}

		std::string path() const {
			char buffer[GIT_OID_HEXSZ + 1];
			git_oid_pathfmt(buffer, id_());
			return {buffer, buffer + GIT_OID_HEXSZ + 1};
		}

		bool is_zero() const noexcept { return git_oid_is_zero(id_()) != 0; }

		friend std::ostream& operator<<(std::ostream& out,
		                                basic_oid<Holder> const& id) {
			return out << id.str();
		}

	private:
		git_oid const* id_() const noexcept {
			return static_cast<Holder const*>(this)->ptr();
		}
	};

	struct oid : basic_oid<oid> {
		git_oid id{};
		oid() = default;
		explicit oid(git_oid const& ref) : id{ref} {}
		void assign(git_oid const& ref) { id = ref; }

		git_oid const* ptr() const noexcept { return &id; }

		bool operator==(oid const& rhs) const noexcept {
			return git_oid_equal(&id, &rhs.id);
		}

		inline bool operator==(git_oid const& rhs) const noexcept;
	};

	struct oid_view : basic_oid<oid_view> {
		git_oid const* ref{};
		oid_view() = delete;
		oid_view(git_oid const& ref) : ref{&ref} {}
		oid_view(git::oid const& ref) : ref{&ref.id} {}

		git_oid const* ptr() const noexcept { return ref; }

		bool operator==(oid_view const& rhs) const noexcept {
			return git_oid_equal(ref, rhs.ref);
		}

		bool operator==(git::oid const& rhs) const noexcept {
			return *this == oid_view{rhs};
		}

		bool operator==(git_oid const& rhs) const noexcept {
			return *this == oid_view{rhs};
		}

		bool operator==(git_oid const* rhs) const noexcept {
			return rhs && *this == oid_view{*rhs};
		}

		git::oid oid() const { return git::oid{*ref}; }
	};

	inline bool oid::operator==(git_oid const& rhs) const noexcept {
		return oid_view{*this} == oid_view{rhs};
	}
}  // namespace git

namespace fmt {
	template <typename Holder>
	struct formatter_impl {
		constexpr auto parse(format_parse_context& ctx)
		    -> decltype(ctx.begin()) {
			auto it = ctx.begin(), end = ctx.end();
			while (it != end && *it != '}')
				++it;
			return it;
		}

		template <typename FormatContext>
		constexpr auto format(git::basic_oid<Holder> const& id,
		                      FormatContext& ctx) const -> decltype(ctx.out()) {
			return fmt::format_to(ctx.out(), "{}", id.str());
		}
	};

	template <>
	struct formatter<git::oid> : formatter_impl<git::oid> {};

	template <>
	struct formatter<git::oid_view> : formatter_impl<git::oid_view> {};

	template <>
	struct formatter<git_oid> {
		constexpr auto parse(format_parse_context& ctx)
		    -> decltype(ctx.begin()) {
			auto it = ctx.begin(), end = ctx.end();
			while (it != end && *it != '}')
				++it;
			return it;
		}

		template <typename FormatContext>
		constexpr auto format(git_oid const& id, FormatContext& ctx) const
		    -> decltype(ctx.out()) {
			return fmt::format_to(ctx.out(), "{}", git::oid_view{id}.str());
		}
	};
}  // namespace fmt
