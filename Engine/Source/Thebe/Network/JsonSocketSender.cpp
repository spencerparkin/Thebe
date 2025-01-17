#include "Thebe/Network/JsonSocketSender.h"

using namespace Thebe;

JsonSocketSender::JsonSocketSender(SOCKET socket) : jsonQueueSemaphore(0)
{
	this->socket = socket;
}

/*virtual*/ JsonSocketSender::~JsonSocketSender()
{
}

void JsonSocketSender::SendJson(const ParseParty::JsonValue* jsonValue)
{
	this->jsonQueue.Add(jsonValue);
	this->jsonQueueSemaphore.release();
}

/*virtual*/ void JsonSocketSender::Run()
{
	using namespace ParseParty;

	while (true)
	{
		this->jsonQueueSemaphore.acquire();

		const JsonValue* jsonValue = nullptr;
		if (!this->jsonQueue.Remove(jsonValue))
			break;

		std::string jsonText;
		if (!jsonValue->PrintJson(jsonText))
			break;

		uint32_t numBytesToSend = jsonText.length() + 1;
		uint32_t totalBytesSent = 0;
		while (totalBytesSent < numBytesToSend)
		{
			uint32_t numBytesRemaining = numBytesToSend - totalBytesSent;
			uint32_t numBytesSent = ::send(this->socket, &jsonText.c_str()[totalBytesSent], numBytesRemaining, 0);
			if (numBytesSent == SOCKET_ERROR)
				break;

			totalBytesSent += numBytesSent;
		}
	}
}