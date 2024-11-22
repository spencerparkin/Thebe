#pragma once

#include "Node.h"

namespace Thebe
{
	class Texture;

	/**
	 *
	 */
	class THEBE_API Material : public Node
	{
	public:
		Material();
		virtual ~Material();

	private:
		// TODO: Own PSO and root signature.
		Reference<Texture> diffuseTexture;
	};
}