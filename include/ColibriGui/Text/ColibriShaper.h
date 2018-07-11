
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"
#include "OgreVector2.h"

#include "hb.h"

#include <vector>
#include <string>

COLIBRIGUI_ASSUME_NONNULL_BEGIN

typedef struct FT_FaceRec_*  FT_Face;
typedef struct FT_LibraryRec_  *FT_Library;

namespace Colibri
{
	struct ShapedGlyph
	{
		Ogre::Vector2 advance;
		Ogre::Vector2 offset;
		/// The caret position at which this shape should be placed at. Calculated by Label.
		/// It's in physical pixels i.e. valid range [0; ColibriManager::getHalfWindowResolution() * 2)
		Ogre::Vector2 caretPos;
		bool isNewline;
		bool isWordBreaker;
		bool isRtl;
		bool isTab;
		uint32_t richTextIdx;
		uint32_t clusterStart;
		CachedGlyph const *glyph;
	};
	typedef std::vector<ShapedGlyph> ShapedGlyphVec;

	class Shaper
	{
	protected:
		hb_script_t		m_script;
		FT_Face			m_ftFont;
		hb_language_t	m_hbLanguage;
		hb_font_t		*m_hbFont;
		hb_buffer_t		*m_buffer;

		std::vector<hb_feature_t> m_features;

		FT_Library		m_library;
		ShaperManager	*m_shaperManager;

		FontSize	m_ptSize; //Font size in points

		bool renderWithSubstituteFont( const uint16_t *utf16Str, size_t stringLength, hb_direction_t dir,
									   uint32_t richTextIdx, ShapedGlyphVec &outShapes,
									   uint32_t clusterStart );

	public:
		Shaper( hb_script_t script, const char *fontLocation,
				const std::string &language,
				ShaperManager *shaperManager );
		~Shaper();

		void setFeatures( const std::vector<hb_feature_t> &features );
		void addFeatures( const hb_feature_t &feature );

		void setFontSize( FontSize ptSize );
		FontSize getFontSize() const;

		void renderString( const uint16_t *utf16Str, size_t stringLength, hb_direction_t dir,
						   uint32_t richTextIdx, ShapedGlyphVec &outShapes, bool substituteIfNotFound );

		bool operator < ( const Shaper &other ) const;

		static const hb_feature_t LigatureOff;
		static const hb_feature_t LigatureOn;
		static const hb_feature_t KerningOff;
		static const hb_feature_t KerningOn;
		static const hb_feature_t CligOff;
		static const hb_feature_t CligOn;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
