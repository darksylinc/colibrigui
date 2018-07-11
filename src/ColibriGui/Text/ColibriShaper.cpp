
#include "ColibriGui/Text/ColibriShaper.h"
#include "ColibriGui/Text/ColibriShaperManager.h"

#include "ColibriGui/ColibriManager.h"

#include "OgreLwString.h"

#include "ft2build.h"
#include "freetype/freetype.h"

#include "hb-ft.h"

#include "utf16.h"
#include "uscript.h"

namespace Colibri
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
		m_shaperManager( shaperManager ),
		m_ptSize( 0u )
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

		setFontSize( FontSize( 24.0f ) );
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
	void Shaper::setFontSize( FontSize ptSize )
	{
		const FontSize oldSize = m_ptSize;
		m_ptSize = ptSize;

		if( oldSize != m_ptSize )
		{
			const FT_UInt deviceHdpi = 96u;
			const FT_UInt deviceVdpi = 96u;
			FT_Error errorCode = FT_Set_Char_Size( m_ftFont, 0, (FT_F26Dot6)ptSize.value26d6,
												   deviceHdpi, deviceVdpi );
			if( colibrigui_unlikely( errorCode ) )
			{
				LogListener *log = m_shaperManager->getLogListener();
				char tmpBuffer[512];
				Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer,
																		   sizeof(tmpBuffer) ) );

				errorMsg.clear();
				errorMsg.a( "[Freetype2 error] Could set font size to ",
							Ogre::LwString::Float( ptSize.asFloat(), 2 ),
							". errorCode: ", errorCode, " Desc: ",
							ShaperManager::getErrorMessage( errorCode ) );
				log->log( errorMsg.c_str(), LogSeverity::Error );
			}
			else if( colibrigui_likely( m_hbFont != 0 ) )
				hb_ft_font_changed( m_hbFont );
		}
	}
	//-------------------------------------------------------------------------
	FontSize Shaper::getFontSize() const
	{
		return m_ptSize;
	}
	//-------------------------------------------------------------------------
	size_t Shaper::renderWithSubstituteFont( const uint16_t *utf16Str, size_t stringLength,
											 hb_direction_t dir, uint32_t richTextIdx,
											 uint32_t clusterOffset, ShapedGlyphVec &outShapes )
	{
		size_t currentSize = outShapes.size();
		size_t numWrittenCodepoints = 0;

		const ShaperManager::ShaperVec &shapers = m_shaperManager->getShapers();

		ShaperManager::ShaperVec::const_iterator itor = shapers.begin() + 1u;
		ShaperManager::ShaperVec::const_iterator end  = shapers.end();

		while( itor != end && outShapes.size() == currentSize )
		{
			if( *itor != this )
			{
				Shaper *otherShaper = *itor;
				otherShaper->setFontSize( m_ptSize );
				numWrittenCodepoints = otherShaper->renderString( utf16Str, stringLength, dir,
																  richTextIdx, clusterOffset,
																  outShapes, false );
			}

			++itor;
		}

		return numWrittenCodepoints;
	}
	//-------------------------------------------------------------------------
	size_t Shaper::renderString( const uint16_t *utf16Str, size_t stringLength,
								 hb_direction_t dir, uint32_t richTextIdx, uint32_t clusterOffset,
								 ShapedGlyphVec &outShapes, bool substituteIfNotFound )
	{
		size_t numWrittenCodepoints = stringLength;

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
			if( glyphInfo[i].codepoint == 0 )
			{
				if( !substituteIfNotFound )
				{
					//Abort rendering this sequence if we're a substitute font
					//and don't know what this character is.
					if( dir != HB_DIRECTION_RTL )
						numWrittenCodepoints = glyphInfo[i].cluster - glyphInfo[0].cluster;
					else
						numWrittenCodepoints = stringLength - glyphInfo[i].cluster;

					break;
				}

				size_t clusterLength;

				size_t firstCluster;
				size_t numUnknownGlyphs = 1u;

				while( i + numUnknownGlyphs < glyphCount &&
					   glyphInfo[i + numUnknownGlyphs].codepoint == 0 )
				{
					++numUnknownGlyphs;
				}

				if( dir != HB_DIRECTION_RTL )
				{
					firstCluster = glyphInfo[i].cluster;
					if( i + numUnknownGlyphs < glyphCount )
						clusterLength = glyphInfo[i+numUnknownGlyphs].cluster - glyphInfo[i].cluster;
					else
						clusterLength = stringLength - glyphInfo[i].cluster;
				}
				else
				{
					firstCluster = glyphInfo[i+numUnknownGlyphs-1u].cluster;
					if( i > 0 )
						clusterLength = glyphInfo[i-1u].cluster - glyphInfo[i+numUnknownGlyphs-1u].cluster;
					else
						clusterLength = stringLength - glyphInfo[i+numUnknownGlyphs-1u].cluster;
				}

				size_t replacedCodepoints = renderWithSubstituteFont( &utf16Str[firstCluster],
																	  clusterLength, dir, richTextIdx,
																	  clusterOffset + firstCluster,
																	  shapesVec );

				if( replacedCodepoints == clusterLength )
					i += numUnknownGlyphs;
				else
				{
					if( dir != HB_DIRECTION_RTL )
					{
						const size_t nextUnknownCluster = firstCluster + replacedCodepoints;
						for( size_t j=i + numUnknownGlyphs; --j>i; )
						{
							if( nextUnknownCluster >= glyphInfo[j].cluster )
							{
								//j can never be 0 because --j is executed first,
								//and if i == 0, then 0 > 0 will exit the loop
								i = j - 1u;
								break;
							}
						}
					}
					else
					{
						const size_t nextUnknownCluster = firstCluster + clusterLength - 1u -
														  replacedCodepoints;
						for( size_t j=i; j<i + numUnknownGlyphs; ++j )
						{
							if( nextUnknownCluster <= glyphInfo[j].cluster )
							{
								i = j + 1u;
								break;
							}
						}
					}
				}
			}

			if( i < glyphCount && glyphInfo[i].codepoint != 0 )
			{
				const CachedGlyph *glyph = m_shaperManager->acquireGlyph( m_ftFont,
																		  glyphInfo[i].codepoint,
																		  m_ptSize.value26d6 );

				size_t cluster = glyphInfo[i].cluster;

				ShapedGlyph shapedGlyph;
				shapedGlyph.advance = Ogre::Vector2( glyphPos[i].x_advance,
													 -glyphPos[i].y_advance ) / 64.0f;
				shapedGlyph.offset = Ogre::Vector2( glyphPos[i].x_offset,
													-glyphPos[i].y_offset ) / 64.0f;
				shapedGlyph.caretPos = Ogre::Vector2::ZERO;
				shapedGlyph.clusterStart = glyphInfo[i].cluster + clusterOffset;
				if( dir != HB_DIRECTION_RTL )
				{
					if( i+1u < glyphCount )
						shapedGlyph.clusterLength = glyphInfo[i+1u].cluster - glyphInfo[i].cluster;
					else
						shapedGlyph.clusterLength = stringLength - glyphInfo[i].cluster;
				}
				else
				{
					if( i > 0 )
						shapedGlyph.clusterLength = glyphInfo[i-1u].cluster - glyphInfo[i].cluster;
					else
						shapedGlyph.clusterLength = stringLength - glyphInfo[i].cluster;
				}
				shapedGlyph.isNewline = utf16Str[cluster] == L'\n';
				shapedGlyph.isWordBreaker = utf16Str[cluster] == L' '	||
											utf16Str[cluster] == L'\t'	||
											utf16Str[cluster] == L'.'	||
											utf16Str[cluster] == L';'	||
											utf16Str[cluster] == L',';

				//Ensure whitespace has zero offset, otherwise it messes up Right & Bottom alignment
				if( utf16Str[cluster] == L' ' || utf16Str[cluster] == L'\t' )
					shapedGlyph.offset = Ogre::Vector2::ZERO;

				{
					//Ask ICU if this character correspond to a language we can
					//break by single letters (e.g. CJK characters, Thai)
					//Word breaking is actually very complex, so we just assume
					//these languages can be broken at any character
					uint32_t utf32Char;
					U16_GET_UNSAFE( utf16Str, cluster, utf32Char );
					UErrorCode ignoredError = U_ZERO_ERROR;
					UScriptCode scriptCode = uscript_getScript( utf32Char, &ignoredError );
					shapedGlyph.isWordBreaker |= uscript_breaksBetweenLetters( scriptCode ) != 0;
				}
				shapedGlyph.isTab = utf16Str[cluster] == L'\t';
				shapedGlyph.isRtl = dir == HB_DIRECTION_RTL;
				shapedGlyph.richTextIdx = richTextIdx;
				shapedGlyph.glyph = glyph;
				shapesVec.push_back( shapedGlyph );
			}
		}

		shapesVec.swap( outShapes );

		return numWrittenCodepoints;
	}
	//-------------------------------------------------------------------------
	bool Shaper::operator < ( const Shaper &other ) const
	{
		return this->m_script < other.m_script;
	}
}
