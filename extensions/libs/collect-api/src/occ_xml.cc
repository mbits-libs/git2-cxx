// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#include "occ_xml.hh"
#include <cov/app/path.hh>
#include <cov/io/file.hh>
#include <stack>

// define DEBUG_IGNORE

#define SIMPLE_PARENT(PARENT, ...)                                      \
	struct PARENT##_handler : handler_impl<__VA_ARGS__> {               \
		using handler_impl<__VA_ARGS__>::handler_impl;                  \
		static std::string_view name() noexcept { return #PARENT##sv; } \
	};

using namespace std::literals;

namespace cov::app::collect {
	std::optional<unsigned> parse_uint(std::string_view text) {
		unsigned value{};
		auto first = text.data();
		auto last = first + text.size();
		auto [end, err] = std::from_chars(first, last, value);
		if (end != last || err != std::errc{}) return std::nullopt;
		return value;
	}

	struct line_handler : handler_interface {
		using handler_interface::handler_interface;
		static std::string_view name() noexcept { return "line"sv; }
		void onElement(char const** attrs) override {
			if (!ctx->sink) return;

			std::optional<unsigned> number, hits;

			for (auto attr = attrs; *attr; attr += 2) {
				auto const name = std::string_view{attr[0]};
				if (name == "number"sv) {
					number = parse_uint(attr[1]);
				} else if (name == "hits"sv) {
					hits = parse_uint(attr[1]);
				}
				if (number && hits) break;
			}

			if (number && hits) ctx->sink->on_visited(*number, *hits);
		}
	};

	SIMPLE_PARENT(lines, line_handler);
	SIMPLE_PARENT(methods);

	struct class_handler : handler_interface {
		using handler_interface::handler_interface;
		static std::string_view name() noexcept { return "class"sv; }
		void onElement(char const** attrs) override {
			for (auto attr = attrs; *attr; attr += 2) {
				auto const name = std::string_view{attr[0]};
				if (name == "filename"sv) {
					auto path = get_u8path(make_u8path(ctx->source) /
					                       make_u8path(attr[1]));
					ctx->sink = ctx->cvg.get_file_mt(path);
					break;
				}
			}
		}

		void onStop() override {
			if (ctx->sink) ctx->cvg.handover_mt(std::move(ctx->sink));
			ctx->sink.reset();
		}

		std::unique_ptr<handler_interface> onChild(
		    std::string_view name) override {
			if (ctx->sink)
				return find_helper<lines_handler, methods_handler>::find(name,
				                                                         ctx);
			return {};
		}
	};

	struct source_handler : handler_interface {
		using handler_interface::handler_interface;
		static std::string_view name() noexcept { return "source"sv; }
		void onElement(char const**) override { ctx->source.clear(); }

		void onCharacter(std::string_view chunk) override {
			ctx->source.append(chunk);
		}

		void onStop() override {
			if (ctx->source.empty() || ctx->source.back() != '/')
				ctx->source.push_back('/');
		}
	};

	SIMPLE_PARENT(classes, class_handler);
	SIMPLE_PARENT(package, classes_handler);
	SIMPLE_PARENT(packages, package_handler);
	SIMPLE_PARENT(sources, source_handler)
	SIMPLE_PARENT(coverage, packages_handler, sources_handler);
	SIMPLE_PARENT(document, coverage_handler);

	class parser : public xml::ExpatBase<parser> {
		std::stack<std::unique_ptr<handler_interface>> handlers_{};
		int ignore_depth_{0};

	public:
		parser(msvc_context* ctx) {
			handlers_.push(std::make_unique<document_handler>(ctx));
		}

		static bool load(std::filesystem::path const& filename,
		                 config const& cfg,
		                 coverage& cvg) {
			auto file = io::fopen(filename);
			if (!file) return false;

			msvc_context ctx{.cfg = cfg, .cvg = cvg};
			parser ldr{&ctx};

			if (!ldr.create()) return false;
			ldr.enableElementHandler();
			ldr.enableCdataSectionHandler();
			ldr.enableCharacterDataHandler();

			char buffer[8196];
			while (auto const byte_count = file.load(buffer, sizeof(buffer))) {
				ldr.parse(buffer, static_cast<int>(byte_count), false);
			}
			ldr.parse(buffer, 0, true);

			return true;
		}

		void onStartElement(char const* name, char const** attrs) {
			if (!ignore_depth_) {
				auto const tag = std::string_view{name};
				auto current = handlers_.top().get();
				auto next = current->onChild(tag);
				if (next) {
					next->onElement(attrs);
					handlers_.push(std::move(next));
					return;
				}
			}
#if defined(DEBUG_IGNORE)
			if (!ignore_depth_) {
				fmt::print(stderr, "[{}] >>> {}", ignore_depth_, name);
				for (auto attr = attrs; *attr; attr += 2) {
					fmt::print(stderr, " {}=\"{}\"", attr[0], attr[1]);
				}
				fmt::print(stderr, "\n");
			}
#endif

			++ignore_depth_;
		}

		void onEndElement([[maybe_unused]] char const* name) {
			if (ignore_depth_) {
				--ignore_depth_;
#if defined(DEBUG_IGNORE)
				if (!ignore_depth_)
					fmt::print(stderr, "[{}] <<< {}\n", ignore_depth_, name);
#endif
				return;
			}

			auto current = std::move(handlers_.top());
			handlers_.pop();
			current->onStop();
		}

		void onCharacterData(char const* data, int length) {
			if (ignore_depth_) return;
			auto current = handlers_.top().get();
			current->onCharacter(
			    std::string_view{data, static_cast<size_t>(length)});
		}
	};

	handler_interface::~handler_interface() = default;
	void handler_interface::onElement(char const**) {}
	std::unique_ptr<handler_interface> handler_interface::onChild(
	    std::string_view) {
		return {};
	}
	void handler_interface::onCharacter(std::string_view) {}
	void handler_interface::onStop() {}

	bool occ_load(std::filesystem::path const& filename,
	              config const& cfg,
	              coverage& cvg) {
		return parser::load(filename, cfg, cvg);
	}

}  // namespace cov::app::collect
