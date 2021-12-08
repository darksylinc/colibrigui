
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "OgreImage2.h"
#include "OgreVector2.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	class HlmsColibri;
}

namespace Colibri
{
	struct BmpChar
	{
		uint32_t id;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
		uint16_t xoffset;
		uint16_t yoffset;
		uint16_t xadvance;

		bool operator<( const BmpChar &other ) const { return this->id < other.id; }
	};

	struct BmpGlyph
	{
		bool           isNewline;
		bool           isTab;
		BmpChar const *bmpChar;
	};

	typedef std::vector<BmpGlyph> BmpGlyphVec;

	class BmpFont
	{
	protected:
		std::string       m_textureName;
		Ogre::TextureGpu *colibrigui_nullable m_fontTexture;
		Ogre::HlmsDatablock *colibrigui_nullable m_datablock;

		FontSize m_fontSize;

		std::vector<BmpChar> m_chars;

		BmpChar m_emptyChar;

		uint16_t m_fontIdx;

		/** Populates m_chars
		@param inFntDat
			Font file string. It's actually const, but we temporarily swap
			its contents as a compiler optimization
		*/
		void parseFntFile( std::vector<char> &inFntData );

	public:
		BmpFont( const char *fontLocation, ShaperManager *shaperManager );
		~BmpFont();

		void setOgre( Ogre::HlmsColibri *hlms, Ogre::TextureGpuManager *textureManager );

		void renderString( const std::string &utf8Str, BmpGlyphVec &outShapes );

		Ogre::Vector4 getInvResolution() const;

		FontSize getBakedFontSize() const { return m_fontSize; }

		/// This pointer can be casted to HlmsColibriDatablock
		Ogre::HlmsDatablock *colibrigui_nullable getDatablock() const { return m_datablock; }
	};
}  // namespace Colibri

COLIBRIGUI_ASSUME_NONNULL_END
