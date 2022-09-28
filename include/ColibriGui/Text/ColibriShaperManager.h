
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "OgrePrerequisites.h"

#include <vector>
#include <map>
#include <string>

COLIBRI_ASSUME_NONNULL_BEGIN

typedef struct FT_LibraryRec_  *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;
typedef int  FT_Error;

typedef struct UBiDi UBiDi;
typedef uint8_t UBiDiLevel;

namespace Ogre
{
	class BufferPacked;
	class HlmsColibri;
	class VaoManager;
}

#ifndef OGRE_MAKE_VERSION
#	define OGRE_MAKE_VERSION( maj, min, patch ) ( ( maj << 16 ) | ( min << 8 ) | patch )
#endif

namespace Colibri
{
	struct CachedGlyph
	{
		uint32_t codepoint;
		uint32_t ptSize;
		uint32_t offsetStart;
		float bearingX;
		float bearingY;
		uint16_t width;
		uint16_t height;
		float newlineSize;
		float regionUp;
		uint16_t font;
		uint32_t refCount;

		size_t getSizeBytes() const;

		bool isCodepointInPrivateArea() const;

		/*bool operator < ( const CachedGlyph &other ) const;
		friend bool operator < ( const CachedGlyph &a, const uint64_t &codePointSize );
		friend bool operator < ( const uint64_t &codePointSize, const CachedGlyph &b );*/
	};

	typedef std::vector<ShapedGlyph> ShapedGlyphVec;

	class ShaperManager
	{
	public:
		typedef std::vector<Shaper *>  ShaperVec;
		typedef std::vector<BmpFont *> BmpFontVec;

	protected:
		struct Range
		{
			size_t	offset;
			size_t	size;
		};
		struct GlyphKey
		{
			uint32_t codepoint;
			uint32_t ptSize;
			uint32_t fontIdx;
			GlyphKey( uint32_t _codepoint, uint32_t _ptSize, uint32_t _fontIdx ) :
				codepoint( _codepoint ), ptSize( _ptSize ), fontIdx( _fontIdx )
			{
				COLIBRI_ASSERT_MEDIUM( fontIdx != 0 );
			}

			bool operator < ( const GlyphKey &other ) const
			{
				if( this->codepoint != other.codepoint )
					return this->codepoint < other.codepoint;

				if( this->ptSize != other.ptSize )
					return this->ptSize < other.ptSize;

				if( this->fontIdx != other.fontIdx )
					return this->fontIdx < other.fontIdx;

				return false;
			}
		};

		FT_Library	m_ftLibrary;
		ColibriManager	*m_colibriManager;

		typedef std::map<GlyphKey, CachedGlyph> CachedGlyphMap;
		CachedGlyphMap	m_glyphCache;

		typedef std::vector<Range> RangeVec;

		uint8_t		*m_glyphAtlas;
		RangeVec	m_freeRanges; //NOT sorted
		size_t		m_offsetPtr;
		size_t		m_atlasCapacity;
		RangeVec	m_dirtyRanges; //NOT sorted?

		VertReadingDir::VertReadingDir m_preferredVertReadingDir;

		UBiDi		*m_bidi;
		UBiDiLevel	m_defaultDirection;
		bool		m_useVerticalLayoutWhenAvailable;

		/// m_shapers[0] is the default and not a strong reference
		ShaperVec  m_shapers;
		/// Unlike m_shapers, m_bmpFonts[0] is not repeated and is a strong ref
		BmpFontVec m_bmpFonts;

		uint16_t m_defaultBmpFontForRaster;

		Ogre::BufferPacked *colibri_nullable m_glyphAtlasBuffer;
		Ogre::HlmsColibri *colibri_nullable  m_hlms;
		Ogre::VaoManager *colibri_nullable   m_vaoManager;

		void growAtlas( size_t sizeBytes );
		size_t getAtlasOffset( size_t sizeBytes );
		CachedGlyph *createGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize, uint16_t fontIdx,
								  bool bDummy );
		/// Used only for private areas
		CachedGlyph *createRasterGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize,
										uint16_t fontIdx );
		void         destroyGlyph( CachedGlyphMap::iterator glyphIt );
		void mergeContiguousBlocks( RangeVec::iterator blockToMerge, RangeVec &blocks );

	public:
		ShaperManager( ColibriManager *colibriManager );
		~ShaperManager();

		void setOgre( Ogre::HlmsColibri * colibri_nullable hlms,
					  Ogre::VaoManager * colibri_nullable vaoManager );

		Shaper* addShaper( uint32_t /*hb_script_t*/ script, const char *fontPath,
						   const std::string &language );
		void setDefaultShaper( uint16_t font, HorizReadingDir::HorizReadingDir horizReadingDir,
							   bool useVerticalLayoutWhenAvailable );

		/// @remark m_shapers[0] contains the default font, which
		///			means it will appear twice in the array
		const ShaperVec& getShapers() const			{ return m_shapers; }

		void addBmpFont( const char *fontPath, bool bBilinearFilter = true );

		BmpFont *getBmpFont( size_t idx ) { return m_bmpFonts[idx]; }

		/// Sets a BmpFont when Label wants to use a character in the Private Use Area (Plane 0)
		void setDefaultBmpFontForRaster( uint16_t font );

		/// Returns the value set by setDefaultBmpFontForRaster
		uint16_t getDefaultBmpFontForRasterIdx() const;

		/// Returns the font as a pointer set by setDefaultBmpFontForRaster
		/// Can be nullptr if none is set
		const BmpFont *colibri_nullable getDefaultBmpFontForRaster() const;

		FT_Library getFreeTypeLibrary() const		{ return m_ftLibrary; }
		LogListener* getLogListener() const;

		/** Looks up in our cache to see if we already have a glyph for the given codepoint and ptSize
			If cache hits, we increase the reference count and return the glyph.
			If cache misses, we rasterize the glyph and add it to the cache; which may enlarge the
			atlas if needed (i.e. cause allocations)
		@remarks
			The cache does not distinguish fonts. If you call acquireGlyph with the same
			codepoint and ptSize but different font, the first call wins to the cache.
		@param font
			Font to use for rasterizing, in case the glyph wasn't in the cache already
		@param codepoint
			Codepoint (UTF-32)
		@param ptSize
			Size in points.
		@param bDummy
			When true, we will use codepoint 0's glyph data
			Useful when Label needs to rely on LabelBmp to draw glyphs from
			private use area.
		@return
			Cached glyph
		*/
		const CachedGlyph *acquireGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize,
										 uint16_t fontIdx, bool bDummy );
		/// WARNING: const_casts cachedGlyph, which means it's not thread safe
		void addRefCount( const CachedGlyph *cachedGlyph );
		/** Decreases the reference count of a glyph, for when it's not needed anymore
		@remarks
			The glyph isn't immediately freed, but rather kept around in case it's needed again.
			However if we run out of space while acquiring new uncached glyphs, we will reclaim
			this memory.
		@param codepoint
		@param ptSize
		*/
		void releaseGlyph( uint32_t codepoint, uint32_t ptSize, uint16_t fontIdx );
		/// This version is faster
		/// WARNING: const_casts cachedGlyph, which means it's not thread safe
		void releaseGlyph( const CachedGlyph *cachedGlyph );

		void flushReleasedGlyphs();

		/**
		@brief renderString
		@param utf8Str
		@param richText
		@param richTextIdx
		@param vertReadingDir
		@param outShapes
		@param bOutHasPrivateUse
			If true, there are glyph in outShapes we inserted that
			are in Unicode's private use.
			See Label.
		@return
			If string is fully LTR, returns Left
			If string is fully RTL, returns Right
			If string is mixed, it returns Mixed
			If string is empty or couldn't be analyzed, it returns Mixed
		*/
		TextHorizAlignment::TextHorizAlignment renderString(
			const char *utf8Str, const RichText &richText, uint32_t richTextIdx,
			VertReadingDir::VertReadingDir vertReadingDir, ShapedGlyphVec &outShapes,
			bool &bOutHasPrivateUse );

		TextHorizAlignment::TextHorizAlignment getDefaultTextDirection() const;
		VertReadingDir::VertReadingDir getPreferredVertReadingDir() const;

		void updateGpuBuffers();

		void prepareToRender();

		Ogre::HlmsColibri *colibri_nullable getOgreHlms() { return m_hlms; }
		Ogre::VaoManager *colibri_nullable getOgreVaoManager() { return m_vaoManager; }

		static const char* getErrorMessage( FT_Error errorCode );
	};
}

COLIBRI_ASSUME_NONNULL_END
