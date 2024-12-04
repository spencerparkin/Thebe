#pragma once

#include "Thebe/Common.h"
#include "Thebe/Math/Vector2.h"
#include "Thebe/Math/Vector3.h"
#include "Thebe/Math/Vector4.h"
#include "Thebe/Math/Matrix2x2.h"
#include "Thebe/Math/Matrix3x3.h"
#include "Thebe/Math/Matrix4x4.h"
#include "Thebe/Math/Transform.h"
#include "JsonValue.h"

namespace Thebe
{
	class THEBE_API JsonHelper
	{
	public:
		static bool VectorFromJsonValue(const ParseParty::JsonValue* jsonValue, Vector2& vector);
		static bool VectorFromJsonValue(const ParseParty::JsonValue* jsonValue, Vector3& vector);
		static bool VectorFromJsonValue(const ParseParty::JsonValue* jsonValue, Vector4& vector);

		static ParseParty::JsonValue* VectorToJsonValue(const Vector2& vector);
		static ParseParty::JsonValue* VectorToJsonValue(const Vector3& vector);
		static ParseParty::JsonValue* VectorToJsonValue(const Vector4& vector);

		static bool MatrixFromJsonValue(const ParseParty::JsonValue* jsonValue, Matrix2x2& matrix);
		static bool MatrixFromJsonValue(const ParseParty::JsonValue* jsonValue, Matrix3x3& matrix);
		static bool MatrixFromJsonValue(const ParseParty::JsonValue* jsonValue, Matrix4x4& matrix);

		static ParseParty::JsonValue* MatrixToJsonValue(const Matrix2x2& matrix);
		static ParseParty::JsonValue* MatrixToJsonValue(const Matrix3x3& matrix);
		static ParseParty::JsonValue* MatrixToJsonValue(const Matrix4x4& matrix);

		static bool TransformFromJsonValue(const ParseParty::JsonValue* jsonValue, Transform& transform);
		static ParseParty::JsonValue* TransformToJsonValue(const Transform& transform);
	};
}