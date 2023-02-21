/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef _OgreHlmsColibri_H_
#define _OgreHlmsColibri_H_

#include "OgreHlmsUnlit.h"

#ifndef OGRE_MAKE_VERSION
#	define OGRE_MAKE_VERSION( maj, min, patch ) ( ( maj << 16 ) | ( min << 8 ) | patch )
#endif

#if OGRE_VERSION >= OGRE_MAKE_VERSION( 4, 0, 0 )
#	define COLIBRI_TID_ARG_DECL , const size_t tid
#	define COLIBRI_TID_ARG , tid
#else
#	define COLIBRI_TID_ARG_DECL
#	define COLIBRI_TID_ARG
#endif

namespace Ogre
{
	class HlmsColibriDatablock;
	/** @ingroup Api_Backend
	@class HlmsColibri
		Overrides HlmsUnlit
		That way wecan create shaders specifically tailored for our UI widgets, while also
		using vanilla Unlit for everything else. But text rendering requires special shaders,
		that is different from UI widgets and from regular entities. To identify them, we overloaded
		calculateHashForPreCreate (which gets called when a Renderable is assigned a new
		datablock/material) and use the magic numbers "6372" to identify the Renderable as an
		UI widget, and "6373" for text widgets. We only look for the presence of the key, and we
		don't care about the value.

		This works because basically all UI widgets follow a different path from regular Unlit,
		and all text widgets follow a different path from the other two (so there's a total of 3 paths).

		UI widgets are only meant to be rendered from a custom compositor pass we provide, which
		is why we use fillBuffersForColibri that handles our special needs, instead of
		overloading the regular fillBuffersForV2.

		@see	CompositorPassColibriGuiProvider
	*/
	class HlmsColibri : public HlmsUnlit
	{
	protected:
		// It's ReadOnlyBufferPacked on Mali
		// It's TexBufferPacked everywhere else
		BufferPacked *mGlyphAtlasBuffer;

#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
		void setupRootLayout( RootLayout &rootLayout COLIBRI_TID_ARG_DECL ) override;
#endif

		const HlmsCache *createShaderCacheEntry(
			uint32 renderableHash, const HlmsCache &passCache, uint32 finalHash,
			const QueuedRenderable &queuedRenderable COLIBRI_TID_ARG_DECL ) override;

		void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override;

		HlmsDatablock *createDatablockImpl( IdString datablockName, const HlmsMacroblock *macroblock,
											const HlmsBlendblock *blendblock,
											const HlmsParamVec   &paramVec ) override;

	public:
		HlmsColibri( Archive *dataFolder, ArchiveVec *libraryFolders );
		HlmsColibri( Archive *dataFolder, ArchiveVec *libraryFolders, HlmsTypes type,
					 const String &typeName );
		~HlmsColibri() override;

		void setGlyphAtlasBuffer( BufferPacked *texBuffer );

		/// Returns true if the GPU supports TexBufferPacked sizes so small
		/// that we need a ReadOnlyBuffer instead.
		static bool needsReadOnlyBuffer( const RenderSystemCapabilities *caps,
										 const VaoManager               *vaoManager );

		uint32 fillBuffersForColibri( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
									  bool casterPass, uint32 baseVertex, uint32 lastCacheHash,
									  CommandBuffer *commandBuffer );

		/// @copydoc HlmsPbs::getDefaultPaths
		static void getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths );
	};
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
