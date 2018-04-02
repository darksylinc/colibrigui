
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"

#include <vector>
#include <map>
#include <string>

CRYSTALGUI_ASSUME_NONNULL_BEGIN

typedef struct FT_LibraryRec_  *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;
typedef int  FT_Error;

typedef struct UBiDi UBiDi;
typedef uint8_t UBiDiLevel;

namespace Ogre
{
	class HlmsCrystal;
	class TexBufferPacked;
	class VaoManager;
}

namespace Crystal
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
		uint32_t refCount;

		size_t getSizeBytes() const;

		/*bool operator < ( const CachedGlyph &other ) const;
		friend bool operator < ( const CachedGlyph &a, const uint64_t &codePointSize );
		friend bool operator < ( const uint64_t &codePointSize, const CachedGlyph &b );*/
	};

	typedef std::vector<ShapedGlyph> ShapedGlyphVec;

	class ShaperManager
	{
		struct Range
		{
			size_t	offset;
			size_t	size;
		};

		FT_Library	m_ftLibrary;
		CrystalManager	*m_crystalManager;

		typedef std::map<uint64_t, CachedGlyph> CachedGlyphMap;
		CachedGlyphMap	m_glyphCache;

		typedef std::vector<Range> RangeVec;

		uint8_t		*m_glyphAtlas;
		RangeVec	m_freeRanges; //NOT sorted
		size_t		m_offsetPtr;
		size_t		m_atlasCapacity;
		RangeVec	m_dirtyRanges; //NOT sorted?

		UBiDi		*m_bidi;
		UBiDiLevel	m_defaultDirection;
		bool		m_useVerticalLayoutWhenAvailable;

		typedef std::vector<Shaper*> ShaperVec;
		/// m_shapers[0] is the default and not a strong reference
		ShaperVec	m_shapers;

		Ogre::TexBufferPacked * crystalgui_nullable m_glyphAtlasBuffer;
		Ogre::HlmsCrystal	* crystalgui_nullable m_hlms;
		Ogre::VaoManager	* crystalgui_nullable m_vaoManager;

		void growAtlas( size_t sizeBytes );
		size_t getAtlasOffset( size_t sizeBytes );
		CachedGlyph* createGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize );
		void destroyGlyph( CachedGlyphMap::iterator glyphIt );
		void mergeContiguousBlocks( RangeVec::iterator blockToMerge, RangeVec &blocks );

	public:
		ShaperManager( CrystalManager *crystalManager );
		~ShaperManager();

		void setOgre( Ogre::HlmsCrystal * crystalgui_nullable hlms,
					  Ogre::VaoManager * crystalgui_nullable vaoManager );

		Shaper* addShaper( uint32_t /*hb_script_t*/ script, const char *fontPath,
						   const std::string &language );
		void setDefaultShaper( uint16_t font, HorizReadingDir::HorizReadingDir horizReadingDir,
							   bool useVerticalLayoutWhenAvailable );

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
		@return
			Cached glyph
		*/
		const CachedGlyph* acquireGlyph( FT_Face font, uint32_t codepoint, uint32_t ptSize );
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
		void releaseGlyph( uint32_t codepoint, uint32_t ptSize );
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
		@return
			If string is fully LTR, returns Left
			If string is fully RTL, returns Right
			If string is mixed, it returns Mixed
			If string is empty or couldn't be analyzed, it returns Mixed
		*/
		TextHorizAlignment::TextHorizAlignment renderString(
				const char *utf8Str, const RichText &richText, uint32_t richTextIdx,
				VertReadingDir::VertReadingDir vertReadingDir,
				ShapedGlyphVec &outShapes );

		TextHorizAlignment::TextHorizAlignment getDefaultTextDirection() const;

		void updateGpuBuffers();

		static const char* getErrorMessage( FT_Error errorCode );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
