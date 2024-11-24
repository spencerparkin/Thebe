#pragma once

#include "Thebe/EnginePart.h"

namespace Thebe
{
	class Texture;

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

	private:
		// TODO: Own PSO and root signature.
		Reference<Texture> diffuseTexture;
	};
}