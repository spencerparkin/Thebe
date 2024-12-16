#include "Thebe/Utilities/CompressionHelper.h"
#include "Thebe/Log.h"
#include "zlib.h"

using namespace Thebe;

//------------------------------------ CompressionHelper ------------------------------------

CompressionHelper::CompressionHelper()
{
}

/*virtual*/ CompressionHelper::~CompressionHelper()
{
}

//------------------------------------ Win32CompressionHelper ------------------------------------

Win32CompressionHelper::Win32CompressionHelper()
{
	this->compressionAlgorithm = COMPRESS_ALGORITHM_XPRESS_HUFF;
}

/*virtual*/ Win32CompressionHelper::~Win32CompressionHelper()
{
}

/*virtual*/ bool Win32CompressionHelper::Compress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer)
{
	COMPRESSOR_HANDLE compressorHandle = NULL;
	bool success = false;

	do
	{
		if (!::CreateCompressor(this->compressionAlgorithm, NULL, &compressorHandle))
		{
			THEBE_LOG("Failed to create compressor.  Error: %d", GetLastError());
			break;
		}

		// How much memory do we need for the compressed buffer?
		SIZE_T compressedBufferSize = 0;
		if (!::Compress(compressorHandle, inputBuffer.data(), inputBuffer.size(), nullptr, 0, &compressedBufferSize))
		{
			DWORD error = GetLastError();
			if (error != ERROR_INSUFFICIENT_BUFFER)
			{
				THEBE_LOG("Failed to query for compressed buffer size.  Error: %d", error);
				break;
			}
		}

		if (compressedBufferSize >= inputBuffer.size())
		{
			THEBE_LOG("Compression is inflating, not deflating.");
			break;
		}

		outputBuffer.resize(compressedBufferSize);

		if (!::Compress(compressorHandle, inputBuffer.data(), inputBuffer.size(), outputBuffer.data(), outputBuffer.size(), &compressedBufferSize))
		{
			THEBE_LOG("Failed to compress buffer.  Error: %d", GetLastError());
			break;
		}

		success = true;
	} while (false);

	if (compressorHandle)
		CloseCompressor(compressorHandle);

	return success;
}

/*virtual*/ bool Win32CompressionHelper::Decompress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer)
{
	COMPRESSOR_HANDLE compressorHandle = NULL;
	bool success = false;

	do
	{
		if (!::CreateCompressor(this->compressionAlgorithm, NULL, &compressorHandle))
		{
			THEBE_LOG("Failed to create decompressor.  Error: %d", GetLastError());
			break;
		}

		// How much memory do we need for the decompressed buffer?
		SIZE_T uncompressedBufferSize = 0;
		if (!::Decompress(compressorHandle, inputBuffer.data(), inputBuffer.size(), nullptr, 0, &uncompressedBufferSize))
		{
			DWORD error = GetLastError();
			if (error != ERROR_INSUFFICIENT_BUFFER)
			{
				THEBE_LOG("Failed to query for decompressed buffer size.  Error: %d", error);
				break;
			}
		}

		outputBuffer.resize(uncompressedBufferSize);

		if (!::Decompress(compressorHandle, inputBuffer.data(), inputBuffer.size(), outputBuffer.data(), outputBuffer.size(), &uncompressedBufferSize))
		{
			THEBE_LOG("Failed to decompress buffer.  Error: %d", GetLastError());
			break;
		}

		success = true;
	} while (false);

	if (compressorHandle)
		CloseCompressor(compressorHandle);

	return success;
}

//------------------------------------ ZLibCompressionHelper ------------------------------------

ZLibCompressionHelper::ZLibCompressionHelper()
{
}

/*virtual*/ ZLibCompressionHelper::~ZLibCompressionHelper()
{
}

/*virtual*/ bool ZLibCompressionHelper::Compress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer)
{
	z_stream zStream{};
	int result = deflateInit(&zStream, 9 /* highest compression level */);
	if (result != Z_OK)
	{
		THEBE_LOG("Failed to initialize deflate algorithm.");
		return false;
	}

	outputBuffer.resize(inputBuffer.size());

	zStream.next_in = (Bytef*)inputBuffer.data();
	zStream.avail_in = (uInt)inputBuffer.size();
	zStream.next_out = (Bytef*)outputBuffer.data();
	zStream.avail_out = (uInt)outputBuffer.size();

	do
	{
		result = deflate(&zStream, 0);
		if (result != Z_OK)
		{
			THEBE_LOG("Deflate failed.  Error: %d", result);
			return false;
		}
	} while (zStream.avail_in > 0);

	result = deflate(&zStream, Z_FINISH);
	if (result != Z_STREAM_END)
	{
		THEBE_LOG("Failed to flush z-stream.  Error: %d", result);
		return false;
	}

	// Truncate the size of our buffer.  This shouldn't damage the data.
	outputBuffer.resize(zStream.total_out);

	result = deflateEnd(&zStream);
	if (result != Z_OK)
	{
		THEBE_LOG("Failed to deinitialize deflate algorithm.  Error: %d", result);
		return false;
	}

	return true;
}

/*virtual*/ bool ZLibCompressionHelper::Decompress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer)
{
	if (outputBuffer.size() == 0)
	{
		THEBE_LOG("Output buffer should be set to the known decompressed size before beginning decompression.");
		return false;
	}

	z_stream zStream{};
	int result = inflateInit(&zStream);
	if (result != Z_OK)
	{
		THEBE_LOG("Failed to initialize inflation algorithm.");
		return false;
	}

	zStream.next_in = (Bytef*)inputBuffer.data();
	zStream.avail_in = (uInt)inputBuffer.size();
	zStream.next_out = (Bytef*)outputBuffer.data();
	zStream.avail_out = (uInt)outputBuffer.size();

	do
	{
		result = inflate(&zStream, 0);
		if (result != Z_OK && result != Z_STREAM_END)
		{
			THEBE_LOG("Inflate failed.  Error: %d", result);
			return false;
		}
	} while (zStream.avail_in > 0);

	result = inflate(&zStream, Z_FINISH);
	if (result != Z_STREAM_END)
	{
		THEBE_LOG("Failed to flush z-stream.  Error: %d", result);
		return false;
	}

	result = inflateEnd(&zStream);
	if (result != Z_OK)
	{
		THEBE_LOG("Failed to deinitialize inflate algorithm.  Error: %d", result);
		return false;
	}

	return true;
}