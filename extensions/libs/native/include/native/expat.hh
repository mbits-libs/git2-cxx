// Copyright (c) 2023 Marcin Zdun
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <expat.h>
#include <cstring>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
namespace xml {
	template <class Final>
	class ExpatBase {
	public:
		ExpatBase() : parser_(nullptr) {}
		~ExpatBase() { destroy(); }

		bool create(const XML_Char* encoding = nullptr,
		            const XML_Char* sep = nullptr) {
			destroy();
			if (encoding != nullptr && encoding[0] == 0) encoding = nullptr;

			if (sep != nullptr && sep[0] == 0) sep = nullptr;

			parser_ = XML_ParserCreate_MM(encoding, nullptr, sep);
			if (parser_ == nullptr) return false;

			Final* pThis = static_cast<Final*>(this);
			pThis->onPostCreate();

			XML_SetUserData(parser_, static_cast<void*>(pThis));
			return true;
		}

		bool isCreated() const { return parser_ != nullptr; }

		void destroy() {
			if (parser_) XML_ParserFree(parser_);
			parser_ = nullptr;
		}

		bool parse(const char* buffer, int length = -1, bool isFinal = true) {
			if (length < 0) length = static_cast<int>(std::strlen(buffer));

			return XML_Parse(parser_, buffer, length, isFinal) != 0;
		}

		bool parseBuffer(int length, bool isFinal = true) {
			return XML_ParseBuffer(parser_, length, isFinal) != 0;
		}

		void* getBuffer(int length) { return XML_GetBuffer(parser_, length); }

		void enableStartElementHandler(bool enable = true) {
			XML_SetStartElementHandler(parser_,
			                           enable ? startElementHandler : nullptr);
		}

		void enableEndElementHandler(bool enable = true) {
			XML_SetEndElementHandler(parser_,
			                         enable ? endElementHandler : nullptr);
		}

		void enableElementHandler(bool enable = true) {
			enableStartElementHandler(enable);
			enableEndElementHandler(enable);
		}

		void enableCharacterDataHandler(bool enable = true) {
			XML_SetCharacterDataHandler(
			    parser_, enable ? characterDataHandler : nullptr);
		}

		void enableProcessingInstructionHandler(bool enable = true) {
			XML_SetProcessingInstructionHandler(
			    parser_, enable ? ProcessingInstructionHandler : nullptr);
		}

		void enableCommentHandler(bool enable = true) {
			XML_SetCommentHandler(parser_, enable ? commentHandler : nullptr);
		}

		void enableStartCdataSectionHandler(bool enable = true) {
			XML_SetStartCdataSectionHandler(
			    parser_, enable ? startCdataSectionHandler : nullptr);
		}

		void enableEndCdataSectionHandler(bool enable = true) {
			XML_SetEndCdataSectionHandler(
			    parser_, enable ? endCdataSectionHandler : nullptr);
		}

		void enableCdataSectionHandler(bool enable = true) {
			enableStartCdataSectionHandler(enable);
			enableEndCdataSectionHandler(enable);
		}

		void enableDefaultHandler(bool enable = true, bool expand = true) {
			if (expand) {
				XML_SetDefaultHandlerExpand(parser_,
				                            enable ? defaultHandler : nullptr);
			} else {
				XML_SetDefaultHandler(parser_,
				                      enable ? defaultHandler : nullptr);
			}
		}

		void enableExternalEntityRefHandler(bool enable = true) {
			XML_SetExternalEntityRefHandler(
			    parser_, enable ? externalEntityRefHandler : nullptr);
		}

		void enableUnknownEncodingHandler(bool enable = true) {
			Final* pThis = static_cast<Final*>(this);
			XML_SetUnknownEncodingHandler(
			    parser_, enable ? unknownEncodingHandler : nullptr,
			    enable ? static_cast<void*>(pThis) : nullptr);
		}

		void enableStartNamespaceDeclHandler(bool enable = true) {
			XML_SetStartNamespaceDeclHandler(
			    parser_, enable ? startNamespaceDeclHandler : nullptr);
		}

		void enableEndNamespaceDeclHandler(bool enable = true) {
			XML_SetEndNamespaceDeclHandler(
			    parser_, enable ? endNamespaceDeclHandler : nullptr);
		}

		void enableNamespaceDeclHandler(bool enable = true) {
			enableStartNamespaceDeclHandler(enable);
			enableEndNamespaceDeclHandler(enable);
		}

		void enableXmlDeclHandler(bool enable = true) {
			XML_SetXmlDeclHandler(parser_, enable ? xmlDeclHandler : nullptr);
		}

		void enableStartDoctypeDeclHandler(bool enable = true) {
			XML_SetStartDoctypeDeclHandler(
			    parser_, enable ? startDoctypeDeclHandler : nullptr);
		}

		void enableEndDoctypeDeclHandler(bool enable = true) {
			XML_SetEndDoctypeDeclHandler(
			    parser_, enable ? endDoctypeDeclHandler : nullptr);
		}

		void enableDoctypeDeclHandler(bool enable = true) {
			enableStartDoctypeDeclHandler(enable);
			enableEndDoctypeDeclHandler(enable);
		}

		enum XML_Error getErrorCode() { return XML_GetErrorCode(parser_); }

		long getCurrentByteIndex() { return XML_GetCurrentByteIndex(parser_); }

		int getCurrentLineNumber() { return XML_GetCurrentLineNumber(parser_); }

		int getCurrentColumnNumber() {
			return XML_GetCurrentColumnNumber(parser_);
		}

		int getCurrentByteCount() { return XML_GetCurrentByteCount(parser_); }

		const char* getInputContext(int* offset, int* size) {
			return XML_GetInputContext(parser_, offset, size);
		}

		const XML_LChar* getErrorString() {
			return XML_ErrorString(getErrorCode());
		}

		static const XML_LChar* getExpatVersion() { return XML_ExpatVersion(); }

		static XML_Expat_Version getExpatVersionInfo() {
			return XML_ExpatVersionInfo();
		}

		static const XML_LChar* getErrorString(enum XML_Error error) {
			return XML_ErrorString(error);
		}

		void onStartElement(const XML_Char* name, const XML_Char** attrs) {}

		void onEndElement(const XML_Char* name) {}

		void onCharacterData(const XML_Char* data, int length) {}

		void onProcessingInstruction(const XML_Char* target,
		                             const XML_Char* data) {}

		void onComment(const XML_Char* data) {}

		void onStartCdataSection() {}

		void onEndCdataSection() {}

		void onDefault(const XML_Char* data, int length) {}

		bool onExternalEntityRef(const XML_Char* context,
		                         const XML_Char* base,
		                         const XML_Char* systemID,
		                         const XML_Char* publicID) {
			return false;
		}

		bool onUnknownEncoding(const XML_Char* name, XML_Encoding* info) {
			return false;
		}

		void onStartNamespaceDecl(const XML_Char* prefix, const XML_Char* uri) {
		}

		void onEndNamespaceDecl(const XML_Char* prefix) {}

		void onXmlDecl(const XML_Char* version,
		               const XML_Char* encoding,
		               bool standalone) {}

		void onStartDoctypeDecl(const XML_Char* doctypeName,
		                        const XML_Char* systemId,
		                        const XML_Char* publicID,
		                        bool hasInternalSubset) {}

		void onEndDoctypeDecl() {}

	protected:
		void onPostCreate() {}

		static auto ptr(void* userData) {
			return static_cast<Final*>(userData);
		}

		template <auto true_value, auto false_value, typename boolean_like>
		static auto bool2(boolean_like result) {
			return result ? true_value : false_value;
		}

		static void startElementHandler(void* userData,
		                                const XML_Char* name,
		                                const XML_Char** attrs) {
			ptr(userData)->onStartElement(name, attrs);
		}

		static void endElementHandler(void* userData, const XML_Char* name) {
			ptr(userData)->onEndElement(name);
		}

		static void characterDataHandler(void* userData,
		                                 const XML_Char* data,
		                                 int length) {
			ptr(userData)->onCharacterData(data, length);
		}

		static void ProcessingInstructionHandler(void* userData,
		                                         const XML_Char* target,
		                                         const XML_Char* data) {
			ptr(userData)->onProcessingInstruction(target, data);
		}

		static void commentHandler(void* userData, const XML_Char* data) {
			ptr(userData)->onComment(data);
		}

		static void startCdataSectionHandler(void* userData) {
			ptr(userData)->onStartCdataSection();
		}

		static void endCdataSectionHandler(void* userData) {
			ptr(userData)->onEndCdataSection();
		}

		static void defaultHandler(void* userData,
		                           const XML_Char* data,
		                           int length) {
			ptr(userData)->onDefault(data, length);
		}

		static int externalEntityRefHandler(XML_Parser parser,
		                                    const XML_Char* context,
		                                    const XML_Char* base,
		                                    const XML_Char* systemID,
		                                    const XML_Char* publicID) {
			auto const userData = XML_GetUserData(parser);
			return bool2<1, 0>(ptr(userData)->onExternalEntityRef(
			    context, base, systemID, publicID));
		}

		static int unknownEncodingHandler(void* userData,
		                                  const XML_Char* name,
		                                  XML_Encoding* info) {
			return bool2<XML_STATUS_OK, XML_STATUS_ERROR>(
			    ptr(userData)->onUnknownEncoding(name, info));
		}

		static void startNamespaceDeclHandler(void* userData,
		                                      const XML_Char* prefix,
		                                      const XML_Char* uri) {
			ptr(userData)->onStartNamespaceDecl(prefix, uri);
		}

		static void endNamespaceDeclHandler(void* userData,
		                                    const XML_Char* prefix) {
			ptr(userData)->onEndNamespaceDecl(prefix);
		}

		static void xmlDeclHandler(void* userData,
		                           const XML_Char* version,
		                           const XML_Char* encoding,
		                           int standalone) {
			ptr(userData)->onXmlDecl(version, encoding, standalone != 0);
		}

		static void startDoctypeDeclHandler(void* userData,
		                                    const XML_Char* doctypeName,
		                                    const XML_Char* systemId,
		                                    const XML_Char* publicID,
		                                    int hasInternalSubset) {
			ptr(userData)->onStartDoctypeDecl(doctypeName, systemId, publicID,
			                                  hasInternalSubset != 0);
		}

		static void endDoctypeDeclHandler(void* userData) {
			ptr(userData)->onEndDoctypeDecl();
		}

		XML_Parser parser_;
	};
}  // namespace xml
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
