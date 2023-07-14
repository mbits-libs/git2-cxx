// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <cov/io/db_object.hh>

namespace cov::io {
	db_handler::~db_handler() = default;

	void db_object::add_handler(uint32_t magic,
	                            std::unique_ptr<db_handler>&& handler,
	                            uint32_t version) {
		if (!handler) {
			remove_handler(magic);
			return;
		}

		auto& node = handlers_[magic];
		node.first = version;
		node.second = std::move(handler);
	}

	void db_object::remove_handler(uint32_t magic) { handlers_.erase(magic); }

	ref_ptr<counted> db_object::load(git::oid_view id,
	                                 read_stream& in,
	                                 std::error_code& ec) const {
		auto const error = [&ec](errc code) -> ref_ptr<counted> {
			ec = make_error_code(code);
			return {};
		};

		file_header hdr{};
		if (!in.load(hdr)) return error(errc::bad_syntax);

		auto [version, handler] = [&] {  // GCOV_EXCL_LINE[GCC]
			uint32_t version = v1::VERSION;
			db_handler* handler = nullptr;
			auto it = handlers_.find(hdr.magic);
			if (it != handlers_.end()) {
				auto const& node = it->second;
				version = node.first;
				handler = node.second.get();
			}
			return std::pair{version, handler};
		}();

		if ((hdr.version & VERSION_MAJOR) != (version & VERSION_MAJOR))
			return error(errc::unsupported_version);
		if (!handler) return error(errc::unknown_magic);

		return handler->load(hdr.magic, hdr.version, id, in, ec);
	}

	bool db_object::store(ref_ptr<counted> const& value,
	                      write_stream& out) const {
		if (!value) return false;

		for (auto const& [magic, pair] : handlers_) {
			auto const& [version, handler] = pair;
			if (!handler->recognized(value)) continue;
			file_header hdr{.magic = magic, .version = version};
			if (!out.store(hdr)) return false;
			return handler->store(value, out);
		}  // GCOV_EXCL_LINE[WIN32]

		return false;
	}
}  // namespace cov::io
