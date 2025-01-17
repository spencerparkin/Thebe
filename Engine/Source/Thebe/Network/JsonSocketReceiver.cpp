#include "Thebe/Network/JsonSocketReceiver.h"
#include "Thebe/Utilities/RingBuffer.h"

using namespace Thebe;

JsonSocketReceiver::JsonSocketReceiver(SOCKET socket)
{
	this->socket = socket;
}

/*virtual*/ JsonSocketReceiver::~JsonSocketReceiver()
{
}

/*virtual*/ void JsonSocketReceiver::ReceiveJson(std::unique_ptr<ParseParty::JsonValue>& jsonValue)
{
}

/*virtual*/ void JsonSocketReceiver::Run()
{
	using namespace ParseParty;

	RingBuffer ringBuffer(64 * 1024);
	std::vector<char> recvBuffer;
	recvBuffer.resize(4 * 1024);

	while (true)
	{
		uint32_t numBytesReceived = ::recv(this->socket, recvBuffer.data(), recvBuffer.size(), 0);
		if (numBytesReceived == SOCKET_ERROR)
			break;

		if (!ringBuffer.WriteBytes((const uint8_t*)recvBuffer.data(), numBytesReceived))
			break;

		uint32_t numStoredBytes = ringBuffer.GetNumStoredBytes();
		std::unique_ptr<uint8_t> buffer(new uint8_t[numStoredBytes]);
		if (!ringBuffer.PeakBytes(buffer.get(), numStoredBytes))
			break;

		bool nullTerminated = false;
		for (uint32_t i = 0; i < numStoredBytes && !nullTerminated; i++)
			if (buffer.get()[i] == '\0')
				nullTerminated = true;

		if (!nullTerminated)
			continue;

		std::string jsonText((const char*)buffer.get());
		std::string parseError;
		std::unique_ptr<JsonValue> jsonValue(JsonValue::ParseJson(jsonText, parseError));
		if (!jsonValue.get())
			break;

		if (!ringBuffer.DeleteBytes(jsonText.length() + 1))
			break;

		this->ReceiveJson(jsonValue);
	}
}