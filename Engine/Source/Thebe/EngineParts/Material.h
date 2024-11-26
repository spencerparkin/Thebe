#pragma once

#include "Thebe/EnginePart.h"
#include <d3d12.h>

namespace Thebe
{
	class Texture;
	class Shader;

	/**
	 *
	 */
	class THEBE_API Material : public EnginePart
	{
	public:
		Material();
		virtual ~Material();

		virtual bool Setup() override;
		virtual void Shutdown() override;

		void Bind(ID3D12GraphicsCommandList* commandList);

	private:
		Reference<Shader> shader;
		std::map<std::string, Reference<Texture>> textureMap;
	};
}