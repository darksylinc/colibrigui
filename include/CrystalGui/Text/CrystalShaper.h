
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"
#include "OgreVector2.h"

#include "hb.h"

#include <vector>
#include <string>

typedef struct FT_FaceRec_*  FT_Face;
typedef struct FT_LibraryRec_  *FT_Library;

namespace Crystal
{
	struct ShapedGlyph
	{
		Ogre::Vector2 advance;
		Ogre::Vector2 offset;
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

		uint32_t	m_ptSize; //Font size in points

	public:
		Shaper( hb_script_t script, const char *fontLocation,
				const std::string &language,
				ShaperManager *shaperManager );
		~Shaper();

		void setFeatures( const std::vector<hb_feature_t> &features );
		void addFeatures( const hb_feature_t &feature );

		/// Set the font size in points. Note the returned value by getFontSize
		/// can differ as the float is internally converted to 26.6 fixed point
		void setFontSizeFloat( float ptSize );
		void setFontSize26d6( uint32_t ptSize );
		float getFontSizeFloat() const;
		uint32_t getFontSize26d6() const;

		void renderString( const uint16_t *utf16Str, size_t stringLength,
						   hb_direction_t dir, ShapedGlyphVec &outShapes );

		bool operator < ( const Shaper &other ) const;

		static const hb_feature_t LigatureOff;
		static const hb_feature_t LigatureOn;
		static const hb_feature_t KerningOff;
		static const hb_feature_t KerningOn;
		static const hb_feature_t CligOff;
		static const hb_feature_t CligOn;
	};
}
