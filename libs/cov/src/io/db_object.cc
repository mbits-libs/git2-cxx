// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/db_object.hh>

namespace cov::io {
	db_handler::~db_handler() = default;

	void db_object::add_handler(uint32_t magic,
	                            std::unique_ptr<db_handler>&& handler) {
		if (!handler) {
			remove_handler(magic);
			return;
		}

		handlers_[magic] = std::move(handler);
	}

	void db_object::remove_handler(uint32_t magic) { handlers_.erase(magic); }

	ref<counted> db_object::load(read_stream& in, std::error_code& ec) const {
		auto const error = [&ec](errc code) -> ref<counted> {
			ec = make_error_code(code);
			return {};
		};

		file_header hdr{};
		if (!in.load(hdr)) return error(errc::bad_syntax);
		if ((hdr.version & VERSION_MAJOR) != v1::VERSION)
			return error(errc::unsupported_version);

		auto handler = [&] {
			db_handler* result = nullptr;
			auto it = handlers_.find(hdr.magic);
			if (it != handlers_.end()) result = it->second.get();
			return result;
		}();
		if (!handler) return error(errc::unknown_magic);

		return handler->load(hdr.magic, hdr.version, in, ec);
	}

	bool db_object::store(ref<counted> const& value, write_stream& out) const {
		if (!value) return false;

		for (auto const& [magic, handler] : handlers_) {
			if (!handler->recognized(value)) continue;
			file_header hdr{.magic = magic, .version = v1::VERSION};
			if (!out.store(hdr)) return false;
			return handler->store(value, out);
		}

		return false;
	}
}  // namespace cov::io
