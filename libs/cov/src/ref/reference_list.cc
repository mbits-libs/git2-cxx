// Copyright (c) 2022 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "../path-utils.hh"
#include "internal.hh"

namespace cov {
	using dir_iter = std::filesystem::recursive_directory_iterator;

	class reference_list_impl : public counted_impl<reference_list> {
	public:
		reference_list_impl(std::filesystem::path const& path,
		                    std::string const& prefix,
		                    ref_ptr<references> const& source)
		    : root_{path}, prefix_{prefix}, source_{source} {}

		ref_ptr<reference> next() noexcept override {
			while (iter_ != dir_iter{}) {
				auto const entry = *iter_;
				++iter_;
				if (!entry.is_regular_file()) continue;
				auto const rel = rel_path(entry.path(), root_);
				auto item = source_->lookup(get_path(rel));
				if (item) return item;
			}  // GCOV_EXCL_LINE[WIN32]

			return {};
		}

	private:
		dir_iter prepare(std::filesystem::path const& path) {
			std::error_code ec{};
			dir_iter result{path, ec};
			if (ec) return {};
			return result;
		}
		std::filesystem::path root_;
		std::string prefix_;
		dir_iter iter_{prepare(root_ / make_path(prefix_))};
		ref_ptr<references> source_;
	};

	ref_ptr<reference_list> reference_list_create(
	    std::filesystem::path const& path,
	    std::string const& prefix,
	    ref_ptr<references> const& source) {
		return make_ref<reference_list_impl>(path, prefix, source);
	}
}  // namespace cov
