#pragma once

#include "App.h"
#include "Builder.h"
#include "Thebe/EngineParts/CubeMapBuffer.h"

/**
 * Here we build a cube-map from 6 given image files.  If we were
 * really fancy, though, we could generate a cube-map from a scene
 * that is currently rendering in the engine.  Doing so might be a
 * worth-while exercise.
 */
class CubeMapBuilder : public Builder
{
public:
	CubeMapBuilder();
	virtual ~CubeMapBuilder();

	bool BuildCubeMap(const wxArrayString& inputCubeMapTexturesArray);

private:
	int GetTexturePathKey(const wxString& inputTexturePath);

	bool compressTextures;
};