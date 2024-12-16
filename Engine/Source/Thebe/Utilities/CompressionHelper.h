#pragma once

#include "Thebe/Common.h"
#include <Windows.h>
#include <compressapi.h>

namespace Thebe
{
	/**
	 * 
	 */
	class THEBE_API CompressionHelper
	{
	public:
		CompressionHelper();
		virtual ~CompressionHelper();

		/**
		 * Compress the given input buffer into the given output buffer.
		 * 
		 * @param[in] inputBuffer This should contain the raw uncompressed data you want to compress.
		 * @param[out] outputBuffer This will be filled with the compressed data.
		 * @return True is returned on success; false, otherwise.
		 */
		virtual bool Compress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer) = 0;

		/**
		 * Decompress the given input buffer into the given output buffer.
		 * 
		 * @param[in] inputBuffer This should contain the compressed data you want to decompress.
		 * @param[in,out] This will be filled with the uncompressed data.
		 * @param True is returned on success; false, otherwise.
		 */
		virtual bool Decompress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer) = 0;
	};

	/**
	 * Provide compression/decompression using the win32 compression API.
	 * In practice, it doesn't work at all unless the data to be compressed is 2 MB or more.
	 */
	class THEBE_API Win32CompressionHelper : public CompressionHelper
	{
	public:
		Win32CompressionHelper();
		virtual ~Win32CompressionHelper();

		virtual bool Compress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer) override;
		virtual bool Decompress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer) override;

	private:
		DWORD compressionAlgorithm;
	};

	/**
	 * zLib to the rescue.
	 */
	class THEBE_API ZLibCompressionHelper : public CompressionHelper
	{
	public:
		ZLibCompressionHelper();
		virtual ~ZLibCompressionHelper();

		virtual bool Compress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer) override;
		virtual bool Decompress(const std::vector<uint8_t>& inputBuffer, std::vector<uint8_t>& outputBuffer) override;
	};
}