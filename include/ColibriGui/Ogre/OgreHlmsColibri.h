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

namespace Ogre
{
    /** \addtogroup Component
    *  @{
    */
    /** \addtogroup Material
    *  @{
    */

	class HlmsColibriDatablock;

	class HlmsColibri : public HlmsUnlit
    {
	protected:
		TexBufferPacked *mGlyphAtlasBuffer;

		virtual const HlmsCache* createShaderCacheEntry( uint32 renderableHash,
														 const HlmsCache &passCache,
														 uint32 finalHash,
														 const QueuedRenderable &queuedRenderable );

		virtual void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces );

		virtual HlmsDatablock* createDatablockImpl( IdString datablockName,
													const HlmsMacroblock *macroblock,
													const HlmsBlendblock *blendblock,
													const HlmsParamVec &paramVec );

    public:
		HlmsColibri( Archive *dataFolder, ArchiveVec *libraryFolders );
		HlmsColibri( Archive *dataFolder, ArchiveVec *libraryFolders,
					 HlmsTypes type, const String &typeName );
		virtual ~HlmsColibri();

		void setGlyphAtlasBuffer( TexBufferPacked *texBuffer );

		uint32 fillBuffersForColibri( const HlmsCache *cache,
									  const QueuedRenderable &queuedRenderable,
									  bool casterPass, uint32 baseVertex,
									  uint32 lastCacheHash, CommandBuffer *commandBuffer );

        /// @copydoc HlmsPbs::getDefaultPaths
        static void getDefaultPaths( String& outDataFolderPath, StringVector& outLibraryFoldersPaths );
	};

 

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
