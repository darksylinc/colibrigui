
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"

#include "hb.h"

#include <vector>
#include <string>

typedef struct FT_FaceRec_*  FT_Face;
typedef struct FT_LibraryRec_  *FT_Library;

namespace Crystal
{
	struct CachedGlyph
	{
		uint32_t codepoint;
		uint32_t ptSize;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
	};

	class Shaper
	{
	protected:
		hb_script_t m_script;
		FT_Face		m_ftFont;
		hb_font_t	*m_hbFont;
		hb_buffer_t	*m_buffer;

		std::vector<hb_feature_t> m_features;

		FT_Library		m_library;
		ShaperManager	*m_shaperManager;

		uint32_t	m_ptSize; //Font size in points

	public:
		Shaper( hb_script_t script, const char *fontLocation,
				hb_direction_t direction, const std::string &language,
				ShaperManager *shaperManager );
		~Shaper();

		void setFeatures( const std::vector<hb_feature_t> &features );

		/// Set the font size in points. Note the returned value by getFontSize
		/// can differ as the float is internally converted to 26.6 fixed point
		void setFontSize( float ptSize );
		float getFontSize() const;

		void renderString( const std::string &utf8Str );
		void renderString( const char *utf8Str, size_t stringLength );

		bool operator < ( const Shaper &other ) const;

		static const hb_feature_t LigatureOff;
		static const hb_feature_t LigatureOn;
		static const hb_feature_t KerningOff;
		static const hb_feature_t KerningOn;
		static const hb_feature_t CligOff;
		static const hb_feature_t CligOn;
	};
}
