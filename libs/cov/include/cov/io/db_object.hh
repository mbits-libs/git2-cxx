// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <concepts>
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <cov/streams.hh>
#include <memory>
#include <unordered_map>
#include <utility>

namespace cov::io {
	enum class errc : int {
		bad_syntax = 1,
		unknown_magic,
		unsupported_version,
	};

	std::error_category const& category();

	inline std::error_code make_error_code(errc git_error) {
		return std::error_code(static_cast<int>(git_error),
		                       cov::io::category());
	}
	inline std::error_condition make_error_condition(errc git_error) {
		return std::error_condition(static_cast<int>(git_error),
		                            cov::io::category());
	}

	struct db_handler {
		virtual ~db_handler();

		virtual ref_ptr<counted> load(uint32_t magic,
		                              uint32_t version,
		                              git_oid const& id,
		                              read_stream& in,
		                              std::error_code& ec) const = 0;
		virtual bool recognized(ref_ptr<counted> const& obj) const = 0;
		virtual bool store(ref_ptr<counted> const& obj,
		                   write_stream& in) const = 0;
	};

	template <typename Handled>
	struct db_handler_for : db_handler {
		bool recognized(ref_ptr<counted> const& obj) const override {
			if (!obj || !obj->is_object()) return false;
			return is_a<Handled>(static_cast<object const*>(obj.get()));
		}
	};

	struct OBJECT_or_uint {
		uint32_t value{};
		constexpr OBJECT_or_uint() = default;
		constexpr OBJECT_or_uint(OBJECT value)
		    : value{static_cast<uint32_t>(value)} {}
		constexpr OBJECT_or_uint(uint32_t value) : value{value} {}
	};

	template <OBJECT_or_uint>
	struct version_from_t : std::integral_constant<uint32_t, v1::VERSION> {};

	template <OBJECT_or_uint TAG>
	constexpr auto version_from = version_from_t<TAG>::value;

	template <>
	struct version_from_t<OBJECT::FILES>
	    : std::integral_constant<uint32_t, io::VERSION_v1_1> {};

	class db_object {
	public:
		db_object() = default;

		template <uint32_t magic, std::derived_from<db_handler> Handler>
		void add_handler() {
			return add_handler(magic, std::make_unique<Handler>(),
			                   version_from<magic>);
		}
		template <OBJECT magic, std::derived_from<db_handler> Handler>
		void add_handler() {
			return add_handler(magic, std::make_unique<Handler>(),
			                   version_from<magic>);
		}
		void add_handler(uint32_t magic,
		                 std::unique_ptr<db_handler>&& handler,
		                 uint32_t version = v1::VERSION);
		void add_handler(OBJECT magic,
		                 std::unique_ptr<db_handler>&& handler,
		                 uint32_t version = v1::VERSION) {
			return add_handler(static_cast<uint32_t>(magic), std::move(handler),
			                   version);
		}
		void remove_handler(uint32_t magic);
		void remove_handler(OBJECT magic) {
			return remove_handler(static_cast<uint32_t>(magic));
		}

		ref_ptr<counted> load(git_oid const& id,
		                      read_stream& in,
		                      std::error_code& ec) const;
		bool store(ref_ptr<counted> const& value, write_stream& in) const;

	private:
		std::unordered_map<uint32_t,
		                   std::pair<uint32_t, std::unique_ptr<db_handler>>>
		    handlers_{};
	};
}  // namespace cov::io

namespace std {
	template <>
	struct is_error_condition_enum<cov::io::errc> : public std::true_type {};
}  // namespace std
