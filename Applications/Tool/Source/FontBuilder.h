#pragma once

#include "Builder.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_MODULE_H
#include <wx/string.h>
#include <filesystem>

class FontBuilder : public Builder
{
public:
	FontBuilder();
	virtual ~FontBuilder();

	bool GenerateFont(const wxString& fontFile, const std::filesystem::path& outputAssetsFolder);

private:
	FT_Library library;
};