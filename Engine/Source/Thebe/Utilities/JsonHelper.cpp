#include "Thebe/Utilities/JsonHelper.h"

using namespace Thebe;
using namespace ParseParty;

/*static*/ bool JsonHelper::VectorFromJsonValue(const JsonValue* jsonValue, Vector2& vector)
{
	auto vectorValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!vectorValue)
		return false;

	auto xValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("x"));
	auto yValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("y"));
	if (!(xValue && yValue))
		return false;

	vector.x = xValue->GetValue();
	vector.y = yValue->GetValue();
	return true;
}

/*static*/ bool JsonHelper::VectorFromJsonValue(const JsonValue* jsonValue, Vector3& vector)
{
	auto vectorValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!vectorValue)
		return false;

	auto xValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("x"));
	auto yValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("y"));
	auto zValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("z"));
	if (!(xValue && yValue && zValue))
		return false;

	vector.x = xValue->GetValue();
	vector.y = yValue->GetValue();
	vector.z = zValue->GetValue();
	return true;
}

/*static*/ bool JsonHelper::VectorFromJsonValue(const JsonValue* jsonValue, Vector4& vector)
{
	auto vectorValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!vectorValue)
		return false;

	auto xValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("x"));
	auto yValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("y"));
	auto zValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("z"));
	auto wValue = dynamic_cast<const JsonFloat*>(vectorValue->GetValue("w"));
	if (!(xValue && yValue && zValue && wValue))
		return false;

	vector.x = xValue->GetValue();
	vector.y = yValue->GetValue();
	vector.z = zValue->GetValue();
	vector.w = wValue->GetValue();
	return true;
}

/*static*/ JsonValue* JsonHelper::VectorToJsonValue(const Vector2& vector)
{
	auto vectorValue = new JsonObject();
	vectorValue->SetValue("x", new JsonFloat(vector.x));
	vectorValue->SetValue("y", new JsonFloat(vector.y));
	return vectorValue;
}

/*static*/ JsonValue* JsonHelper::VectorToJsonValue(const Vector3& vector)
{
	auto vectorValue = new JsonObject();
	vectorValue->SetValue("x", new JsonFloat(vector.x));
	vectorValue->SetValue("y", new JsonFloat(vector.y));
	vectorValue->SetValue("z", new JsonFloat(vector.z));
	return vectorValue;
}

/*static*/ JsonValue* JsonHelper::VectorToJsonValue(const Vector4& vector)
{
	auto vectorValue = new JsonObject();
	vectorValue->SetValue("x", new JsonFloat(vector.x));
	vectorValue->SetValue("y", new JsonFloat(vector.y));
	vectorValue->SetValue("z", new JsonFloat(vector.z));
	vectorValue->SetValue("w", new JsonFloat(vector.w));
	return vectorValue;
}

/*static*/ bool JsonHelper::MatrixFromJsonValue(const JsonValue* jsonValue, Matrix2x2& matrix)
{
	auto matrixValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!matrixValue)
		return false;

	auto elementsArrayValue = dynamic_cast<const JsonArray*>(matrixValue->GetValue("elements"));
	if (!elementsArrayValue || elementsArrayValue->GetSize() != 4)
		return false;

	for (unsigned int i = 0; i < elementsArrayValue->GetSize(); i++)
	{
		auto elementValue = dynamic_cast<const JsonFloat*>(elementsArrayValue->GetValue(i));
		if (!elementValue)
			return false;

		unsigned int r = i / 2;
		unsigned int c = i % 2;
		matrix.ele[r][c] = elementValue->GetValue();
	}

	return true;
}

/*static*/ bool JsonHelper::MatrixFromJsonValue(const JsonValue* jsonValue, Matrix3x3& matrix)
{
	auto matrixValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!matrixValue)
		return false;

	auto elementsArrayValue = dynamic_cast<const JsonArray*>(matrixValue->GetValue("elements"));
	if (!elementsArrayValue || elementsArrayValue->GetSize() != 9)
		return false;

	for (unsigned int i = 0; i < elementsArrayValue->GetSize(); i++)
	{
		auto elementValue = dynamic_cast<const JsonFloat*>(elementsArrayValue->GetValue(i));
		if (!elementValue)
			return false;

		unsigned int r = i / 3;
		unsigned int c = i % 3;
		matrix.ele[r][c] = elementValue->GetValue();
	}

	return true;
}

/*static*/ bool JsonHelper::MatrixFromJsonValue(const JsonValue* jsonValue, Matrix4x4& matrix)
{
	auto matrixValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!matrixValue)
		return false;

	auto elementsArrayValue = dynamic_cast<const JsonArray*>(matrixValue->GetValue("elements"));
	if (!elementsArrayValue || elementsArrayValue->GetSize() != 16)
		return false;

	for (unsigned int i = 0; i < elementsArrayValue->GetSize(); i++)
	{
		auto elementValue = dynamic_cast<const JsonFloat*>(elementsArrayValue->GetValue(i));
		if (!elementValue)
			return false;

		unsigned int r = i / 4;
		unsigned int c = i % 4;
		matrix.ele[r][c] = elementValue->GetValue();
	}

	return true;
}

/*static*/ JsonValue* JsonHelper::MatrixToJsonValue(const Matrix2x2& matrix)
{
	auto matrixValue = new JsonObject();

	auto elementsArrayValue = new JsonArray();
	matrixValue->SetValue("elements", elementsArrayValue);

	for (unsigned int i = 0; i < 2; i++)
		for (unsigned int j = 0; j < 2; j++)
			elementsArrayValue->PushValue(new JsonFloat(matrix.ele[i][j]));

	return matrixValue;
}

/*static*/ JsonValue* JsonHelper::MatrixToJsonValue(const Matrix3x3& matrix)
{
	auto matrixValue = new JsonObject();

	auto elementsArrayValue = new JsonArray();
	matrixValue->SetValue("elements", elementsArrayValue);

	for (unsigned int i = 0; i < 3; i++)
		for (unsigned int j = 0; j < 3; j++)
			elementsArrayValue->PushValue(new JsonFloat(matrix.ele[i][j]));

	return matrixValue;
}

/*static*/ JsonValue* JsonHelper::MatrixToJsonValue(const Matrix4x4& matrix)
{
	auto matrixValue = new JsonObject();

	auto elementsArrayValue = new JsonArray();
	matrixValue->SetValue("elements", elementsArrayValue);

	for (unsigned int i = 0; i < 4; i++)
		for (unsigned int j = 0; j < 4; j++)
			elementsArrayValue->PushValue(new JsonFloat(matrix.ele[i][j]));

	return matrixValue;
}

/*static*/ bool JsonHelper::TransformFromJsonValue(const JsonValue* jsonValue, Transform& transform)
{
	auto transformValue = dynamic_cast<const JsonObject*>(jsonValue);
	if (!transformValue)
		return false;

	if (!MatrixFromJsonValue(transformValue->GetValue("matrix"), transform.matrix))
		return false;

	if (!VectorFromJsonValue(transformValue->GetValue("translation"), transform.translation))
		return false;

	return true;
}

/*static*/ JsonValue* JsonHelper::TransformToJsonValue(const Transform& transform)
{
	auto transformValue = new JsonObject();
	transformValue->SetValue("matrix", MatrixToJsonValue(transform.matrix));
	transformValue->SetValue("translation", VectorToJsonValue(transform.translation));
	return transformValue;
}