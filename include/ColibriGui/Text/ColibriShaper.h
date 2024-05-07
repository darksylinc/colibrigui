
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"
#include "OgreVector2.h"

#include "hb.h"

#include <string>
#include <vector>

COLIBRI_ASSUME_NONNULL_BEGIN

typedef struct FT_FaceRec_    *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;

#ifdef __ANDROID__
struct AAsset;
typedef struct FT_StreamRec_ FT_StreamRec;
#endif

namespace Colibri
{
	struct ShapedGlyph
	{
		Ogre::Vector2 advance;
		Ogre::Vector2 offset;
		/// The caret position at which this shape should be placed at. Calculated by Label.
		/// It's in physical pixels i.e. valid range [0; ColibriManager::getHalfWindowResolution() * 2)
		Ogre::Vector2      caretPos;
		bool               isNewline;
		bool               isWordBreaker;
		bool               isRtl : 1;
		bool               isPrivateArea : 1;
		bool               isTab;
		uint32_t           richTextIdx;
		uint32_t           clusterStart;
		uint32_t           clusterLength;
		CachedGlyph const *glyph;
	};
	typedef std::vector<ShapedGlyph> ShapedGlyphVec;

	class Shaper
	{
	protected:
		hb_script_t   m_script;
		FT_Face       m_ftFont;
		hb_language_t m_hbLanguage;
		hb_font_t    *m_hbFont;
		hb_buffer_t  *m_buffer;

		std::vector<hb_feature_t> m_features;

		FT_Library     m_library;
		ShaperManager *m_shaperManager;

#ifdef __ANDROID__
		AAsset *colibri_nullable m_asset;
		FT_StreamRec            *m_stream;
#endif

		FontSize m_ptSize;  // Font size in points
		uint16_t m_fontIdx;

		/// See setUseCodepoint0ForRaster()
		bool m_useCodepoint0ForRaster;

		size_t renderWithSubstituteFont( const uint16_t *utf16Str, size_t stringLength,
										 hb_direction_t dir, uint32_t richTextIdx,
										 uint32_t clusterOffset, ShapedGlyphVec &outShapes,
										 bool &bOutHasPrivateUse );

	public:
		Shaper( hb_script_t script, const char *fontLocation, const std::string &language,
				ShaperManager *shaperManager );
		~Shaper();

		void setFeatures( const std::vector<hb_feature_t> &features );
		void addFeatures( const hb_feature_t &feature );

		void     setFontSize( FontSize ptSize );
		FontSize getFontSize() const;

		/** When raster fonts are used, we need to fetch a dummy glyph to base our parameters
			and align the raster glyphs (e.g. emoji).

			For that we have two methods. One based on codepoint 0 and another on codepoint 'Y'.
			Neither of them is perfect so toggle this setting if the emojis are misaligned.
		@remarks
			Some fonts do not specify codepoint 0 and return invalid data, thus you'll have to
			set this to true.

			See BmpFont::addFontAlignment for manually tweaking alignment between
			normal and specific BMP fonts.
		@param useCodepoint0ForRaster
			True to use codepoint 0 (not '0', but '\0').
			False to use 'Y'
		*/
		void setUseCodepoint0ForRaster( bool useCodepoint0ForRaster );
		bool getUseCodepoint0ForRaster() const;

		size_t renderString( const uint16_t *utf16Str, size_t stringLength, hb_direction_t dir,
							 uint32_t richTextIdx, uint32_t clusterOffset, ShapedGlyphVec &outShapes,
							 bool &bOutHasPrivateUse, bool substituteIfNotFound );

		bool operator<( const Shaper &other ) const;

		static const hb_feature_t LigatureOff;
		static const hb_feature_t LigatureOn;
		static const hb_feature_t KerningOff;
		static const hb_feature_t KerningOn;
		static const hb_feature_t CligOff;
		static const hb_feature_t CligOn;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
