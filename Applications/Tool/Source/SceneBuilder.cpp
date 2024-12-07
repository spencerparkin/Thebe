#include "SceneBuilder.h"
#include "Thebe/Log.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/Mesh.h"
#include <assimp/postprocess.h>

SceneBuilder::SceneBuilder()
{
}

/*virtual*/ SceneBuilder::~SceneBuilder()
{
}

void SceneBuilder::SetAssetsFolder(const std::filesystem::path& outputAssetsFolder)
{
	this->outputAssetsFolder = outputAssetsFolder;
}

bool SceneBuilder::BuildScene(const std::filesystem::path& inputSceneFile)
{
	THEBE_LOG("Building scene!");
	THEBE_LOG("Input file: %s", inputSceneFile.string().c_str());
	THEBE_LOG("Output assets folder: %s", this->outputAssetsFolder.string().c_str());

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->RemoveAllAssetFolders();
	graphicsEngine->AddAssetFolder(this->outputAssetsFolder);

	this->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0);

	const aiScene* inputScene = this->importer.ReadFile(inputSceneFile.string().c_str(), aiProcess_GlobalScale);
	if (!inputScene)
	{
		THEBE_LOG("Failed to read scene file!  Error: %s", this->importer.GetErrorString());
		return false;
	}

	Thebe::Reference<Thebe::Space> outputRootNode = this->GenerateSceneTree(inputScene->mRootNode);
	if (!outputRootNode.Get())
	{
		THEBE_LOG("Failed to generate root space.");
		return false;
	}

	Thebe::Reference<Thebe::Scene> outputScene(new Thebe::Scene());
	outputScene->SetGraphicsEngine(graphicsEngine);
	outputScene->SetName(inputScene->mName.C_Str());
	outputScene->SetRootSpace(outputRootNode);

	std::filesystem::path outputSceneFile = this->outputAssetsFolder / "Scenes" / inputSceneFile.filename();
	outputSceneFile.replace_extension(".scene");
	if (!graphicsEngine->DumpEnginePartToFile(outputSceneFile, outputScene, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to write scene file!");
		return false;
	}

	for (int i = 0; i < (int)inputScene->mNumMeshes; i++)
	{
		const aiMesh* inputMesh = inputScene->mMeshes[i];
		Thebe::Reference<Thebe::Mesh> outputMesh = this->GenerateMesh(inputMesh);
		if (!outputMesh.Get())
		{
			THEBE_LOG("Failed to generate mesh %d.", i);
			return false;
		}

		std::filesystem::path outputMeshFile = this->outputAssetsFolder / this->GenerateMeshPath(inputMesh);
		if (!graphicsEngine->DumpEnginePartToFile(outputMeshFile, outputMesh, THEBE_DUMP_FLAG_CAN_OVERWRITE))
		{
			THEBE_LOG("Failed to dump mesh %d.", i);
			return false;
		}
	}

	for (int i = 0; i < (int)inputScene->mNumMaterials; i++)
	{
		//...
	}

	return true;
}

Thebe::Reference<Thebe::Mesh> SceneBuilder::GenerateMesh(const aiMesh* inputMesh)
{
	Thebe::Reference<Thebe::Mesh> outputMesh(new Thebe::Mesh());
	outputMesh->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());
	outputMesh->SetName(inputMesh->mName.C_Str());

	const aiMaterial* inputMaterial = this->importer.GetScene()->mMaterials[inputMesh->mMaterialIndex];
	outputMesh->SetMaterialPath(this->GenerateMaterialPath(inputMaterial));

	Thebe::Reference<Thebe::IndexBuffer> outputIndexBuffer = this->GenerateIndexBuffer(inputMesh);
	if (!outputIndexBuffer.Get())
	{
		THEBE_LOG("Failed to generate index buffer for mesh %s.", inputMesh->mName.C_Str());
		return Thebe::Reference<Thebe::Mesh>();
	}

	std::filesystem::path outputIndexBufferPath = this->GenerateIndexBufferPath(inputMesh);
	outputMesh->SetIndexBufferPath(outputIndexBufferPath);

	Thebe::Reference<Thebe::VertexBuffer> outputVertexBuffer = this->GenerateVertexBuffer(inputMesh);
	if (!outputVertexBuffer.Get())
	{
		THEBE_LOG("Failed to generate vertex buffer for mesh %s.", inputMesh->mName.C_Str());
		return Thebe::Reference<Thebe::Mesh>();
	}

	std::filesystem::path outputVertexBufferPath = this->GenerateVertexBufferPath(inputMesh);
	outputMesh->SetVertexBufferPath(outputVertexBufferPath);

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();

	outputIndexBufferPath = this->outputAssetsFolder / outputIndexBufferPath;
	if (!graphicsEngine->DumpEnginePartToFile(outputIndexBufferPath, outputIndexBuffer, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to dump index buffer for mesh %s.", inputMesh->mName.C_Str());
		return Thebe::Reference<Thebe::Mesh>();
	}

	outputVertexBufferPath = this->outputAssetsFolder / outputVertexBufferPath;
	if (!graphicsEngine->DumpEnginePartToFile(outputVertexBufferPath, outputVertexBuffer, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to dump vertex buffer for mesh %s.", inputMesh->mName.C_Str());
		return Thebe::Reference<Thebe::Mesh>();
	}

	return outputMesh;
}

Thebe::Reference<Thebe::VertexBuffer> SceneBuilder::GenerateVertexBuffer(const aiMesh* inputMesh)
{
	Thebe::Reference<Thebe::VertexBuffer> outputVertexBuffer(new Thebe::VertexBuffer());
	outputVertexBuffer->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());

	D3D12_RESOURCE_DESC& gpuBufferDesc = outputVertexBuffer->GetResourceDesc();
	gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	std::vector<D3D12_INPUT_ELEMENT_DESC>& outputElementDescArray = outputVertexBuffer->GetElementDescArray();
	
	uint32_t strideInBytes = 3 * sizeof(float);

	D3D12_INPUT_ELEMENT_DESC vertexElementDesc{};
	vertexElementDesc.SemanticName = "POSITION";
	vertexElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	vertexElementDesc.AlignedByteOffset = 0;
	outputElementDescArray.push_back(vertexElementDesc);

	if (inputMesh->HasNormals())
	{
		strideInBytes += 3 * sizeof(float);

		D3D12_INPUT_ELEMENT_DESC normalElementDesc{};
		normalElementDesc.SemanticName = "NORMAL";
		normalElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		normalElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		normalElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		outputElementDescArray.push_back(normalElementDesc);
	}

	if (inputMesh->HasTextureCoords(0))
	{
		strideInBytes += 2 * sizeof(float);

		D3D12_INPUT_ELEMENT_DESC texCoordElementDesc{};
		texCoordElementDesc.SemanticName = "TEXCOORD";
		texCoordElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		texCoordElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		texCoordElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		outputElementDescArray.push_back(texCoordElementDesc);
	}

	if (inputMesh->HasVertexColors(0))
	{
		strideInBytes += 4 * sizeof(float);

		D3D12_INPUT_ELEMENT_DESC colorElementDesc{};
		colorElementDesc.SemanticName = "COLOR";
		colorElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		colorElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		colorElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		outputElementDescArray.push_back(colorElementDesc);
	}

	strideInBytes = THEBE_ALIGNED(strideInBytes, 16);	// TODO: Is this right?

	outputVertexBuffer->SetStride(strideInBytes);
	outputVertexBuffer->SetBufferType(Thebe::Buffer::STATIC);

	std::vector<UINT8>& outputBuffer = outputVertexBuffer->GetOriginalBuffer();
	outputBuffer.resize(inputMesh->mNumVertices * strideInBytes);
	::memset(outputBuffer.data(), 0, outputBuffer.size());

	for (int i = 0; i < (int)inputMesh->mNumVertices; i++)
	{
		UINT8* outputVertex = &outputBuffer.data()[i * strideInBytes];
		auto outputComponent = reinterpret_cast<float*>(outputVertex);

		const aiVector3D& inputVertexPosition = inputMesh->mVertices[i];
		*outputComponent++ = (float)inputVertexPosition.x;
		*outputComponent++ = (float)inputVertexPosition.y;
		*outputComponent++ = (float)inputVertexPosition.z;

		if (inputMesh->HasNormals())
		{
			const aiVector3D& inputNormal = inputMesh->mNormals[i];
			*outputComponent++ = (float)inputNormal.x;
			*outputComponent++ = (float)inputNormal.y;
			*outputComponent++ = (float)inputNormal.z;
		}

		if (inputMesh->HasTextureCoords(0))
		{
			const aiVector3D& inputTexCoords = inputMesh->mTextureCoords[0][i];
			*outputComponent++ = (float)inputTexCoords.x;
			*outputComponent++ = (float)inputTexCoords.y;
		}

		if (inputMesh->HasVertexColors(0))
		{
			const aiColor4D& inputColor = inputMesh->mColors[0][i];
			*outputComponent++ = (float)inputColor.r;
			*outputComponent++ = (float)inputColor.g;
			*outputComponent++ = (float)inputColor.b;
			*outputComponent++ = (float)inputColor.a;
		}
	}

	return outputVertexBuffer;
}

Thebe::Reference<Thebe::IndexBuffer> SceneBuilder::GenerateIndexBuffer(const aiMesh* inputMesh)
{
	Thebe::Reference<Thebe::IndexBuffer> outputIndexBuffer(new Thebe::IndexBuffer());
	outputIndexBuffer->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());

	D3D12_RESOURCE_DESC& gpuBufferDesc = outputIndexBuffer->GetResourceDesc();
	gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	std::string inputMeshName(inputMesh->mName.C_Str());
	std::string outputIndexBufferName = this->NoSpaces(inputMeshName) + "_IndexBuffer";
	outputIndexBuffer->SetName(outputIndexBufferName);

	outputIndexBuffer->SetFormat(DXGI_FORMAT_R32_UINT);
	outputIndexBuffer->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::vector<UINT8>& outputBuffer = outputIndexBuffer->GetOriginalBuffer();
	outputBuffer.resize(inputMesh->mNumFaces * 3 * sizeof(uint32_t));

	auto outputIndex = reinterpret_cast<uint32_t*>(outputBuffer.data());
	for (int i = 0; i < (int)inputMesh->mNumFaces; i++)
	{
		const aiFace* inputFace = &inputMesh->mFaces[i];
		if (inputFace->mNumIndices != 3)
		{
			THEBE_LOG("Expected each face to have exactly three vertices (be a triangle.)");
			return Thebe::Reference<Thebe::IndexBuffer>();
		}

		for (int j = 0; j < (int)inputFace->mNumIndices; j++)
		{
			uint32_t inputIndex = (uint32_t)inputFace->mIndices[j];
			*outputIndex++ = inputIndex;
		}
	}

	return outputIndexBuffer;
}

Thebe::Reference<Thebe::Space> SceneBuilder::GenerateSceneTree(const aiNode* inputParentNode)
{
	Thebe::Reference<Thebe::Space> outputParentNode(new Thebe::Space());
	outputParentNode->SetName(inputParentNode->mName.C_Str());
	outputParentNode->SetChildToParentTransform(this->MakeTransform(inputParentNode->mTransformation));

	for (int i = 0; i < (int)inputParentNode->mNumChildren; i++)
	{
		const aiNode* inputChildNode = inputParentNode->mChildren[i];
		Thebe::Reference<Thebe::Space> outputChildNode = this->GenerateSceneTree(inputChildNode);
		outputParentNode->AddSubSpace(outputChildNode);
	}

	for (int i = 0; i < (int)inputParentNode->mNumMeshes; i++)
	{
		const aiMesh* inputMesh = this->importer.GetScene()->mMeshes[inputParentNode->mMeshes[i]];
		Thebe::Reference<Thebe::MeshInstance> meshInstance = this->GenerateMeshInstance(inputMesh);
		outputParentNode->AddSubSpace(meshInstance);
	}

	return outputParentNode;
}

Thebe::Reference<Thebe::MeshInstance> SceneBuilder::GenerateMeshInstance(const aiMesh* inputMesh)
{
	Thebe::Reference<Thebe::MeshInstance> outputMeshInstance(new Thebe::MeshInstance());
	std::filesystem::path outputMeshPath = this->GenerateMeshPath(inputMesh);
	outputMeshInstance->SetMeshPath(outputMeshPath);
	return outputMeshInstance;
}

std::filesystem::path SceneBuilder::GenerateMeshPath(const aiMesh* inputMesh)
{
	std::string inputMeshName(inputMesh->mName.C_Str());
	std::string outputMeshName = inputMeshName + ".mesh";
	outputMeshName = this->NoSpaces(this->PrefixWithSceneName(outputMeshName));
	std::filesystem::path outputMeshPath = std::filesystem::path("Meshes") / outputMeshName;
	return outputMeshPath;
}

std::filesystem::path SceneBuilder::GenerateMaterialPath(const aiMaterial* inputMaterial)
{
	std::string inputMaterialName(inputMaterial->GetName().C_Str());
	std::string outputMaterialName = inputMaterialName + ".material";
	outputMaterialName = this->NoSpaces(this->PrefixWithSceneName(outputMaterialName));
	std::filesystem::path outputMaterialPath = std::filesystem::path("Materials") / outputMaterialName;
	return outputMaterialPath;
}

std::filesystem::path SceneBuilder::GenerateIndexBufferPath(const aiMesh* inputMesh)
{
	std::string inputMeshName(inputMesh->mName.C_Str());
	std::string outputIndexBufferName = inputMeshName + ".index_buffer";
	outputIndexBufferName = this->NoSpaces(this->PrefixWithSceneName(outputIndexBufferName));
	std::filesystem::path outputIndexBufferPath = std::filesystem::path("Buffers") / outputIndexBufferName;
	return outputIndexBufferPath;
}

std::filesystem::path SceneBuilder::GenerateVertexBufferPath(const aiMesh* inputMesh)
{
	std::string inputMeshName(inputMesh->mName.C_Str());
	std::string outputVertexBufferName = inputMeshName + ".vertex_buffer";
	outputVertexBufferName = this->NoSpaces(this->PrefixWithSceneName(outputVertexBufferName));
	std::filesystem::path outputVertexBufferPath = std::filesystem::path("Buffers") / outputVertexBufferName;
	return outputVertexBufferPath;
}

std::string SceneBuilder::NoSpaces(const std::string& givenString)
{
	std::string resultString = givenString;

	while (true)
	{
		size_t pos = resultString.find(' ');
		if (pos == std::string::npos)
			break;

		resultString.replace(pos, 1, "_");
	}

	return resultString;
}

std::string SceneBuilder::PrefixWithSceneName(const std::string& givenString)
{
	std::string resultString = givenString;
	std::string inputSceneName(this->importer.GetScene()->mName.C_Str());
	if (inputSceneName.length() > 0)
		resultString = inputSceneName + "_" + resultString;
	return resultString;
}

Thebe::Transform SceneBuilder::MakeTransform(const aiMatrix4x4& givenMatrix)
{
	THEBE_ASSERT(givenMatrix.d1 == 0.0 && givenMatrix.d2 == 0.0 && givenMatrix.d3 == 0.0 && givenMatrix.d4 == 1.0);
	
	Thebe::Transform xform;

	xform.matrix.ele[0][0] = givenMatrix.a1;
	xform.matrix.ele[0][1] = givenMatrix.a2;
	xform.matrix.ele[0][2] = givenMatrix.a3;

	xform.matrix.ele[1][0] = givenMatrix.b1;
	xform.matrix.ele[1][1] = givenMatrix.b2;
	xform.matrix.ele[1][2] = givenMatrix.b3;

	xform.matrix.ele[2][0] = givenMatrix.c1;
	xform.matrix.ele[2][1] = givenMatrix.c2;
	xform.matrix.ele[2][2] = givenMatrix.c3;

	xform.translation.x = givenMatrix.a4;
	xform.translation.y = givenMatrix.b4;
	xform.translation.z = givenMatrix.c4;

	return xform;
}

Thebe::Vector3 SceneBuilder::MakeVector(const aiVector3D& givenVector)
{
	Thebe::Vector3 vec;
	vec.SetComponents(givenVector.x, givenVector.y, givenVector.z);
	return vec;
}

Thebe::Vector2 SceneBuilder::MakeTexCoords(const aiVector3D& givenTexCoords)
{
	Thebe::Vector2 vec;
	vec.SetComponents(givenTexCoords.x, givenTexCoords.y);
	return vec;
}

Thebe::Quaternion SceneBuilder::MakeQuat(const aiQuaternion& givenQuaternion)
{
	Thebe::Quaternion quat;
	quat.w = givenQuaternion.w;
	quat.x = givenQuaternion.x;
	quat.y = givenQuaternion.y;
	quat.z = givenQuaternion.z;
	return quat;
}