
#include "CrystalGui/Text/CrystalShaper.h"
#include "CrystalGui/Text/CrystalShaperManager.h"

#include "CrystalGui/CrystalManager.h"

#include "OgreLwString.h"

#include "ft2build.h"
#include "freetype/freetype.h"

#include "hb-ft.h"

namespace Crystal
{
	/*  See http://www.microsoft.com/typography/otspec/name.htm
		for a list of some possible platform-encoding pairs.
		We're interested in 0-3 aka 3-1 - UCS-2.
		Otherwise, fail. If a font has some unicode map, but lacks
		UCS-2 - it is a broken or irrelevant font. What exactly
		Freetype will select on face load (it promises most wide
		unicode, and if that will be slower that UCS-2 - left as
		an excercise to check.
	*/
	static int force_ucs2_charmap( FT_Face ftf )
	{
		for(int i = 0; i < ftf->num_charmaps; i++)
		{
			if ((  (ftf->charmaps[i]->platform_id == 0)
				&& (ftf->charmaps[i]->encoding_id == 3))
			   || ((ftf->charmaps[i]->platform_id == 3)
				&& (ftf->charmaps[i]->encoding_id == 1)))
			{
				return FT_Set_Charmap( ftf, ftf->charmaps[i] );
			}
		}
		return -1;
	}

	static const hb_tag_t KernTag = HB_TAG('k', 'e', 'r', 'n'); // kerning operations
	static const hb_tag_t LigaTag = HB_TAG('l', 'i', 'g', 'a'); // standard ligature substitution
	static const hb_tag_t CligTag = HB_TAG('c', 'l', 'i', 'g'); // contextual ligature substitution

	const hb_feature_t Shaper::LigatureOff = { LigaTag, 0, 0, std::numeric_limits<unsigned int>::max() };
	const hb_feature_t Shaper::LigatureOn  = { LigaTag, 1, 0, std::numeric_limits<unsigned int>::max() };
	const hb_feature_t Shaper::KerningOff  = { KernTag, 0, 0, std::numeric_limits<unsigned int>::max() };
	const hb_feature_t Shaper::KerningOn   = { KernTag, 1, 0, std::numeric_limits<unsigned int>::max() };
	const hb_feature_t Shaper::CligOff     = { CligTag, 0, 0, std::numeric_limits<unsigned int>::max() };
	const hb_feature_t Shaper::CligOn      = { CligTag, 1, 0, std::numeric_limits<unsigned int>::max() };

	Shaper::Shaper( hb_script_t script, const char *fontLocation,
					const std::string &language,
					ShaperManager *shaperManager ) :
		m_script( script ),
		m_ftFont( 0 ),
		m_hbFont( 0 ),
		m_buffer( 0 ),
		m_library( shaperManager->getFreeTypeLibrary() ),
		m_shaperManager( shaperManager )
	{
		FT_Error errorCode = FT_New_Face( m_library, fontLocation, 0, &m_ftFont );
		if( errorCode )
		{
			LogListener *log = m_shaperManager->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[Freetype2 error] Could not open font ", fontLocation, " errorCode: ",
						errorCode, " Desc: ", ShaperManager::getErrorMessage( errorCode ) );
			log->log( errorMsg.c_str(), LogSeverity::Fatal );
		}

		setFontSizeFloat( 13.0f );
		force_ucs2_charmap( m_ftFont );

		m_hbFont = hb_ft_font_create( m_ftFont, NULL );
		m_buffer = hb_buffer_create();

		m_hbLanguage = hb_language_from_string( language.c_str(), language.size() );
	}
	//-------------------------------------------------------------------------
	Shaper::~Shaper()
	{
		hb_buffer_destroy( m_buffer );
		hb_font_destroy( m_hbFont );

		FT_Error errorCode = FT_Done_Face( m_ftFont );

		if( errorCode )
		{
			LogListener *log = m_shaperManager->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[Freetype2 error] Error while closing font handle", " errorCode: ",
						errorCode, " Desc: ", ShaperManager::getErrorMessage( errorCode ) );
			log->log( errorMsg.c_str(), LogSeverity::Fatal );
		}
	}
	//-------------------------------------------------------------------------
	void Shaper::setFeatures( const std::vector<hb_feature_t> &features )
	{
		m_features = features;
	}
	//-------------------------------------------------------------------------
	void Shaper::addFeatures( const hb_feature_t &feature )
	{
		m_features.push_back( feature );
	}
	//-------------------------------------------------------------------------
	void Shaper::setFontSizeFloat( float ptSize )
	{
		setFontSize26d6( static_cast<uint32_t>( round( ptSize * 64.0f ) ) );
	}
	//-------------------------------------------------------------------------
	void Shaper::setFontSize26d6( uint32_t ptSize )
	{
		const uint32_t oldSize = m_ptSize;
		m_ptSize = ptSize;

		if( oldSize != m_ptSize )
		{
			const FT_UInt deviceHdpi = 72u;
			const FT_UInt deviceVdpi = 72u;
			FT_Error errorCode = FT_Set_Char_Size( m_ftFont, 0, (FT_F26Dot6)ptSize,
												   deviceHdpi, deviceVdpi );
			if( crystalgui_unlikely( errorCode ) )
			{
				LogListener *log = m_shaperManager->getLogListener();
				char tmpBuffer[512];
				Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer,
																		   sizeof(tmpBuffer) ) );

				errorMsg.clear();
				errorMsg.a( "[Freetype2 error] Could set font size to ",
							Ogre::LwString::Float( getFontSizeFloat(), 2 ),
							". errorCode: ", errorCode, " Desc: ",
							ShaperManager::getErrorMessage( errorCode ) );
				log->log( errorMsg.c_str(), LogSeverity::Error );
			}
			else if( crystalgui_likely( m_hbFont != 0 ) )
				hb_ft_font_changed( m_hbFont );
		}
	}
	//-------------------------------------------------------------------------
	float Shaper::getFontSizeFloat() const
	{
		return m_ptSize / 64.0f;
	}
	//-------------------------------------------------------------------------
	uint32_t Shaper::getFontSize26d6() const
	{
		return m_ptSize;
	}
	//-------------------------------------------------------------------------
	void Shaper::renderString( const uint16_t *utf16Str, size_t stringLength,
							   hb_direction_t dir, ShapedGlyphVec &outShapes )
	{
		ShapedGlyphVec shapesVec;
		shapesVec.swap( outShapes );

		hb_buffer_clear_contents( m_buffer );
		hb_buffer_set_direction( m_buffer, dir );

		hb_buffer_set_script( m_buffer, m_script );
		hb_buffer_set_language( m_buffer, m_hbLanguage );

		hb_buffer_add_utf16( m_buffer, utf16Str, stringLength, 0, stringLength );
		hb_shape( m_hbFont, m_buffer, m_features.empty() ? 0 : &m_features[0], m_features.size() );

		unsigned int glyphCount;
		hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos( m_buffer, &glyphCount );
		hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions( m_buffer, &glyphCount );

		for( size_t i=0; i<glyphCount; ++i )
		{
			if( glyphInfo[i].codepoint == '\n' )
			{
				ShapedGlyph shapedGlyph;
				memset( &shapedGlyph, 0, sizeof( ShapedGlyph ) );
				shapesVec.push_back( shapedGlyph );
			}
			else
			{
				const CachedGlyph *glyph = m_shaperManager->acquireGlyph( m_ftFont,
																		  glyphInfo[i].codepoint,
																		  m_ptSize );

				ShapedGlyph shapedGlyph;
				shapedGlyph.advance = Ogre::Vector2( glyphPos[i].x_advance,
													 glyphPos[i].y_advance ) / 64.0f;
				shapedGlyph.offset = Ogre::Vector2( glyphPos[i].x_offset,
													glyphPos[i].y_offset ) / 64.0f;
				shapedGlyph.glyph = glyph;
				shapesVec.push_back( shapedGlyph );
			}
		}

		shapesVec.swap( outShapes );
	}
	//-------------------------------------------------------------------------
	bool Shaper::operator < ( const Shaper &other ) const
	{
		return this->m_script < other.m_script;
	}
}
