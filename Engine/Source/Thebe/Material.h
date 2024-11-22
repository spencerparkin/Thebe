#pragma once

#include "Reference.h"

namespace Thebe
{
	class Texture;

	/**
	 *
	 */
	class THEBE_API Material : public ReferenceCounted
	{
	public:
		Material();
		virtual ~Material();

	private:
		// TODO: Own PSO and root signature.
		Reference<Texture> diffuseTexture;
	};
}