#pragma once

#include "Thebe/EngineParts/Buffer.h"

namespace Thebe
{
	class Shader;

	class THEBE_API ConstantsBuffer : public Buffer
	{
	public:
		ConstantsBuffer();
		virtual ~ConstantsBuffer();

		//bool SetParameter(const std::string& name, double scalar);
		//bool SetParameter(const std::string& name, const Vector3& vector);
		//bool SetParameter(const std::string& name, const Matrix3x3& matrix);
		//bool SetParameter(const std::string& name, const Matrix4x4& matrix);

	private:
		Reference<Shader> shader;
	};
}