#pragma once

#include "Thebe/EngineParts/Buffer.h"
#include "Thebe/EngineParts/DescriptorHeap.h"
#include "Thebe/Math/Vector2.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Matrix2x2.h"
#include "Thebe/Math/Matrix3x3.h"
#include "Thebe/Math/Matrix4x4.h"

namespace Thebe
{
	class Shader;

	class THEBE_API ConstantsBuffer : public Buffer
	{
	public:
		ConstantsBuffer();
		virtual ~ConstantsBuffer();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		bool SetParameter(const std::string& name, double scalar);
		bool SetParameter(const std::string& name, const Vector2& vector);
		bool SetParameter(const std::string& name, const Vector3& vector);
		bool SetParameter(const std::string& name, const Matrix2x2& matrix);
		bool SetParameter(const std::string& name, const Matrix3x3& matrix);
		bool SetParameter(const std::string& name, const Matrix4x4& matrix);

		void SetShader(Shader* shader);

	private:
		Reference<Shader> shader;
		DescriptorHeap::Descriptor cbvDescriptor;
	};
}