#pragma once

#include "common.h"

class Server {
public:
	using Handler = std::function<void(const Request&, Response&)>;

	using ExceptionHandler = std::function<void(const Request&, Response&, std::exception_ptr ep)>;

	enum class HandlerResponse {
		Handled,
		UnHandled
	};

	using HandlerWithResponse = std::function<HandlerResponse(const Request&, Response&)>;

	using HandlerWithContentReader = std::function<void(const Request&, Response&,
	                                                    const ContentReader& contentReader)>;

	using Expect100ContinueHandler = std::function<int(const Request&, Response&)>;

	Server();

	virtual ~Server();
};
