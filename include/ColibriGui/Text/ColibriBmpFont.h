
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "OgreImage2.h"
#include "OgreVector2.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

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
		Ogre::Vector2 topLeft;
		bool          isNewline;
		bool          isTab;
		BmpChar *     bmpChar;
	};

	typedef std::vector<BmpGlyph> BmpGlyphVec;

	class BmpFont
	{
	protected:
		Ogre::Image2         m_fontTexture;
		std::vector<BmpChar> m_chars;

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

		void renderString( const std::string &utf8Str, BmpGlyphVec &outShapes );
	};
}  // namespace Colibri

COLIBRIGUI_ASSUME_NONNULL_END
