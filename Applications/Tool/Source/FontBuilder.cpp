#include "FontBuilder.h"
#include "TextureBuilder.h"
#include "Thebe/Log.h"
#include "Thebe/Utilities/JsonHelper.h"
#include "App.h"
#include <wx/image.h>
#include <wx/filename.h>
#include <fstream>

FontBuilder::FontBuilder()
{
	this->library = nullptr;
}

/*virtual*/ FontBuilder::~FontBuilder()
{
	if (this->library)
		FT_Done_Library(this->library);
}

bool FontBuilder::GenerateFont(const wxString& fontFile, const std::filesystem::path& outputAssetsFolder)
{
	using namespace ParseParty;

	wxFileName nativeFontFile(fontFile);
	wxString fontFolder = nativeFontFile.GetPath();
	wxString fontName = nativeFontFile.GetName();

	wxFileName atlasFileName;
	atlasFileName.SetPath(fontFolder);
	atlasFileName.SetName(fontName);
	atlasFileName.SetExt("png");

	wxFileName fontFileName;
	fontFileName.SetPath(fontFolder);
	fontFileName.SetName(fontName);
	fontFileName.SetExt("font");

	FT_Error error = 0;

	if (!this->library)
	{
		error = FT_Init_FreeType(&this->library);
		if (error)
		{
			THEBE_LOG("Got error %d trying to initialize free-type library.", error);
			return false;
		}
	}

	FT_Face face = nullptr;
	auto fontRootValue = new JsonObject();
	bool success = false;
	do
	{
		fontRootValue->SetValue("name", new JsonString((const char*)fontFileName.GetName()));
		
		auto characterArrayValue = new JsonArray();
		fontRootValue->SetValue("character_array", characterArrayValue);

		error = FT_New_Face(this->library, (const char*)fontFile.c_str(), 0, &face);
		if (error)
		{
			THEBE_LOG("Got error %d trying to create new font-face from file %s.", error, (const char* )fontFile.c_str());
			break;
		}

		THEBE_LOG("Num glyphs found: %d", face->num_glyphs);
		THEBE_LOG("Num faces found: %d", face->num_faces);
		THEBE_LOG("Num charmaps found: %d", face->num_charmaps);
		THEBE_LOG("Num fixed sizes found: %d", face->num_fixed_sizes);
	
		error = FT_Set_Pixel_Sizes(face, 45, 45);
		if (error)
		{
			THEBE_LOG("Failed to set pixel sizes with error: %d", error);
			break;
		}

		FT_UInt charWidth = 64;
		FT_UInt charHeight = 64;
		FT_UInt atlasWidth = 1024;
		FT_UInt atlasHeight = 1024;
		FT_UInt atlasNumRows = atlasHeight / charHeight;
		FT_UInt atlasNumCols = atlasWidth / charWidth;

		if (atlasWidth * atlasHeight < 256 * charWidth * charHeight)
		{
			THEBE_LOG("Can't fit 256 characters of size %d x %d in a %d x %d image.", charWidth, charHeight, atlasWidth, atlasHeight);
			break;
		}

		wxImage fontAtlasImage(atlasWidth, atlasHeight);
		fontAtlasImage.SetAlpha(nullptr);

		unsigned char* atlasImageColorBuffer = fontAtlasImage.GetData();
		unsigned char* atlasImageAlphaBuffer = fontAtlasImage.GetAlpha();

		FT_UInt bytesPerAtlasColorPixel = 3;
		FT_UInt bytesPerAtlasAlphaPixel = 1;

		::memset(atlasImageColorBuffer, 0x00, atlasWidth * atlasHeight * bytesPerAtlasColorPixel);
		::memset(atlasImageAlphaBuffer, 0x00, atlasWidth * atlasHeight * bytesPerAtlasAlphaPixel);

		// Loop through all 256 ASCII characters codes.
		FT_UInt i;
		for (i = 0; i < 256; i++)
		{
			auto characterValue = new JsonObject();
			characterArrayValue->PushValue(characterValue);

			FT_UInt glyphIndex = FT_Get_Char_Index(face, i);

			error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
			if (error)
			{
				THEBE_LOG("Failed to load glyph at index %d for char %c with error: %d", glyphIndex, char(i), error);
				break;
			}

			if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
			{
				error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
				if (error)
				{
					THEBE_LOG("Failed to render glyph at index %d for char %c with error: %d", glyphIndex, char(i), error);
					break;
				}
			}

			FT_GlyphSlot slot = face->glyph;
			double advance = double(slot->advance.x >> 6) / double(atlasWidth);
			characterValue->SetValue("advance", new JsonFloat(advance));

			double penOffsetX = double(slot->bitmap_left) / double(atlasWidth);
			double penOffsetY = double(int(slot->bitmap_top) - int(slot->bitmap.rows)) / double(atlasHeight);
			characterValue->SetValue("pen_offset_x", new JsonFloat(penOffsetX));
			characterValue->SetValue("pen_offset_y", new JsonFloat(penOffsetY));

			if (!slot->bitmap.buffer)
			{
				THEBE_LOG("Character %d didn't have a bitmap associated with it.  (Could just be a space.)", i);
				characterValue->SetValue("no_glyph", new JsonBool(true));
			}
			else
			{
				if (slot->bitmap.width > charWidth || slot->bitmap.rows > charHeight)
				{
					THEBE_LOG("Rendered glyph is size %d x %d, which won't fit in %d x %d.", slot->bitmap.width, slot->bitmap.rows, charWidth, charHeight);
					break;
				}

				FT_UInt atlasRow = i / atlasNumCols;
				FT_UInt atlasCol = i % atlasNumCols;

				FT_UInt atlasY = atlasRow * charHeight;
				FT_UInt atlasX = atlasCol * charWidth;

				FT_UInt bytesPerGlyphPixel = 0;
				if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
					bytesPerGlyphPixel = 1;
				else
				{
					THEBE_LOG("Pixel mode %d not yet accounted for.", slot->bitmap.pixel_mode);
					break;
				}

				double minU = double(atlasX) / double(atlasWidth);
				double minV = double(atlasY) / double(atlasHeight);
				double maxU = double(atlasX + slot->bitmap.width) / double(atlasWidth);
				double maxV = double(atlasY + slot->bitmap.rows) / double(atlasHeight);

				characterValue->SetValue("min_u", new JsonFloat(minU));
				characterValue->SetValue("min_v", new JsonFloat(minV));
				characterValue->SetValue("max_u", new JsonFloat(maxU));
				characterValue->SetValue("max_v", new JsonFloat(maxV));

				for (int row = 0; row < slot->bitmap.rows; row++)
				{
					for (int col = 0; col < slot->bitmap.width; col++)
					{
						unsigned char* glyphPixel = &slot->bitmap.buffer[row * slot->bitmap.pitch + col * bytesPerGlyphPixel];

						unsigned char* atlasColorPixel = &atlasImageColorBuffer[(atlasY + row) * atlasWidth * bytesPerAtlasColorPixel + (atlasX + col) * bytesPerAtlasColorPixel];
						unsigned char* atlasAlphaPixel = &atlasImageAlphaBuffer[(atlasY + row) * atlasWidth * bytesPerAtlasAlphaPixel + (atlasX + col) * bytesPerAtlasAlphaPixel];

						switch (slot->bitmap.pixel_mode)
						{
							case FT_PIXEL_MODE_GRAY:
							{
								double alpha = double(*glyphPixel) / double(slot->bitmap.num_grays);
								if (alpha > 1.0)
									alpha = 1.0;
								else if (alpha < 0.0)
									alpha = 0.0;

								unsigned char greyValue = unsigned char(alpha * 255.0);

								atlasColorPixel[0] = 0x00;
								atlasColorPixel[1] = 0x00;
								atlasColorPixel[2] = 0x00;
								atlasAlphaPixel[0] = greyValue;

								break;
							}
						}
					}
				}
			}
		}

		if (i < 256)
			break;

		if (!fontAtlasImage.SaveFile(atlasFileName.GetFullPath(), wxBITMAP_TYPE_PNG))
		{
			THEBE_LOG("Failed to save atlas image file: %s", (const char*)atlasFileName.GetFullPath().c_str());
			break;
		}

		std::filesystem::path inputTexturePath((const char*)atlasFileName.GetFullPath().c_str());
		TextureBuilder textureBuilder;
		textureBuilder.AddTexture(inputTexturePath, TextureBuilder::TextureBuildInfo{ DXGI_FORMAT_A8_UNORM, 1 });
		if (!textureBuilder.GenerateTextures(outputAssetsFolder))
		{
			THEBE_LOG("Failed to generate atlas texture!");
			break;
		}

		std::filesystem::path outputTexturePath = textureBuilder.GenerateTextureBufferPath(inputTexturePath);
		fontRootValue->SetValue("texture", new JsonString(outputTexturePath.string().c_str()));

		std::string jsonString;
		if (!fontRootValue->PrintJson(jsonString))
			break;

		std::ofstream fileStream;
		fileStream.open((const char*)fontFileName.GetFullPath(), std::ios::out);
		if (!fileStream.is_open())
			break;

		fileStream.write(jsonString.c_str(), jsonString.length());
		fileStream.close();

		success = true;
	} while (false);

	if (face)
		FT_Done_Face(face);

	delete fontRootValue;

	return success;
}