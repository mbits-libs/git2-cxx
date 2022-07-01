// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once
#include <concepts>
#include <cov/io/types.hh>
#include <cov/object.hh>
#include <cov/streams.hh>
#include <memory>
#include <unordered_map>

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

	class db_object {
	public:
		db_object() = default;

		template <uint32_t magic, std::derived_from<db_handler> Handler>
		void add_handler() {
			return add_handler(magic, std::make_unique<Handler>());
		}
		template <OBJECT magic, std::derived_from<db_handler> Handler>
		void add_handler() {
			return add_handler(magic, std::make_unique<Handler>());
		}
		void add_handler(uint32_t magic, std::unique_ptr<db_handler>&& handler);
		void add_handler(OBJECT magic, std::unique_ptr<db_handler>&& handler) {
			return add_handler(static_cast<uint32_t>(magic),
			                   std::move(handler));
		}
		void remove_handler(uint32_t magic);
		void remove_handler(OBJECT magic) {
			return remove_handler(static_cast<uint32_t>(magic));
		}

		ref_ptr<counted> load(read_stream& in, std::error_code& ec) const;
		bool store(ref_ptr<counted> const& value, write_stream& in) const;

	private:
		std::unordered_map<uint32_t, std::unique_ptr<db_handler>> handlers_{};
	};
}  // namespace cov::io

namespace std {
	template <>
	struct is_error_condition_enum<cov::io::errc> : public std::true_type {};
}  // namespace std
