
#include "ColibriGui/Text/ColibriBmpFont.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/Text/ColibriShaperManager.h"

#include "ColibriGui/Ogre/OgreHlmsColibri.h"
#include "ColibriGui/Ogre/OgreHlmsColibriDatablock.h"

#include "OgreLwString.h"
#include "OgreResourceGroupManager.h"
#include "OgreTextureGpuManager.h"

#include "unicode/schriter.h"
#include "unicode/unistr.h"

#include "sds/sds_fstream.h"
#include "sds/sds_fstreamApk.h"
#include "sds/sds_string.h"

namespace Colibri
{
	/// Outputs the next newline into outString, starting from data[idx]
	inline size_t getNextNewLine( const std::vector<char> &data, size_t idx, std::string &outString )
	{
		outString.clear();

		const size_t numData = data.size();

		for( size_t i = idx; i < numData; ++i )
		{
			if( data[i] == '\n' || data[i] == '\0' )
			{
				for( size_t j = idx; j < i; ++j )
					outString.push_back( data[j] );
				return i + 1u;
			}
		}

		// If we're here we hit EOL, thus treat it as the last newline
		for( size_t j = idx; j < numData; ++j )
			outString.push_back( data[j] );
		return numData;
	}
	//-------------------------------------------------------------------------
	BmpFont::BmpFont( const char *fontLocation, ShaperManager *shaperManager ) :
		m_fontTexture( 0 ),
		m_datablock( 0 ),
		m_fontIdx(
			std::max<uint16_t>( static_cast<uint16_t>( shaperManager->getShapers().size() ), 1u ) ),
		m_bilinearFilter( true )
	{
		memset( &m_emptyChar, 0, sizeof( m_emptyChar ) );

		// Open FNT file
		sds::PackageFstream fntFile( fontLocation, sds::fstream::InputEnd );

		if( fntFile.is_open() )
		{
			const size_t fileSize = fntFile.getFileSize( false );
			fntFile.seek( 0, sds::fstream::beg );

			std::vector<char> fntData;
			fntData.resize( fileSize + 1u );
			fntFile.read( &( *fntData.begin() ), fileSize );
			fntData[fileSize] = '\0';  // Ensure string is null-terminated

			parseFntFile( fntData );
		}
		else
		{
			LogListener *log = shaperManager->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg(
				Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

			errorMsg.clear();
			errorMsg.a( "Could not open BmpFont ", fontLocation );
			log->log( errorMsg.c_str(), LogSeverity::Fatal );
		}
	}
	//-------------------------------------------------------------------------
	BmpFont::~BmpFont()
	{
		if( m_datablock )
		{
			m_datablock->getCreator()->destroyDatablock( m_datablock->getName() );
			m_datablock = 0;
		}

		if( m_fontTexture )
		{
			Ogre::TextureGpuManager *textureManager = m_fontTexture->getTextureManager();
			textureManager->destroyTexture( m_fontTexture );
			m_fontTexture = 0;
		}
	}
	//-------------------------------------------------------------------------
	void BmpFont::parseFntFile( std::vector<char> &inFntData )
	{
		std::vector<char> fntData;
		fntData.swap( inFntData );

		const size_t fileSize = fntData.size();

		std::string newline;
		size_t currPos = 0u;

		bool bIsSorted = true;

		while( currPos < fileSize )
		{
			currPos = getNextNewLine( fntData, currPos, newline );

			if( newline.find( "file=" ) != std::string::npos )
			{
				const std::vector<std::string> values = sds::stringSplit( newline, ' ' );
				std::map<std::string, std::string> settings = sds::stringMap( values, '=' );
				m_textureName = settings["file"];

				if( m_textureName.size() >= 2u )
				{
					// Remove quotations around filename
					if( m_textureName.back() == '"' )
						m_textureName.pop_back();
					if( m_textureName.front() == '"' )
						m_textureName.erase( m_textureName.begin() );
				}
			}
			if( newline.find( "base=" ) != std::string::npos )
			{
				const std::vector<std::string> values = sds::stringSplit( newline, ' ' );
				std::map<std::string, std::string> settings = sds::stringMap( values, '=' );

				m_fontSize = sds::toU32withDefault( settings["base"] ) << 6u;
			}
			else if( newline.find( "char " ) != std::string::npos )
			{
				const std::vector<std::string> values = sds::stringSplit( newline, ' ' );
				std::map<std::string, std::string> settings = sds::stringMap( values, '=' );

				BmpChar bmpChar;
				bmpChar.id = sds::toU32withDefault( settings["id"] );
				bmpChar.x = sds::toU16withDefault( settings["x"] );
				bmpChar.y = sds::toU16withDefault( settings["y"] );
				bmpChar.width = sds::toU16withDefault( settings["width"] );
				bmpChar.height = sds::toU16withDefault( settings["height"] );
				bmpChar.xoffset = sds::toU16withDefault( settings["xoffset"] );
				bmpChar.yoffset = sds::toU16withDefault( settings["yoffset"] );
				bmpChar.xadvance = sds::toU16withDefault( settings["xadvance"] );

				// Colibri extensions
				// fontScale =  scales the base font size (up or down) in case the atlas has different
				//				resolutions but you don't want to respect that resolution.
				// TODO: This is a float, not an uint
				bmpChar.fontScale = sds::toU16withDefault( settings["fontScale"], 1u );

				if( !m_chars.empty() && m_chars.back().id > bmpChar.id )
					bIsSorted = false;
				m_chars.push_back( bmpChar );
			}
		}

		if( !bIsSorted )
			std::sort( m_chars.begin(), m_chars.end() );

		if( !m_chars.empty() )
		{
			m_emptyChar.height = m_chars.back().yoffset + m_chars.back().height;
			m_emptyChar.xadvance = m_chars.back().xadvance;
		}
	}
	//-------------------------------------------------------------------------
	void BmpFont::setBilinearFilter( bool bBilinearFilter )
	{
		if( m_bilinearFilter == bBilinearFilter )
			return;

		m_bilinearFilter = bBilinearFilter;

		if( m_datablock )
		{
			COLIBRI_ASSERT_HIGH( dynamic_cast<Ogre::HlmsColibriDatablock *>( m_datablock ) );
			Ogre::HlmsColibriDatablock *datablock =
				static_cast<Ogre::HlmsColibriDatablock *>( m_datablock );
			Ogre::HlmsSamplerblock samplerblock;
			if( !m_bilinearFilter )
				samplerblock.setFiltering( Ogre::TFO_NONE );
			datablock->setSamplerblock( 0u, samplerblock );
		}
	}
	//-------------------------------------------------------------------------
	void BmpFont::setOgre( Ogre::HlmsColibri *hlms, Ogre::TextureGpuManager *textureManager )
	{
		m_fontTexture = textureManager->createOrRetrieveTexture(
			m_textureName, Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::Diffuse,
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

		Ogre::HlmsMacroblock macroblock;
		Ogre::HlmsBlendblock blendblock;

		macroblock.mDepthCheck = false;
		macroblock.mDepthWrite = false;
		blendblock.setBlendType( Ogre::SBT_TRANSPARENT_ALPHA );

		const std::string datablockName = "BmpFont/" + m_textureName;
		m_datablock = hlms->createDatablock( datablockName, datablockName, macroblock, blendblock,
											 Ogre::HlmsParamVec(), false );
		COLIBRI_ASSERT_HIGH( dynamic_cast<Ogre::HlmsColibriDatablock *>( m_datablock ) );
		Ogre::HlmsColibriDatablock *datablock = static_cast<Ogre::HlmsColibriDatablock *>( m_datablock );
		if( !m_bilinearFilter )
		{
			Ogre::HlmsSamplerblock samplerblock;
			samplerblock.setFiltering( Ogre::TFO_NONE );
			datablock->setSamplerblock( 0u, samplerblock );
		}
		datablock->setTexture( 0u, m_fontTexture );
	}
	//-------------------------------------------------------------------------
	void BmpFont::addFontAlignment( uint16_t fontIdx, const Ogre::Vector2 &offset )
	{
		if( fontIdx >= m_bmpToFontAlignments.size() )
			m_bmpToFontAlignments.resize( fontIdx + 1u, { 0.0f, 0.0f } );
		m_bmpToFontAlignments[fontIdx] = { (float)offset.x * m_fontSize.asFloat(),
										   (float)offset.y * m_fontSize.asFloat() };
	}
	//-------------------------------------------------------------------------
	BmpToFontAlignment BmpFont::getFontAlignment( size_t fontIdx ) const
	{
		if( fontIdx >= m_bmpToFontAlignments.size() )
			return { 0.0f, 0.0f };
		return m_bmpToFontAlignments[fontIdx];
	}
	//-------------------------------------------------------------------------
	struct OrderByCodepoint
	{
		bool operator()( uint32_t a, const BmpChar &b ) const { return a < b.id; }
		bool operator()( const BmpChar &a, uint32_t b ) const { return a.id < b; }
	};

	void BmpFont::renderString( const std::string &utf8Str, BmpGlyphVec &outShapes ) const
	{
		icu::UnicodeString uStr( icu::UnicodeString::fromUTF8( utf8Str ) );

		BmpGlyphVec localShapes;
		localShapes.swap( outShapes );
		localShapes.clear();

		icu::StringCharacterIterator itor( uStr );

		for( uint32_t codepoint = static_cast<uint32_t>( itor.first32() );  //
			 itor.hasNext();                                                //
			 codepoint = static_cast<uint32_t>( itor.next32() ) )
		{
			std::vector<BmpChar>::const_iterator itBmp =
				std::lower_bound( m_chars.begin(), m_chars.end(), codepoint, OrderByCodepoint() );

			BmpGlyph bmpGlyph;
			bmpGlyph.isNewline = codepoint == U'\n';
			bmpGlyph.isTab = codepoint == U'\t';
			if( itBmp != m_chars.end() && itBmp->id == codepoint )
				bmpGlyph.bmpChar = &( *itBmp );
			else
				bmpGlyph.bmpChar = &m_emptyChar;
			bmpGlyph.xoffset = bmpGlyph.bmpChar->xoffset;
			bmpGlyph.yoffset = bmpGlyph.bmpChar->yoffset;
			bmpGlyph.width = bmpGlyph.bmpChar->width;
			bmpGlyph.height = bmpGlyph.bmpChar->height;
			localShapes.push_back( bmpGlyph );
		}

		localShapes.swap( outShapes );
	}
	//-------------------------------------------------------------------------
	BmpGlyph BmpFont::renderCodepoint( const uint32_t codepoint ) const
	{
		std::vector<BmpChar>::const_iterator itBmp =
			std::lower_bound( m_chars.begin(), m_chars.end(), codepoint, OrderByCodepoint() );

		BmpGlyph bmpGlyph;
		bmpGlyph.isNewline = codepoint == U'\n';
		bmpGlyph.isTab = codepoint == U'\t';
		if( itBmp != m_chars.end() && itBmp->id == codepoint )
			bmpGlyph.bmpChar = &( *itBmp );
		else
			bmpGlyph.bmpChar = &m_emptyChar;
		bmpGlyph.xoffset = bmpGlyph.bmpChar->xoffset;
		bmpGlyph.yoffset = bmpGlyph.bmpChar->yoffset;
		bmpGlyph.width = bmpGlyph.bmpChar->width;
		bmpGlyph.height = bmpGlyph.bmpChar->height;

		return bmpGlyph;
	}
	//-------------------------------------------------------------------------
	void BmpFont::renderCodepoint( const uint32_t codepoint, BmpGlyphVec &outShapes ) const
	{
		outShapes.push_back( renderCodepoint( codepoint ) );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector4 BmpFont::getInvResolution() const
	{
		if( m_fontTexture->isDataReady() )
		{
			return 1.0f / Ogre::Vector4( static_cast<float>( m_fontTexture->getWidth() ),
										 static_cast<float>( m_fontTexture->getHeight() ),
										 static_cast<float>( m_fontTexture->getWidth() ),
										 static_cast<float>( m_fontTexture->getHeight() ) );
		}

		return Ogre::Vector4::ZERO;
	}
	//-------------------------------------------------------------------------
	float BmpFont::getFontScale( const ShaperManager *shaperManager ) const
	{
		return static_cast<float>( shaperManager->getDPI() ) / ( m_fontSize.asFloat() * 96.0f );
	}
}  // namespace Colibri
