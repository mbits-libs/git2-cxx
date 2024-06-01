// Copyright (c) 2024 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include <fmt/format.h>
#include <arch/io/file.hh>
#include <arch/unpacker.hh>
#include <cov/tz.hh>
#include <mutex>

namespace arch {
	// GCOV_EXCL_START

	// Code below is _only_ called, when a TZ database needs to be unpacked.
	// Which is to say, almost never. So, consequently, it lines are never being
	// ran through.

	static inline std::filesystem::path make_u8path(std::string_view u8) {
		return std::u8string_view{reinterpret_cast<char8_t const*>(u8.data()),
		                          u8.size()};
	}

	static inline std::string get_u8path(std::filesystem::path copy) {
		copy.make_preferred();
		auto u8 = copy.u8string();
		return {reinterpret_cast<char const*>(u8.data()), u8.size()};
	}

	class expand_unpacker : public unpacker {
	public:
		using unpacker::unpacker;

		// This code works best if it is never called...
		void on_error(fs::path const& filename,
		              char const* msg) const override {
			fmt::print(stderr, "expand: {}: {}\n", get_u8path(filename), msg);
		}
		void on_note(char const* msg) const override {
			fmt::print(stderr, "        note: {}\n", msg);
		}
	};

	bool extract_gz_file(const std::string& /*version*/,
	                     const std::string& gz_file,
	                     const std::string& dest_folder) {
		auto file = io::file::open(make_u8path(gz_file));
		if (!file) return false;

		base::archive::ptr archive{};
		auto status = open(std::move(file), archive);
		if (status != open_status::ok) return false;

		expand_unpacker unp{make_u8path(dest_folder)};

		return unp.unpack(*archive);
	}
	// GCOV_EXCL_STOP

}  // namespace arch

namespace cov::init_magic {
	namespace {
		std::once_flag flag{};

		void setup_archives() {
			date::set_tar_gz_helper(arch::extract_gz_file);
		}
	}  // namespace

	setup_tz_archives::setup_tz_archives() {
		std::call_once(flag, setup_archives);
	}
}  // namespace cov::init_magic
