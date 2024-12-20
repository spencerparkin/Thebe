#include "SceneBuilder.h"
#include "Thebe/Log.h"
#include "Thebe/EngineParts/Space.h"
#include "Thebe/EngineParts/Mesh.h"
#include <assimp/postprocess.h>
#include <wx/image.h>

SceneBuilder::SceneBuilder()
{
	this->compressTextures = true;
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
	this->texturesToBuildSet.clear();

	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	graphicsEngine->RemoveAllAssetFolders();
	graphicsEngine->AddAssetFolder(this->outputAssetsFolder);

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
		THEBE_LOG("Processing mesh %s", inputMesh->mName.C_Str());

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
		THEBE_LOG("Processing material %s", inputMaterial->GetName().C_Str());

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

	THEBE_LOG("Processing %d textures...", this->texturesToBuildSet.size());
	for (const std::filesystem::path& inputTexturePath : this->texturesToBuildSet)
	{
		THEBE_LOG("Processing texture %s", inputTexturePath.string().c_str());

		Thebe::Reference<Thebe::TextureBuffer> outputTexture = this->GenerateTextureBuffer(inputTexturePath);
		if (!outputTexture.Get())
		{
			THEBE_LOG("Failed to generate texture: %s", inputTexturePath.string().c_str());
			return Thebe::Reference<Thebe::Material>();
		}

		std::filesystem::path outputTexturePath = (this->outputAssetsFolder / this->GenerateTextureBufferPath(inputTexturePath)).lexically_normal();
		if (!wxGetApp().GetGraphicsEngine()->DumpEnginePartToFile(outputTexturePath, outputTexture, THEBE_DUMP_FLAG_CAN_OVERWRITE))
		{
			THEBE_LOG("Failed to dump texture: %s", outputTexturePath.string().c_str());
			return Thebe::Reference<Thebe::Material>();
		}
	}

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
		this->texturesToBuildSet.insert(inputDiffuseTexturePath);
		std::filesystem::path outputDiffuseTexturePath = this->GenerateTextureBufferPath(inputDiffuseTexturePath);
		outputMaterial->SetTexturePath("diffuse", outputDiffuseTexturePath);
	}
	else
	{
		aiString inputAlbedoTexture;
		if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), &inputAlbedoTexture))
		{
			outputMaterial->SetShaderPath("Shaders/PBRShader.shader");

			std::filesystem::path inputAlbedoTexturePath = (this->inputSceneFileFolder / inputAlbedoTexture.C_Str()).lexically_normal();
			this->texturesToBuildSet.insert(inputAlbedoTexturePath);
			std::filesystem::path outputAlbedoTexturePath = this->GenerateTextureBufferPath(inputAlbedoTexturePath);
			outputMaterial->SetTexturePath("albedo", outputAlbedoTexturePath);

			aiString inputAmbientOcclusionTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), &inputAmbientOcclusionTexture))
			{
				std::filesystem::path inputAmbientOcclusionTexturePath = (this->inputSceneFileFolder / inputAmbientOcclusionTexture.C_Str()).lexically_normal();
				this->texturesToBuildSet.insert(inputAmbientOcclusionTexturePath);
				std::filesystem::path outputAmbientOcclusionTexturePath = this->GenerateTextureBufferPath(inputAmbientOcclusionTexturePath);
				outputMaterial->SetTexturePath("ambient_occlusion", outputAmbientOcclusionTexturePath);
			}

			aiString inputHeightTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), &inputHeightTexture))
			{
				std::filesystem::path inputHeightTexturePath = (this->inputSceneFileFolder / inputHeightTexture.C_Str()).lexically_normal();
				this->texturesToBuildSet.insert(inputHeightTexturePath);
				std::filesystem::path outputHeightTexturePath = this->GenerateTextureBufferPath(inputHeightTexturePath);
				outputMaterial->SetTexturePath("height", outputHeightTexturePath);
			}

			aiString inputMetalicTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), &inputMetalicTexture))
			{
				std::filesystem::path inputMetalicTexturePath = (this->inputSceneFileFolder / inputMetalicTexture.C_Str()).lexically_normal();
				this->texturesToBuildSet.insert(inputMetalicTexturePath);
				std::filesystem::path outputMetalicTexturePath = this->GenerateTextureBufferPath(inputMetalicTexturePath);
				outputMaterial->SetTexturePath("metalic", outputMetalicTexturePath);
			}

			aiString inputNormalTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_NORMAL_CAMERA, 0), &inputNormalTexture))
			{
				std::filesystem::path inputNormalTexturePath = (this->inputSceneFileFolder / inputNormalTexture.C_Str()).lexically_normal();
				this->texturesToBuildSet.insert(inputNormalTexturePath);
				std::filesystem::path outputNormalTexturePath = this->GenerateTextureBufferPath(inputNormalTexturePath);
				outputMaterial->SetTexturePath("normal", outputNormalTexturePath);
			}

			aiString inputRoughnessTexture;
			if (AI_SUCCESS == aiGetMaterialString(inputMaterial, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), &inputRoughnessTexture))
			{
				std::filesystem::path inputRoughnessTexturePath = (this->inputSceneFileFolder / inputRoughnessTexture.C_Str()).lexically_normal();
				this->texturesToBuildSet.insert(inputRoughnessTexturePath);
				std::filesystem::path outputRoughnessTexturePath = this->GenerateTextureBufferPath(inputRoughnessTexturePath);
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

Thebe::Reference<Thebe::TextureBuffer> SceneBuilder::GenerateTextureBuffer(const std::filesystem::path& inputTexturePath)
{
	Thebe::Reference<Thebe::TextureBuffer> outputTexture(new Thebe::TextureBuffer());
	outputTexture->SetGraphicsEngine(wxGetApp().GetGraphicsEngine());
	outputTexture->SetName(inputTexturePath.stem().string());
	outputTexture->SetBufferType(Thebe::Buffer::STATIC);
	outputTexture->SetCompressed(this->compressTextures);

	wxImage inputImage;
	if (!inputImage.LoadFile(inputTexturePath.string().c_str()))
	{
		THEBE_LOG("Failed to load file: %s", inputTexturePath.string().c_str());
		return Thebe::Reference<Thebe::TextureBuffer>();
	}

	D3D12_RESOURCE_DESC& gpuBufferDesc = outputTexture->GetResourceDesc();
	gpuBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	gpuBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpuBufferDesc.Width = inputImage.GetWidth();
	gpuBufferDesc.Height = inputImage.GetHeight();

	UINT64 outputBufferSize = 0;
	UINT numMips = 0;
	UINT64 mipWidth = (UINT)inputImage.GetWidth();
	UINT64 mipHeight = (UINT)inputImage.GetHeight();
	while (mipWidth >= 1 && mipHeight >= 1)
	{
		numMips++;
		outputBufferSize += mipWidth * mipHeight * outputTexture->GetBytesPerPixel();
		if ((mipWidth & 0x1) != 0 || (mipHeight & 0x1) != 0)
			break;
		mipWidth >>= 1;
		mipHeight >>= 1;
	}

	gpuBufferDesc.MipLevels = numMips;

	UINT bytesPerInputColorPixel = 3;
	UINT bytesPerInputAlphaPixel = 1;

	std::vector<UINT8>& outputBuffer = outputTexture->GetOriginalBuffer();
	outputBuffer.resize(outputBufferSize);

	mipWidth = (UINT)inputImage.GetWidth();
	mipHeight = (UINT)inputImage.GetHeight();
	UINT64 mipImageOffset = 0;

	for (UINT mipNumber = 0; mipNumber < numMips; mipNumber++)
	{
		wxImage sourceImage;
		if (mipNumber > 0)
			sourceImage = inputImage.Scale((int)mipWidth, (int)mipHeight, wxIMAGE_QUALITY_HIGH);
		else
			sourceImage = inputImage;

		UINT8* destinationImageBuffer = &outputBuffer.data()[mipImageOffset];

		for (UINT64 i = 0; i < mipHeight; i++)
		{
			for (UINT64 j = 0; j < mipWidth; j++)
			{
				UINT pixelOffset = i * mipWidth + j;

				const unsigned char* inputColor = &sourceImage.GetData()[pixelOffset * bytesPerInputColorPixel];
				const unsigned char* inputAlpha = sourceImage.HasAlpha() ? &sourceImage.GetAlpha()[pixelOffset * bytesPerInputAlphaPixel] : nullptr;

				UINT8* outputPixel = &destinationImageBuffer[pixelOffset * outputTexture->GetBytesPerPixel()];

				outputPixel[0] = inputColor[0];
				outputPixel[1] = inputColor[1];
				outputPixel[2] = inputColor[2];
				outputPixel[3] = inputAlpha ? inputAlpha[0] : 0;
			}
		}

		mipImageOffset += mipWidth * mipHeight * outputTexture->GetBytesPerPixel();
		mipWidth >>= 1;
		mipHeight >>= 1;
	}

	return outputTexture;
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

std::filesystem::path SceneBuilder::GenerateTextureBufferPath(const std::filesystem::path& inputTexturePath)
{
	Thebe::GraphicsEngine* graphicsEngine = wxGetApp().GetGraphicsEngine();
	std::filesystem::path relativeInputTexturePath = inputTexturePath;
	bool found = graphicsEngine->GetRelativeToAssetFolder(relativeInputTexturePath);
	THEBE_ASSERT(found);
	std::string inputTextureName(inputTexturePath.stem().string());
	std::string outputTextureName = inputTextureName + ".texture_buffer";
	outputTextureName = this->NoSpaces(this->PrefixWithSceneName(outputTextureName));
	std::filesystem::path outputTexturePath = relativeInputTexturePath.parent_path() / outputTextureName;
	return outputTexturePath;
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