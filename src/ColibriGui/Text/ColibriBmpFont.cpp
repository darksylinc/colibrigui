
#include "ColibriGui/Text/ColibriBmpFont.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/Text/ColibriShaperManager.h"

#include "OgreLwString.h"

#include "unicode/schriter.h"
#include "unicode/unistr.h"

#include "sds/sds_fstream.h"
#include "sds/sds_fstreamApk.h"
#include "sds/sds_string.h"

namespace Colibri
{
	/// Outputs the next newline into outString, starting from data[idx]
	inline size_t getNextNewLine( std::vector<char> data, size_t idx, std::string &outString )
	{
		outString.clear();

		const size_t numData = data.size();

		for( size_t i = idx; i < numData; ++i )
		{
			if( data[i] == '\n' || data[i] == '\0' )
			{
				outString.reserve( i - idx );
				for( size_t j = idx; j < i; ++j )
					outString.push_back( data[j] );
				return idx + 1u;
			}
		}

		// If we're here we hit EOL, thus treat it as the last newline
		for( size_t j = idx; j < numData; ++j )
			outString.push_back( data[j] );
		return idx + 1u;
	}
	//-------------------------------------------------------------------------
	BmpFont::BmpFont( const char *fontLocation, ShaperManager *shaperManager ) :
		m_fontIdx(
			std::max<uint16_t>( static_cast<uint16_t>( shaperManager->getShapers().size() ), 1u ) )
	{
		// Open FNT file
		sds::PackageFstream fntFile( fontLocation, sds::fstream::InputEnd );

		const size_t fileSize = fntFile.getFileSize( false );
		fntFile.seek( 0, sds::fstream::beg );

		std::vector<char> fntData;
		fntData.resize( fileSize + 1u );
		fntFile.read( &( *fntData.begin() ), fileSize );
		fntData[fileSize] = '\0';  // Ensure string is null-terminated

		parseFntFile( fntData );
	}
	//-------------------------------------------------------------------------
	BmpFont::~BmpFont() {}
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

			if( newline.find( "char " ) != std::string::npos )
			{
				std::vector<std::string> values = sds::stringSplit( newline, ' ' );
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

				if( !m_chars.empty() && m_chars.back().id > bmpChar.id )
					bIsSorted = false;
				m_chars.push_back( bmpChar );
			}
		}

		if( !bIsSorted )
			std::sort( m_chars.begin(), m_chars.end() );
	}
	//-------------------------------------------------------------------------
	struct OrderByCodepoint
	{
		bool operator()( uint32_t a, const BmpChar &b ) const { return a < b.id; }
		bool operator()( const BmpChar &a, uint32_t b ) const { return a.id < b; }
	};

	void BmpFont::renderString( const std::string &utf8Str, BmpGlyphVec &outShapes )
	{
		icu::UnicodeString uStr( icu::UnicodeString::fromUTF8( utf8Str ) );

		BmpGlyphVec localShapes;
		localShapes.swap( outShapes );
		localShapes.clear();

		icu::StringCharacterIterator itor( uStr );

		while( itor.hasNext() )
		{
			const uint32_t codepoint = static_cast<uint32_t>( itor.next32() );

			std::vector<BmpChar>::const_iterator itBmp =
				std::lower_bound( m_chars.begin(), m_chars.end(), codepoint, OrderByCodepoint() );

			if( itBmp != m_chars.end() && itBmp->id == codepoint )
			{
				BmpGlyph bmpGlyph;
				bmpGlyph.topLeft = Ogre::Vector2::ZERO;
				bmpGlyph.isNewline = codepoint == U'\n';
				bmpGlyph.isTab = codepoint == U'\t';
				localShapes.push_back( bmpGlyph );
			}
		}

		localShapes.swap( outShapes );
	}
}  // namespace Colibri
