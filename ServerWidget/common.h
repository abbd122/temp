#pragma once

#include <string>
#include <map>
#include <regex>
#include <vector>
#include <xutility>
#include <functional>
#include <iostream>

#include "common.h"

#ifdef _WIN64
using ssize_t = __int64;
#else
using ssize_t = long;
#endif

#ifndef CPPHTTPLIB_REDIRECT_MAX_COUNT
#define CPPHTTPLIB_REDIRECT_MAX_COUNT 20
#endif

struct CompareIgnoreCase {
	bool operator()(const std::string& s1, const std::string& s2) const
	{
		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
		                                    [](unsigned char c1, unsigned char c2) {
			                                    return std::tolower(c1) < std::tolower(c2);
		                                    });
	}
};

struct MultiPartFormData {
	std::string name;

	std::string content;

	std::string filename;

	std::string contentType;
};

using MultiPartFormDataItems = std::vector<MultiPartFormData>;

using MultiPartFormDataMap = std::multimap<std::string, MultiPartFormData>;

using Headers = std::multimap<std::string, std::string, CompareIgnoreCase>;

using Params = std::multimap<std::string, std::string>;

using Range = std::pair<ssize_t, ssize_t>;

using Ranges = std::vector<Range>;

using Match = std::smatch;

struct Response;

using ResponseHandler = std::function<bool(const Response& response)>;

using ContentReceiverWithProgress = std::function<bool(const char* data, size_t dataLength,
                                                       uint64_t offset, uint64_t totalLength)>;

using Progress = std::function<bool(uint64_t current, uint64_t total)>;

class DataSink {
public:
	DataSink();

	DataSink& operator=(const DataSink&) = delete;

	DataSink(DataSink&&) = delete;

	DataSink& operator=(DataSink&&) = delete;

	std::function<bool(const char* data, size_t len)> m_write;

	std::function<void()> m_done;

	std::function<bool()> m_isWritable;

	std::ostream m_os;

private:
	class DataSinkStreamBuf : public std::streambuf {
	public:
		explicit DataSinkStreamBuf(DataSink& sink): m_sink(sink)
		{
		}

	protected:
		std::streamsize xsputn(const char* _Ptr, std::streamsize _Count) override;

	private:
		DataSink& m_sink;
	};

	DataSinkStreamBuf m_buf;
};

using ContentProvider = std::function<bool(size_t offset, size_t length, DataSink& sink)>;

using ContentProviderWithoutLength = std::function<bool(size_t offset, DataSink& sink)>;

using ContentProviderResourceReleaser = std::function<void(bool success)>;

class ContentProviderAdapter {
public:
	explicit ContentProviderAdapter(ContentProviderWithoutLength&& provider);

	bool operator()(size_t offset, size_t length, DataSink& sink);

private:
	ContentProviderWithoutLength contentProvider;
};

using ContentReceiver = std::function<bool(const char* data, size_t data_length)>;

using MultipartContentHeader = std::function<bool(const MultiPartFormData& file)>;

class ContentReader {
public:
	using Reader = std::function<bool(ContentReceiver receiver)>;
	using MultiPartReader = std::function<bool(MultipartContentHeader header,
	                                           ContentReceiver receiver)>;

	ContentReader(Reader reader, MultiPartReader multipart_reader)
		: m_reader(std::move(reader)),
		  m_multiPartReader(std::move(multipart_reader))
	{
	}

	bool operator()(MultipartContentHeader header,
	                ContentReceiver receiver) const
	{
		return m_multiPartReader(std::move(header), std::move(receiver));
	}

	bool operator()(ContentReceiver receiver) const
	{
		return m_reader(std::move(receiver));
	}

	Reader m_reader;

	MultiPartReader m_multiPartReader;
};

inline bool HasHeader(const Headers& headers, const std::string& key)
{
	return headers.find(key) != headers.end();
}

inline const char* GetHeaderValue(const Headers& headers, const std::string& key, size_t id,
                                  const char* def)
{
	auto rng = headers.equal_range(key);
	auto it = rng.first;
	std::advance(it, static_cast<ssize_t>(id));
	if (it != rng.second) {
		return it->second.c_str();
	}
	return def;
}

inline bool HasClrf(const std::string& key)
{
	auto p = key.c_str();
	while (*p) {
		if (*p == '\r' || *p == '\n') {
			return true;
		}
		++p;
	}
	return false;
}

struct Request {
	std::string method;

	std::string path;

	Headers headers;

	std::string body;

	std::string remoteAddr;

	int remotePort{-1};

	std::string localAddr;

	int localPort{-1};

	std::string version;

	std::string target;

	Params params;

	MultiPartFormDataMap files;

	Ranges ranges;

	Match matches;

	// for client
	ResponseHandler responseHandler;

	ContentReceiverWithProgress contentReceiver;

	Progress progress;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
	const SSL* ssl = nullptr;
#endif
	bool HasHeader(const std::string& key) const;

	std::string GetHeaderValue(const std::string& key, size_t id = 0) const;

	ssize_t GetHeaderValueCount(const std::string& key) const;

	void SetHeader(const std::string& key, const std::string& val);

	bool HasParams(const std::string& key);

	std::string GetParamsValue(const std::string& key, size_t id = 0);

	size_t GetParamsValueCount(const std::string& key);

	bool IsMultiPartFormData() const;

	bool HasFile(const std::string& key);

	MultiPartFormData GetFileValue(const std::string& key);

	size_t redirectCount = CPPHTTPLIB_REDIRECT_MAX_COUNT;

	size_t contentLength{};

	ContentProvider contentProvider;

	bool isChunkedContentProvider{};

	size_t authorizationCount;
};

struct Response {
	std::string version;

	int status{-1};

	std::string reason;

	Headers headers;

	std::string body;

	std::string location;

	bool HasHeader(const std::string& key) const;

	std::string GetHeaderValue(const std::string& key, size_t id = 0) const;

	size_t GetHeaderValueCount(const std::string& key) const;

	void SetHeader(const std::string& key, const std::string& val);

	void SetRedirect(const std::string& url, int status = 302);

	void SetContent(const char* str, size_t n, const std::string& contentType);

	void SetContent(const std::string& str, const std::string& contentType);

	void SetContentProvider(size_t length, const std::string& contentType, ContentProvider provider,
	                        ContentProviderResourceReleaser resourceReleaser = nullptr);

	void SetContentProvider(const std::string& contentType,
	                        ContentProviderWithoutLength provider,
	                        ContentProviderResourceReleaser resourceReleaser = nullptr);

	void SetChunckedContentProvider(const std::string& contentType,
	                                ContentProviderWithoutLength provider,
	                                ContentProviderResourceReleaser resourceReleaser = nullptr);

	Response() = default;

	Response(const Response&) = default;

	Response& operator=(const Response&) = default;

	Response(Response&&) = default;

	Response& operator=(Response&&) = default;

	~Response();

	// private
	size_t contentLength{};

	ContentProvider contentProvider;

	ContentProviderResourceReleaser contentProviderResourceReleaser;

	bool isChunkedContentProvider{};

	bool contentProviderSuccess{};
};

inline DataSink::DataSink(): m_os(&m_buf), m_buf(*this)
{
}

inline std::streamsize DataSink::DataSinkStreamBuf::xsputn(const char* _Ptr, std::streamsize _Count)
{
	m_sink.m_write(_Ptr, static_cast<size_t>(_Count));
	return _Count;
}

inline ContentProviderAdapter::ContentProviderAdapter(ContentProviderWithoutLength&& provider):
	contentProvider(provider)
{
}

inline bool ContentProviderAdapter::operator()(size_t offset, size_t length, DataSink& sink)
{
	return contentProvider(offset, sink);
}

inline bool Request::HasHeader(const std::string& key) const
{
	return ::HasHeader(headers, key);
}

inline std::string Request::GetHeaderValue(const std::string& key, size_t id) const
{
	return ::GetHeaderValue(headers, key, id, "");
}

inline ssize_t Request::GetHeaderValueCount(const std::string& key) const
{
	auto rng = headers.equal_range(key);
	return static_cast<ssize_t>(std::distance(rng.first, rng.second));
}

inline void Request::SetHeader(const std::string& key, const std::string& val)
{
	if (HasClrf(key) || HasClrf(val)) {
		return;
	}
	headers.emplace(key, val);
}

inline bool Request::HasParams(const std::string& key)
{
	return params.find(key) != params.end();
}

inline std::string Request::GetParamsValue(const std::string& key, size_t id)
{
	auto rng = params.equal_range(key);
	auto it = rng.first;
	std::advance(it, static_cast<ssize_t>(id));
	if (it != rng.second) {
		return it->second;
	}
	return {};
}

inline size_t Request::GetParamsValueCount(const std::string& key)
{
	auto rng = params.equal_range(key);
	return static_cast<size_t>(std::distance(rng.first, rng.second));
}

inline bool Request::IsMultiPartFormData() const
{
	const auto& contentType = GetHeaderValue("Content-Type");
	return contentType.rfind("multipart/form-data", 0) == 0;
}

inline bool Request::HasFile(const std::string& key)
{
	return files.find(key) != files.end();
}

inline MultiPartFormData Request::GetFileValue(const std::string& key)
{
	auto it = files.find(key);
	if (it != files.end()) {
		return it->second;
	}
	return {};
}

inline bool Response::HasHeader(const std::string& key) const
{
	return ::HasHeader(headers, key);
}

inline std::string Response::GetHeaderValue(const std::string& key, size_t id) const
{
	return ::GetHeaderValue(headers, key, id, "");
}

inline size_t Response::GetHeaderValueCount(const std::string& key) const
{
	auto rng = headers.equal_range(key);
	return static_cast<size_t>(std::distance(rng.first, rng.second));
}

inline void Response::SetHeader(const std::string& key, const std::string& val)
{
	if (HasClrf(key) || HasClrf(val)) {
		return;
	}
	headers.emplace(key, val);
}

inline void Response::SetRedirect(const std::string& url, int status)
{
	if (HasClrf(url)) {
		return;
	}
	SetHeader("Location", url);
	if (status >= 300 && status < 400) {
		this->status = status;
	} else {
		this->status = 302;
	}
}

inline void Response::SetContent(const char* str, size_t n, const std::string& contentType)
{
	body.assign(str, n);
	auto rng = headers.equal_range("Content-Type");
	headers.erase(rng.first, rng.second);
	SetHeader("Content-Type", contentType);
}

inline void Response::SetContent(const std::string& str, const std::string& contentType)
{
	SetContent(str.data(), str.size(), contentType);
}

inline void Response::SetContentProvider(size_t length, const std::string& contentType,
                                         ContentProvider provider,
                                         ContentProviderResourceReleaser resourceReleaser)
{
	if (length < 0) {
		return;
	}
	SetHeader("Content-Type", contentType);
	contentLength = length;
	contentProvider = provider;
	contentProviderResourceReleaser = resourceReleaser;
	isChunkedContentProvider = false;
}

inline void Response::SetContentProvider(const std::string& contentType,
                                         ContentProviderWithoutLength provider,
                                         ContentProviderResourceReleaser resourceReleaser)
{
	SetHeader("Content-Type", contentType);
	contentLength = 0;
	contentProvider = ContentProviderAdapter(std::move(provider));
	contentProviderResourceReleaser = resourceReleaser;
	isChunkedContentProvider = false;
}

inline void Response::SetChunckedContentProvider(const std::string& contentType,
                                                 ContentProviderWithoutLength provider,
                                                 ContentProviderResourceReleaser resourceReleaser)
{
	SetHeader("Content-Type", contentType);
	contentLength = 0;
	contentProvider = ContentProviderAdapter(std::move(provider));
	contentProviderResourceReleaser = resourceReleaser;
	isChunkedContentProvider = true;
}

inline Response::~Response()
{
	if (contentProviderResourceReleaser) {
		contentProviderResourceReleaser(contentProviderSuccess);
	}
}
