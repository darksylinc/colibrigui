
#include "ColibriGui/Text/ColibriShaperManager.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/Text/ColibriBmpFont.h"
#include "ColibriGui/Text/ColibriShaper.h"

#include "ColibriGui/Ogre/OgreHlmsColibri.h"

#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
#	include "Vao/OgreReadOnlyBufferPacked.h"
#endif
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

#include "OgreLwString.h"

#include "ft2build.h"

#include "freetype/freetype.h"

#include "unicode/ubidi.h"
#include "unicode/unistr.h"

namespace Colibri
{
	ShaperManager::ShaperManager( ColibriManager *colibriManager ) :
		m_ftLibrary( 0 ),
		m_colibriManager( colibriManager ),
		m_glyphAtlas( 0 ),
		m_offsetPtr( 1 ),  // The 1st byte is taken. See ShaperManager::updateGpuBuffers
		m_atlasCapacity( 0 ),
		m_preferredVertReadingDir( VertReadingDir::Disabled ),
		m_bidi( 0 ),
		m_defaultDirection( UBIDI_DEFAULT_LTR /*Note: non-defaults like UBIDI_RTL work differently!*/ ),
		m_useVerticalLayoutWhenAvailable( false ),
		m_defaultBmpFontForRaster( std::numeric_limits<uint16_t>::max() ),
		m_dpi( 96u ),
		m_glyphAtlasBuffer( 0 ),
		m_hlms( 0 ),
		m_vaoManager( 0 )
	{
		FT_Error errorCode = FT_Init_FreeType( &m_ftLibrary );
		if( errorCode )
		{
			LogListener *log = this->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[Freetype2 error] Could not initialize Freetype errorCode: ",
						errorCode, " Desc: ", ShaperManager::getErrorMessage( errorCode ) );
			log->log( errorMsg.c_str(), LogSeverity::Fatal );
		}

		m_bidi = ubidi_open();
		ubidi_orderParagraphsLTR( m_bidi, 1 );
	}
	//-------------------------------------------------------------------------
	ShaperManager::~ShaperManager()
	{
		if( !m_shapers.empty() )
		{
			ShaperVec::const_iterator itor = m_shapers.begin() + 1u;
			ShaperVec::const_iterator end  = m_shapers.end();

			while( itor != end )
				delete *itor++;

			m_shapers.clear();
		}

		{
			BmpFontVec::const_iterator itor = m_bmpFonts.begin();
			BmpFontVec::const_iterator endt = m_bmpFonts.end();

			while( itor != endt )
				delete *itor++;
			m_bmpFonts.clear();
		}

		if( m_glyphAtlas )
		{
			free( m_glyphAtlas );
			m_glyphAtlas = 0;
		}

		setOgre( 0, 0 );

		ubidi_close( m_bidi );
		m_bidi = 0;

		FT_Done_FreeType( m_ftLibrary );
		m_ftLibrary = 0;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::setOgre( Ogre::HlmsColibri * colibri_nullable hlms,
								 Ogre::VaoManager * colibri_nullable vaoManager )
	{
		if( m_hlms )
			m_hlms->setGlyphAtlasBuffer( 0 );
		if( m_vaoManager && m_glyphAtlasBuffer )
		{
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
			if( m_glyphAtlasBuffer->getBufferPackedType() != Ogre::BP_TYPE_TEX )
			{
				m_vaoManager->destroyReadOnlyBuffer(
					static_cast<Ogre::ReadOnlyBufferPacked *>( m_glyphAtlasBuffer ) );
			}
			else
#endif
			{
				m_vaoManager->destroyTexBuffer(
					static_cast<Ogre::TexBufferPacked *>( m_glyphAtlasBuffer ) );
			}
			m_glyphAtlasBuffer = 0;
		}

		m_hlms = hlms;
		m_vaoManager = vaoManager;

		if( hlms )
		{
			Ogre::TextureGpuManager *textureManager = hlms->getRenderSystem()->getTextureGpuManager();
			BmpFontVec::const_iterator itor = m_bmpFonts.begin();
			BmpFontVec::const_iterator endt = m_bmpFonts.end();

			while( itor != endt )
			{
				( *itor )->setOgre( hlms, textureManager );
				++itor;
			}
		}
	}
	//-------------------------------------------------------------------------
	void ShaperManager::setDPI( uint32_t dpi )
	{
		LogListener *log = getLogListener();
		if( dpi == 0 )
		{
			log->log( "Invalid DPI value. Using a default value.", LogSeverity::Error );
			dpi = 96u;
		}
		m_dpi = dpi;

		char tmpBuffer[128];
		Ogre::LwString msg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
		msg.a( "setDPI = ", dpi );
		log->log( msg.c_str(), LogSeverity::Info );
	}
	//-------------------------------------------------------------------------
	Shaper* ShaperManager::addShaper( uint32_t /*hb_script_t*/ script, const char *fontPath,
									  const std::string &language )
	{
		Shaper *shaper = new Shaper( static_cast<hb_script_t>( script ), fontPath, language, this );
		if( m_shapers.empty() )
			m_shapers.push_back( shaper );

		m_shapers.push_back( shaper );

		return shaper;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::setDefaultShaper( uint16_t font,
										  HorizReadingDir::HorizReadingDir horizReadingDir,
										  bool useVerticalLayoutWhenAvailable )
	{
		COLIBRI_ASSERT_LOW( font < m_shapers.size() );

		m_shapers[0] = m_shapers[font];
		switch( horizReadingDir )
		{
		case HorizReadingDir::Default:	m_defaultDirection = UBIDI_DEFAULT_LTR;	break;
		case HorizReadingDir::AutoLTR:	m_defaultDirection = UBIDI_DEFAULT_LTR;	break;
		case HorizReadingDir::AutoRTL:	m_defaultDirection = UBIDI_DEFAULT_RTL;	break;
		case HorizReadingDir::LTR:		m_defaultDirection = UBIDI_LTR;			break;
		case HorizReadingDir::RTL:		m_defaultDirection = UBIDI_RTL;			break;
		}
		m_useVerticalLayoutWhenAvailable = useVerticalLayoutWhenAvailable;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::addBmpFont( const char *fontPath, bool bBilinearFilter )
	{
		BmpFont *bmpFont = new BmpFont( fontPath, this );
		bmpFont->setBilinearFilter( bBilinearFilter );
		m_bmpFonts.push_back( bmpFont );
	}
	//-------------------------------------------------------------------------
	void ShaperManager::setDefaultBmpFontForRaster( uint16_t font ) { m_defaultBmpFontForRaster = font; }
	//-------------------------------------------------------------------------
	uint16_t ShaperManager::getDefaultBmpFontForRasterIdx() const { return m_defaultBmpFontForRaster; }
	//-------------------------------------------------------------------------
	const BmpFont *ShaperManager::getDefaultBmpFontForRaster() const
	{
		if( m_defaultBmpFontForRaster < m_bmpFonts.size() )
			return m_bmpFonts[m_defaultBmpFontForRaster];
		return nullptr;
	}
	//-------------------------------------------------------------------------
	LogListener* ShaperManager::getLogListener() const
	{
		return m_colibriManager->getLogListener();
	}
	//-------------------------------------------------------------------------
	void ShaperManager::growAtlas( size_t sizeBytes )
	{
		m_atlasCapacity = std::max( m_offsetPtr + sizeBytes,
									m_atlasCapacity + (m_atlasCapacity >> 1u) + 1u );
		m_glyphAtlas = reinterpret_cast<uint8_t*>( realloc( m_glyphAtlas, m_atlasCapacity ) );
	}
	//-------------------------------------------------------------------------
	size_t ShaperManager::getAtlasOffset( size_t sizeBytes )
	{
		//Get smallest available free range
		RangeVec::iterator end = m_freeRanges.end();
		RangeVec::iterator bestRange = end;

		{
			RangeVec::iterator itor = m_freeRanges.begin();
			while( itor != end )
			{
				if( sizeBytes < itor->size && (bestRange == end || bestRange->size > itor->size) )
					bestRange = itor;
				++itor;
			}
		}

		size_t retVal = 0;

		if( bestRange != end )
		{
			retVal = bestRange->offset;
			bestRange->offset	+= sizeBytes;
			bestRange->size		-= sizeBytes;
			if( bestRange->size == 0 )
				Ogre::efficientVectorRemove( m_freeRanges, bestRange );
		}
		else
		{
			//Couldn't find free space in fragmented pool.
			if( m_offsetPtr + sizeBytes > m_atlasCapacity )
			{
				//We're out of space. First check if we can steal another slot.
				const CachedGlyphMap::iterator enGlyph = m_glyphCache.end();
				CachedGlyphMap::iterator bestUnusedGlyph = enGlyph;

				for( size_t i = 0; i < 2u && bestUnusedGlyph == enGlyph; ++i )
				{
					CachedGlyphMap::iterator itGlyph = m_glyphCache.begin();
					while( itGlyph != enGlyph )
					{
						if( !itGlyph->second.refCount && itGlyph->second.getSizeBytes() >= sizeBytes &&
							( bestUnusedGlyph == enGlyph ||
							  itGlyph->second.getSizeBytes() < bestUnusedGlyph->second.getSizeBytes() ) )
						{
							bestUnusedGlyph = itGlyph;
						}
						++itGlyph;
					}

					// Not found? Try again, this time with all unused glyphs removed and merged.
					// We may have two contiguous unused glyphs that are big enough to hold
					// this new glyph, but weren't big enough individually.
					if( i == 0 && bestUnusedGlyph == enGlyph )
						flushReleasedGlyphs();
				}

				if( bestUnusedGlyph == enGlyph )
				{
					// Cannot steal. Grow the atlas, advance the pointer and get a fresh region
					growAtlas( sizeBytes );
					retVal = m_offsetPtr;
					m_offsetPtr += sizeBytes;
				}
				else
				{
					//Steal successful! Put the unused glyph back into the pool and try again
					destroyGlyph( bestUnusedGlyph );
					retVal = getAtlasOffset( sizeBytes );
				}
			}
			else
			{
				//Advance the pointer and get a fresh region
				retVal = m_offsetPtr;
				m_offsetPtr += sizeBytes;
			}
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	CachedGlyph *ShaperManager::createGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize,
											 uint16_t fontIdx, bool bDummy )
	{
		FT_Error errorCode = FT_Load_Glyph( font, bDummy ? 0u : codepoint, FT_LOAD_DEFAULT );
		if( colibri_unlikely( errorCode ) )
		{
			LogListener *log = getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[Freetype2 error] Could not load glyph for codepoint ", codepoint,
						" errorCode: ", errorCode, " Desc: ",
						ShaperManager::getErrorMessage( errorCode ) );
			log->log( errorMsg.c_str(), LogSeverity::Warning );
		}

		//Rasterize the glyph
		FT_GlyphSlot slot = font->glyph;
		FT_Render_Glyph( slot, FT_RENDER_MODE_NORMAL );

		FT_Bitmap ftBitmap = slot->bitmap;

		//Create a cache entry
		CachedGlyph newGlyph;
		newGlyph.codepoint	= codepoint;
		newGlyph.ptSize		= ptSize;
		newGlyph.bearingX	= static_cast<float>( slot->bitmap_left );
		newGlyph.bearingY	= static_cast<float>( slot->bitmap_top );
		newGlyph.width		= static_cast<uint16_t>( ftBitmap.width );
		newGlyph.height		= static_cast<uint16_t>( ftBitmap.rows );
		newGlyph.offsetStart = (uint32_t)getAtlasOffset( newGlyph.getSizeBytes() );
		newGlyph.newlineSize = (float)font->size->metrics.height / 64.0f;
		newGlyph.regionUp = (float)font->size->metrics.ascender /
							float( font->size->metrics.ascender - font->size->metrics.descender );
		newGlyph.font = fontIdx;
		newGlyph.refCount	= 0;

		const GlyphKey glyphKey( codepoint, ptSize, fontIdx );
		std::pair<CachedGlyphMap::iterator, bool> pair =
				m_glyphCache.emplace( glyphKey, newGlyph );

		if( newGlyph.getSizeBytes() > 0 )
		{
			//Copy the rasterized results to our atlas
			memcpy( m_glyphAtlas + newGlyph.offsetStart, ftBitmap.buffer, newGlyph.getSizeBytes() );
			{
				//Schedule a transfer to the GPU.
				Range dirtyRange;
				dirtyRange.offset	= newGlyph.offsetStart;
				dirtyRange.size		= newGlyph.getSizeBytes();
				m_dirtyRanges.push_back( dirtyRange );
			}
		}

		return &pair.first->second;
	}
	//-------------------------------------------------------------------------
	CachedGlyph *ShaperManager::createRasterGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize,
												   uint16_t fontIdx )
	{
		// We need to know the bearing of most characters and hope it aligns well
		const CachedGlyph *dummyCodepoint = acquireGlyph( font, 0u, ptSize, fontIdx, false );

		const BmpFont *bmpFont = getDefaultBmpFontForRaster();
		COLIBRI_ASSERT_MEDIUM( bmpFont );

		const BmpGlyph bmpGlyph = bmpFont->renderCodepoint( codepoint );
		const BmpToFontAlignment fontAlignment = bmpFont->getFontAlignment( fontIdx );

		// fontAlignment.xoffset must NOT include bmpGlyph.bmpChar->fontScale, so we
		// don't include it in fontScale variable.
		const float fontScale = FontSize( ptSize ).asFloat() * bmpFont->getFontScale( this );

		//Create a cache entry
		CachedGlyph newGlyph;
		newGlyph.codepoint	= codepoint;
		newGlyph.ptSize		= ptSize;
		newGlyph.width =
			uint16_t( std::round( bmpGlyph.width * fontScale * bmpGlyph.bmpChar->fontScale ) );
		newGlyph.height =
			uint16_t( std::round( bmpGlyph.height * fontScale * bmpGlyph.bmpChar->fontScale ) );
		newGlyph.bearingX = ( fontAlignment.xoffset * fontScale ) / dummyCodepoint->height;
		newGlyph.bearingY = ( ( newGlyph.height * float( dummyCodepoint->bearingY ) ) +
							  ( fontAlignment.yoffset * fontScale ) ) /
							dummyCodepoint->height;
		newGlyph.offsetStart = 0u;
		newGlyph.newlineSize= newGlyph.height;
		newGlyph.regionUp	= 1.0f;  // Is this correct?
		newGlyph.font		= fontIdx;
		newGlyph.refCount	= 0;

		releaseGlyph( dummyCodepoint );

		const GlyphKey glyphKey( codepoint, ptSize, fontIdx );
		std::pair<CachedGlyphMap::iterator, bool> pair =
				m_glyphCache.emplace( glyphKey, newGlyph );

		return &pair.first->second;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::destroyGlyph( CachedGlyphMap::iterator glyphIt )
	{
		CachedGlyph &glyph = glyphIt->second;

		if( glyph.offsetStart + glyph.getSizeBytes() == m_offsetPtr )
		{
			//Easy case. LIFO.
			m_offsetPtr -= glyph.getSizeBytes();
		}
		else
		{
			Range freeRange;
			freeRange.offset= glyph.offsetStart;
			freeRange.size	= glyph.getSizeBytes();
			m_freeRanges.push_back( freeRange );
			mergeContiguousBlocks( m_freeRanges.end() - 1u, m_freeRanges );
		}

		m_glyphCache.erase( glyphIt );
	}
	//-------------------------------------------------------------------------
	void ShaperManager::mergeContiguousBlocks( RangeVec::iterator blockToMerge,
											   RangeVec &blocks )
	{
		RangeVec::iterator itor = blocks.begin();
		RangeVec::iterator end  = blocks.end();

		while( itor != end )
		{
			if( itor->offset + itor->size == blockToMerge->offset )
			{
				itor->size += blockToMerge->size;
				ptrdiff_t idx = itor - blocks.begin();

				//When blockToMerge is the last one, its index won't be the same
				//after removing the other iterator, they will swap.
				if( idx == ptrdiff_t( blocks.size() - 1u ) )
					idx = blockToMerge - blocks.begin();

				Ogre::efficientVectorRemove( blocks, blockToMerge );

				blockToMerge = blocks.begin() + idx;
				itor = blocks.begin();
				end  = blocks.end();
			}
			else if( blockToMerge->offset + blockToMerge->size == itor->offset )
			{
				blockToMerge->size += itor->size;
				ptrdiff_t idx = blockToMerge - blocks.begin();

				//When blockToMerge is the last one, its index won't be the same
				//after removing the other iterator, they will swap.
				if( idx == ptrdiff_t( blocks.size() - 1u ) )
					idx = itor - blocks.begin();

				Ogre::efficientVectorRemove( blocks, itor );

				blockToMerge = blocks.begin() + idx;
				itor = blocks.begin();
				end  = blocks.end();
			}
			else
			{
				++itor;
			}
		}
	}
	//-------------------------------------------------------------------------
	const CachedGlyph *ShaperManager::acquireGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize,
													uint16_t fontIdx, bool bDummy )
	{
		CachedGlyph *retVal = 0;

		const GlyphKey glyphKey( codepoint, ptSize, fontIdx );
		CachedGlyphMap::iterator itor = m_glyphCache.find( glyphKey );

		if( itor != m_glyphCache.end() )
		{
			retVal = &itor->second;
		}
		else
		{
			if( !bDummy || !getDefaultBmpFontForRaster() )
				retVal = createGlyph( font, codepoint, ptSize, fontIdx, bDummy );
			else
				retVal = createRasterGlyph( font, codepoint, ptSize, fontIdx );
		}

		++retVal->refCount;

		return retVal;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::addRefCount( const CachedGlyph *cachedGlyph )
	{
		const GlyphKey glyphKey( cachedGlyph->codepoint, cachedGlyph->ptSize, cachedGlyph->font );

		COLIBRI_ASSERT_MEDIUM( m_glyphCache.find( glyphKey ) != m_glyphCache.end() &&
							   "Invalid glyph cache entry. Use-after-free perhaps?" );

		CachedGlyph *nonConstCachedGlyph = const_cast<CachedGlyph*>( cachedGlyph );
		++nonConstCachedGlyph->refCount;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::releaseGlyph( uint32_t codepoint, uint32_t ptSize, uint16_t fontIdx )
	{
		const GlyphKey glyphKey( codepoint, ptSize, fontIdx );

		CachedGlyphMap::iterator itor = m_glyphCache.find( glyphKey );

		COLIBRI_ASSERT_LOW( itor != m_glyphCache.end() &&
							"Invalid glyph cache entry not found. Use-after-free perhaps?" );
		COLIBRI_ASSERT_LOW( itor->second.refCount > 0 );

		if( itor != m_glyphCache.end() && itor->second.refCount > 0 )
			--itor->second.refCount;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::releaseGlyph( const CachedGlyph *cachedGlyph )
	{
		const GlyphKey glyphKey( cachedGlyph->codepoint, cachedGlyph->ptSize, cachedGlyph->font );

		COLIBRI_ASSERT_MEDIUM( m_glyphCache.find( glyphKey ) != m_glyphCache.end() &&
							   "Invalid glyph cache entry. Use-after-free perhaps?" );
		COLIBRI_ASSERT_LOW( cachedGlyph->refCount > 0 );

		CachedGlyph *nonConstCachedGlyph = const_cast<CachedGlyph*>( cachedGlyph );
		if( nonConstCachedGlyph->refCount > 0 )
			--nonConstCachedGlyph->refCount;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::flushReleasedGlyphs()
	{
		CachedGlyphMap::iterator itor = m_glyphCache.begin();
		CachedGlyphMap::iterator end  = m_glyphCache.end();

		while( itor != end )
		{
			if( !itor->second.refCount )
			{
				CachedGlyphMap::iterator toDelete = itor++;
				destroyGlyph( toDelete );
			}
			else
			{
				++itor;
			}
		}
	}
	//-------------------------------------------------------------------------
	TextHorizAlignment::TextHorizAlignment ShaperManager::renderString(
			const char *utf8Str, const RichText &richText, uint32_t richTextIdx,
			VertReadingDir::VertReadingDir vertReadingDir,
			ShapedGlyphVec &outShapes, bool &bOutHasPrivateUse )
	{
		bOutHasPrivateUse = false;

		UBiDiDirection retVal = UBIDI_NEUTRAL;

		UnicodeString uStr( utf8Str, (int32_t)richText.length );

		UBiDiLevel textHorizDir = m_defaultDirection;

		switch( richText.readingDir )
		{
		case HorizReadingDir::Default:	textHorizDir = m_defaultDirection;	break;
		case HorizReadingDir::AutoLTR:	textHorizDir = UBIDI_DEFAULT_LTR;	break;
		case HorizReadingDir::AutoRTL:	textHorizDir = UBIDI_DEFAULT_RTL;	break;
		case HorizReadingDir::LTR:		textHorizDir = UBIDI_LTR;			break;
		case HorizReadingDir::RTL:		textHorizDir = UBIDI_RTL;			break;
		}

		UErrorCode errorCode = U_ZERO_ERROR;
		ubidi_setPara( m_bidi, uStr.getBuffer(), uStr.length(), textHorizDir, 0, &errorCode );

		if( colibri_unlikely( !U_SUCCESS(errorCode) ) )
		{
			LogListener *log = this->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[UBiDi error] Error analyzing text. Error code: ", errorCode,
						" Desc: ", u_errorName( errorCode ), "\n[UBiDi error] String:" );
			log->log( errorMsg.c_str(), LogSeverity::Warning );
			log->log( utf8Str, LogSeverity::Warning );
			return getDefaultTextDirection();
		}

		Shaper *shaper = 0;
		if( colibri_unlikely( richText.font >= m_shapers.size() ) )
		{
			LogListener *log = this->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[ShaperManager::renderString] RichText wants font ", richText.font,
						" but there's only ", (uint32_t)m_shapers.size(), " fonts installed" );
			log->log( errorMsg.c_str(), LogSeverity::Error );

			shaper = m_shapers[0];
		}
		else
			shaper = m_shapers[richText.font];

		UnicodeString uniStr( false, ubidi_getText( m_bidi ), ubidi_getLength( m_bidi ) );

		const int32_t numBlocks = ubidi_countRuns( m_bidi, &errorCode );
		for( int32_t i=0; i<numBlocks; ++i )
		{
			int32_t logicalStart, length;
			UBiDiDirection dir = ubidi_getVisualRun( m_bidi, i, &logicalStart, &length );

			UnicodeString temp = uniStr.tempSubString( logicalStart, length );

			hb_direction_t hbDir = dir == UBIDI_LTR ? HB_DIRECTION_LTR : HB_DIRECTION_RTL;

			if( (vertReadingDir == VertReadingDir::IfNeededTTB && m_useVerticalLayoutWhenAvailable) ||
				vertReadingDir == VertReadingDir::ForceTTB ||
				vertReadingDir == VertReadingDir::ForceTTBLTR )
			{
				hbDir = HB_DIRECTION_TTB;
			}

			if( retVal == UBIDI_NEUTRAL )
				retVal = dir;
			/*else if( retVal != dir )
				retVal = UBIDI_MIXED;*/

#if U_SIZEOF_WCHAR_T == 2
			const uint16_t *utf16Str = reinterpret_cast<const uint16_t*>( temp.getBuffer() );
#else
			const uint16_t *utf16Str = temp.getBuffer();
#endif
			shaper->setFontSize( richText.ptSize );
			shaper->renderString( utf16Str, (size_t)temp.length(), hbDir, richTextIdx,
								  (uint32_t)logicalStart, outShapes, bOutHasPrivateUse, true );
		}

		TextHorizAlignment::TextHorizAlignment finalRetVal;
		switch( retVal )
		{
		case UBIDI_LTR:
			finalRetVal = TextHorizAlignment::Left;		break;
		case UBIDI_RTL:
			finalRetVal = TextHorizAlignment::Right;	break;
		case UBIDI_MIXED:
		case UBIDI_NEUTRAL:
			finalRetVal = TextHorizAlignment::Mixed;
			break;
		}

		return finalRetVal;
	}
	//-------------------------------------------------------------------------
	TextHorizAlignment::TextHorizAlignment ShaperManager::getDefaultTextDirection() const
	{
		return (m_defaultDirection == UBIDI_DEFAULT_LTR || m_defaultDirection == UBIDI_LTR) ?
					TextHorizAlignment::Left : TextHorizAlignment::Right;
	}
	//-------------------------------------------------------------------------
	VertReadingDir::VertReadingDir ShaperManager::getPreferredVertReadingDir() const
	{
		return m_preferredVertReadingDir;
	}
	//-------------------------------------------------------------------------
	void ShaperManager::updateGpuBuffers()
	{
		if( (!m_glyphAtlasBuffer ||
			 m_atlasCapacity !=
			 m_glyphAtlasBuffer->getTotalSizeBytes()) &&
			m_atlasCapacity > 0u )
		{
			// Local buffer has changed (i.e. growAtlas was called). Realloc the GPU buffer.
			if( m_glyphAtlasBuffer )
			{
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
				if( m_glyphAtlasBuffer->getBufferPackedType() != Ogre::BP_TYPE_TEX )
				{
					m_vaoManager->destroyReadOnlyBuffer(
						static_cast<Ogre::ReadOnlyBufferPacked *>( m_glyphAtlasBuffer ) );
				}
				else
#endif
				{
					m_vaoManager->destroyTexBuffer(
						static_cast<Ogre::TexBufferPacked *>( m_glyphAtlasBuffer ) );
				}
			}

#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
			if( Ogre::HlmsColibri::needsReadOnlyBuffer( m_hlms->getRenderSystem()->getCapabilities(),
														m_vaoManager ) )
			{
				m_glyphAtlasBuffer = m_vaoManager->createReadOnlyBuffer(
					Ogre::PFG_R8_UNORM, m_atlasCapacity, Ogre::BT_DEFAULT, 0, false );
			}
			else
#endif
			{
				m_glyphAtlasBuffer = m_vaoManager->createTexBuffer( Ogre::PFG_R8_UNORM, m_atlasCapacity,
																	Ogre::BT_DEFAULT, 0, false );
			}
			m_hlms->setGlyphAtlasBuffer( m_glyphAtlasBuffer );

			// The 1st byte is taken. We use this byte to render arbitrary fixed-colour
			// stuff without having to switch shaders and would complicate rendering.
			// It's mostly used for the background colour by Label.
			m_glyphAtlas[0] = 0xff;

			m_glyphAtlasBuffer->upload( m_glyphAtlas, 0, m_offsetPtr );
			m_dirtyRanges.clear();
		}
		else
		{
			RangeVec::const_iterator itor = m_dirtyRanges.begin();
			RangeVec::const_iterator endt = m_dirtyRanges.end();

			while( itor != endt )
			{
				size_t offset = itor->offset;
				size_t size = itor->size;
#ifdef OGRE_VK_WORKAROUND_PVR_ALIGNMENT
				if( Ogre::Workarounds::mPowerVRAlignment && offset > 0u )
				{
					const size_t newOffset =
						Ogre::alignToPreviousMult( offset, Ogre::Workarounds::mPowerVRAlignment );
					size += offset - newOffset;
					offset = newOffset;
				}
#endif
				m_glyphAtlasBuffer->upload( m_glyphAtlas + offset, offset, size );
				++itor;
			}

			m_dirtyRanges.clear();
		}
	}
	//-------------------------------------------------------------------------
	void ShaperManager::prepareToRender() { m_hlms->setGlyphAtlasBuffer( m_glyphAtlasBuffer ); }
	//-------------------------------------------------------------------------
	const char* ShaperManager::getErrorMessage( FT_Error errorCode )
	{
		#undef __FTERRORS_H__
		#define FT_ERRORDEF( e, v, s )  case e: return s;
		#define FT_ERROR_START_LIST     switch( errorCode ) {
		#define FT_ERROR_END_LIST       }
		#include FT_ERRORS_H
		return "(Unknown error)";
	}
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	size_t CachedGlyph::getSizeBytes() const
	{
		if( isCodepointInPrivateArea() )
			return 0u;
		return this->width * this->height;
	}
	//-------------------------------------------------------------------------
	bool CachedGlyph::isCodepointInPrivateArea() const
	{
		return codepoint >= 0xE000 && codepoint <= 0xF8FF;
	}
	/*bool CachedGlyph::operator < ( const CachedGlyph &other ) const
	{
		const uint64_t thisKey = ((uint64_t)this->codepoint << 32ul) | ((uint64_t)this->ptSize);
		const uint64_t otherKey = ((uint64_t)other.codepoint << 32ul) | ((uint64_t)other.ptSize);
		return thisKey < otherKey;
	}
	bool operator < ( const CachedGlyph &a, const uint64_t &codePointSize )
	{
		const uint64_t aKey = ((uint64_t)a.codepoint << 32ul) | ((uint64_t)a.ptSize);
		return aKey < codePointSize;
	}
	bool operator < ( const uint64_t &codePointSize, const CachedGlyph &b )
	{
		const uint64_t bKey = ((uint64_t)b.codepoint << 32ul) | ((uint64_t)b.ptSize);
		return codePointSize < bKey;
	}*/
}
