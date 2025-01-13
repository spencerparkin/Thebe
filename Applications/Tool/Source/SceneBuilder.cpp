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

	this->inputSceneFileFolder = inputSceneFile.parent_path();
	this->textureBuilder.ClearTextures();

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->RemoveAllAssetFolders();
	graphicsEngine->AddAssetFolder(this->outputAssetsFolder);
	graphicsEngine->AddAssetFolder("Engine/Assets");

	this->importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0);

	THEBE_LOG("Loading scene file: %s", inputSceneFile.string().c_str());
	const aiScene* inputScene = this->importer.ReadFile(inputSceneFile.string().c_str(), aiProcess_GlobalScale);
	if (!inputScene)
	{
		THEBE_LOG("Failed to read scene file!  Error: %s", this->importer.GetErrorString());
		return false;
	}

	THEBE_LOG("Generating scene tree...");
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

	THEBE_LOG("Dumping scene tree...");
	std::filesystem::path outputSceneFile = this->outputAssetsFolder / "Scenes" / inputSceneFile.filename();
	outputSceneFile.replace_extension(".scene");
	if (!graphicsEngine->DumpEnginePartToFile(outputSceneFile, outputScene, THEBE_DUMP_FLAG_CAN_OVERWRITE))
	{
		THEBE_LOG("Failed to write scene file!");
		return false;
	}

	THEBE_LOG("Processing %d meshes...", inputScene->mNumMeshes);
	for (int i = 0; i < (int)inputScene->mNumMeshes; i++)
	{
		const aiMesh* inputMesh = inputScene->mMeshes[i];
		THEBE_LOG("Processing mesh: %s", inputMesh->mName.C_Str());

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

	THEBE_LOG("Processing %d materials...", inputScene->mNumMaterials);
	for (int i = 0; i < (int)inputScene->mNumMaterials; i++)
	{
		const aiMaterial* inputMaterial = inputScene->mMaterials[i];
		THEBE_LOG("Processing material: %s", inputMaterial->GetName().C_Str());

		Thebe::Reference<Thebe::Material> outputMaterial = this->GenerateMaterial(inputMaterial);
		if (!outputMaterial.Get())
		{
			THEBE_LOG("Failed to generate material %d.", i);
			return false;
		}

		std::filesystem::path outputMaterialFile = this->outputAssetsFolder / this->GenerateMaterialPath(inputMaterial);
		if (!graphicsEngine->DumpEnginePartToFile(outputMaterialFile, outputMaterial, THEBE_DUMP_FLAG_CAN_OVERWRITE))
		{
			THEBE_LOG("Failed to dump material %d.", i);
			return false;
		}
	}

	if (!this->textureBuilder.GenerateTextures())
		return false;

	return true;
}

Thebe::Reference<Thebe::Material> SceneBuilder::GenerateMaterial(const aiMaterial* inputMaterial)
{
	Thebe::Reference<Thebe::Material> outputMaterial(new Thebe::Material());
	outputMaterial->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());
	outputMaterial->SetName(inputMaterial->GetName().C_Str());

	aiString inputDiffuseTexture;
	if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), &inputDiffuseTexture))
	{
		outputMaterial->SetShaderPath("Shaders/BasicShader.shader");

		std::filesystem::path inputDiffuseTexturePath = (this->inputSceneFileFolder / inputDiffuseTexture.C_Str()).lexically_normal();
		this->textureBuilder.AddTexture(inputDiffuseTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8G8B8A8_UNORM, UINT(-1) });
		std::filesystem::path outputDiffuseTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputDiffuseTexturePath);
		outputMaterial->SetTexturePath("diffuse", outputDiffuseTexturePath);
	}
	else
	{
		aiString inputAlbedoTexture;
		if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), &inputAlbedoTexture))
		{
			outputMaterial->SetShaderPath("Shaders/PBRShader.shader");

			std::filesystem::path inputAlbedoTexturePath = (this->inputSceneFileFolder / inputAlbedoTexture.C_Str()).lexically_normal();
			this->textureBuilder.AddTexture(inputAlbedoTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8G8B8A8_UNORM, UINT(-1) });
			std::filesystem::path outputAlbedoTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputAlbedoTexturePath);
			outputMaterial->SetTexturePath("albedo", outputAlbedoTexturePath);

			aiString inputAmbientOcclusionTexture;
			if (AI_SUCCESS != aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), &inputAmbientOcclusionTexture))
			{
				// 3Ds Max doesn't seem to have a slot in the material for AO, so just look to see if we can find an AO map.
				std::string name = inputAlbedoTexturePath.stem().string();
				std::string match = "_albedo";
				size_t i = name.find(match);
				if (i != std::string::npos)
				{
					name.replace(i, match.length(), "_ao");
					std::filesystem::path aoPath = inputAlbedoTexturePath.parent_path() / name;
					aoPath.replace_extension(inputAlbedoTexturePath.extension());
					if (std::filesystem::exists(aoPath))
						inputAmbientOcclusionTexture = aoPath.string().c_str();
				}
			}

			if(inputAmbientOcclusionTexture.length > 0)
			{
				std::filesystem::path inputAmbientOcclusionTexturePath = (this->inputSceneFileFolder / inputAmbientOcclusionTexture.C_Str()).lexically_normal();
				this->textureBuilder.AddTexture(inputAmbientOcclusionTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8_UNORM, UINT(-1) });
				std::filesystem::path outputAmbientOcclusionTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputAmbientOcclusionTexturePath);
				outputMaterial->SetTexturePath("ambient_occlusion", outputAmbientOcclusionTexturePath);
			}

			aiString inputHeightTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), &inputHeightTexture))
			{
				std::filesystem::path inputHeightTexturePath = (this->inputSceneFileFolder / inputHeightTexture.C_Str()).lexically_normal();
				this->textureBuilder.AddTexture(inputHeightTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8G8B8A8_UNORM, UINT(-1) });
				std::filesystem::path outputHeightTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputHeightTexturePath);
				outputMaterial->SetTexturePath("height", outputHeightTexturePath);
			}

			aiString inputMetalicTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), &inputMetalicTexture))
			{
				std::filesystem::path inputMetalicTexturePath = (this->inputSceneFileFolder / inputMetalicTexture.C_Str()).lexically_normal();
				this->textureBuilder.AddTexture(inputMetalicTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8_UNORM, UINT(-1) });
				std::filesystem::path outputMetalicTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputMetalicTexturePath);
				outputMaterial->SetTexturePath("metalic", outputMetalicTexturePath);
			}

			aiString inputNormalTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_NORMAL_CAMERA, 0), &inputNormalTexture))
			{
				std::filesystem::path inputNormalTexturePath = (this->inputSceneFileFolder / inputNormalTexture.C_Str()).lexically_normal();
				this->textureBuilder.AddTexture(inputNormalTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8G8B8A8_UNORM, UINT(-1) });
				std::filesystem::path outputNormalTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputNormalTexturePath);
				outputMaterial->SetTexturePath("normal", outputNormalTexturePath);
			}

			aiString inputRoughnessTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), &inputRoughnessTexture))
			{
				std::filesystem::path inputRoughnessTexturePath = (this->inputSceneFileFolder / inputRoughnessTexture.C_Str()).lexically_normal();
				this->textureBuilder.AddTexture(inputRoughnessTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_R8_UNORM, UINT(-1) });
				std::filesystem::path outputRoughnessTexturePath = this->textureBuilder.GenerateTextureBufferPath(inputRoughnessTexturePath);
				outputMaterial->SetTexturePath("roughness", outputRoughnessTexturePath);
			}
		}
		else
		{
			outputMaterial->SetShaderPath("Shaders/FlatShader.shader");
		}
	}

	return outputMaterial;
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
	outputVertexBuffer->SetBufferType(Thebe::Buffer::STATIC);

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

	if (inputMesh->HasTangentsAndBitangents())
	{
		strideInBytes += 3 * sizeof(float);

		D3D12_INPUT_ELEMENT_DESC tangentElementDesc{};
		tangentElementDesc.SemanticName = "TEXCOORD";
		tangentElementDesc.SemanticIndex = 1;
		tangentElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		tangentElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		tangentElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		outputElementDescArray.push_back(tangentElementDesc);
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

		if (inputMesh->HasTangentsAndBitangents())
		{
			const aiVector3D& inputTangents = inputMesh->mTangents[i];
			*outputComponent++ = (float)inputTangents.x;
			*outputComponent++ = (float)inputTangents.y;
			*outputComponent++ = (float)inputTangents.z;
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
	outputIndexBuffer->SetBufferType(Thebe::Buffer::STATIC);

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
		Thebe::Reference<Thebe::MeshInstance> meshInstance = this->GenerateMeshInstance(inputMesh, inputParentNode);
		outputParentNode->AddSubSpace(meshInstance);
	}

	return outputParentNode;
}

Thebe::Reference<Thebe::MeshInstance> SceneBuilder::GenerateMeshInstance(const aiMesh* inputMesh, const aiNode* inputNode)
{
	Thebe::Reference<Thebe::MeshInstance> outputMeshInstance(new Thebe::MeshInstance());

	std::filesystem::path outputMeshPath = this->GenerateMeshPath(inputMesh);
	outputMeshInstance->SetMeshPath(outputMeshPath);

	aiString userProperties;
	if (inputNode->mMetaData->Get("UserProperties", userProperties))
	{
		std::filesystem::path overrideMaterialPath(userProperties.C_Str());
		outputMeshInstance->SetOverrideMaterialPath(overrideMaterialPath);
	}

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

std::string SceneBuilder::PrefixWithSceneName(const std::string& givenString)
{
	std::string resultString = givenString;
	std::string inputSceneName(this->importer.GetScene()->mName.C_Str());
	if (inputSceneName.length() > 0)
		resultString = inputSceneName + "_" + resultString;
	return resultString;
}