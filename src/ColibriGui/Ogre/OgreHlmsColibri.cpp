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

#include "OgreStableHeaders.h"

#include "ColibriGui/Ogre/OgreHlmsColibri.h"

#include "ColibriGui/ColibriAssert.h"
#include "ColibriGui/Ogre/OgreHlmsColibriDatablock.h"

#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "OgreCamera.h"
#include "OgreDescriptorSetTexture.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgreRenderQueue.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpu.h"
#include "OgreUnlitProperty.h"
#include "OgreViewport.h"
#include "OgreWorkarounds.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
#	include "Vao/OgreReadOnlyBufferPacked.h"
#	include "OgreRootLayout.h"
#endif

namespace Ogre
{
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 4, 0, 0 )
#	define COLIBRI_TID tid,
#	define COLIBRI_NOTID kNoTid,
#else
#	define COLIBRI_TID
#	define COLIBRI_NOTID
#endif

	extern const String c_unlitBlendModes[];

	HlmsColibri::HlmsColibri( Archive *dataFolder, ArchiveVec *libraryFolders ) :
		HlmsUnlit( dataFolder, libraryFolders ),
		mGlyphAtlasBuffer( 0 )
	{
		mTexUnitSlotStart = 3u;
		mSamplerUnitSlotStart = 3u;
	}
	HlmsColibri::HlmsColibri( Archive *dataFolder, ArchiveVec *libraryFolders, HlmsTypes type,
							  const String &typeName ) :
		HlmsUnlit( dataFolder, libraryFolders, type, typeName ),
		mGlyphAtlasBuffer( 0 )
	{
		mTexUnitSlotStart = 3u;
		mSamplerUnitSlotStart = 3u;
	}
	//-----------------------------------------------------------------------------------
	HlmsColibri::~HlmsColibri() {}
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
	//-----------------------------------------------------------------------------------
	void HlmsColibri::setupRootLayout( RootLayout &rootLayout COLIBRI_TID_ARG_DECL )
	{
		HlmsUnlit::setupRootLayout( rootLayout COLIBRI_TID_ARG );

		if( getProperty( COLIBRI_TID "colibri_text" ) )
		{
			DescBindingRange *descBindingRanges = rootLayout.mDescBindingRanges[0];

			if( getProperty( COLIBRI_TID "use_read_only_buffer" ) )
			{
				descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 3u;
			}
			else
			{
				descBindingRanges[DescBindingTypes::TexBuffer].start = 2u;
				descBindingRanges[DescBindingTypes::TexBuffer].end = 3u;
			}
		}
	}
#endif
	//-----------------------------------------------------------------------------------
	const HlmsCache *HlmsColibri::createShaderCacheEntry(
		uint32 renderableHash, const HlmsCache &passCache, uint32 finalHash,
		const QueuedRenderable &queuedRenderable COLIBRI_STUB_ENTRY_ARG_DECL COLIBRI_TID_ARG_DECL )
	{
		const HlmsCache *retVal =
			HlmsUnlit::createShaderCacheEntry( renderableHash, passCache, finalHash,
											   queuedRenderable COLIBRI_STUB_ENTRY_ARG COLIBRI_TID_ARG );

		if( mShaderProfile != "glsl" )
			return retVal;  // D3D embeds the texture slots in the shader.

		if( getProperty( COLIBRI_TID "colibri_text" ) )
		{
			GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();
			psParams->setNamedConstant( "glyphAtlas", 2 );
			mRenderSystem->bindGpuProgramParameters( GPT_FRAGMENT_PROGRAM, psParams, GPV_ALL );
		}

		return retVal;
	}
	//-----------------------------------------------------------------------------------
	void HlmsColibri::calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces )
	{
		HlmsUnlit::calculateHashForPreCreate( renderable, inOutPieces );

		// See ColibriOgreRenderable
		const Ogre::Renderable::CustomParameterMap &customParams = renderable->getCustomParameters();
		if( customParams.find( 6372 ) != customParams.end() )
		{
			setProperty( COLIBRI_NOTID "colibri_gui", 1 );
			setProperty( COLIBRI_NOTID HlmsBaseProp::IdentityWorld, 1 );

			if( mRenderSystem->getCapabilities()->hasCapability( RSC_USER_CLIP_PLANES ) )
				setProperty( COLIBRI_NOTID HlmsBaseProp::PsoClipDistances, 4 );

			setProperty(
				COLIBRI_NOTID "ogre_version",
				( OGRE_VERSION_MAJOR * 1000000 + OGRE_VERSION_MINOR * 1000 + OGRE_VERSION_PATCH ) );
		}

		// See Colibri::Label
		if( customParams.find( 6373 ) != customParams.end() )
		{
			setProperty( COLIBRI_NOTID "colibri_text", 1 );

			setProperty(
				COLIBRI_NOTID "ogre_version",
				( OGRE_VERSION_MAJOR * 1000000 + OGRE_VERSION_MINOR * 1000 + OGRE_VERSION_PATCH ) );

			if( needsReadOnlyBuffer( mRenderSystem->getCapabilities(), mRenderSystem->getVaoManager() ) )
				setProperty( COLIBRI_NOTID "use_read_only_buffer", 1 );
		}

		// See Colibri::Label
		if( customParams.find( 6374 ) != customParams.end() )
		{
			setProperty( COLIBRI_NOTID "colibri_graph", 1 );
		}
	}
	//-----------------------------------------------------------------------------------
	void HlmsColibri::setGlyphAtlasBuffer( BufferPacked *texBuffer ) { mGlyphAtlasBuffer = texBuffer; }
	//-----------------------------------------------------------------------------------
	bool HlmsColibri::needsReadOnlyBuffer( const RenderSystemCapabilities *caps,
										   const VaoManager *vaoManager )
	{
		// For Qualcomm, see OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
		//
		// PowerVR 8000 also seems to have the same problem with VERY early drivers
		// but we couldn't test what happens if we force it.
		// Their PVR 6000 family also seem to report this. Untested.
		//
		// Mali GPUs are confirmed to actually need this path.
#ifdef OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
		if( !Workarounds::mAdreno5xx6xxMinCaps )
#endif
		{
			if( vaoManager->getTexBufferMaxSize() < 16 * 1024 * 1024 )
				return true;
		}

		return false;
	}
	//-----------------------------------------------------------------------------------
	uint32 HlmsColibri::fillBuffersForColibri( const HlmsCache *cache,
											   const QueuedRenderable &queuedRenderable, bool casterPass,
											   uint32 baseVertex, uint32 lastCacheHash,
											   CommandBuffer *commandBuffer )
	{
		COLIBRI_ASSERT_HIGH( getProperty( cache->setProperties, HlmsBaseProp::GlobalClipPlanes ) == 0 &&
							 "Clipping planes not supported! Generated shader may be buggy!" );

		assert(
			dynamic_cast<const HlmsColibriDatablock *>( queuedRenderable.renderable->getDatablock() ) );
		const HlmsColibriDatablock *datablock =
			static_cast<const HlmsColibriDatablock *>( queuedRenderable.renderable->getDatablock() );

		if( OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH( lastCacheHash ) != mType )
		{
			// We changed HlmsType, rebind the shared textures.
			mLastDescTexture = 0;
			mLastDescSampler = 0;
			mLastBoundPool = 0;

			// layout(binding = 0) uniform PassBuffer {} pass
			ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer - 1];
			*commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
				VertexShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );
			*commandBuffer->addCommand<CbShaderBuffer>() =
				CbShaderBuffer( PixelShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );

			// layout(binding = 2) uniform InstanceBuffer {} instance
			if( mCurrentConstBuffer < mConstBuffers.size() &&
				(size_t)( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) + 4 ) <=
					mCurrentConstBufferSize )
			{
				*commandBuffer->addCommand<CbShaderBuffer>() =
					CbShaderBuffer( VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
				*commandBuffer->addCommand<CbShaderBuffer>() =
					CbShaderBuffer( PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
			}

			// layout(binding = 3) uniform samplerBuffer glyphAtlas
			if( mGlyphAtlasBuffer )
			{
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
				if( mGlyphAtlasBuffer->getBufferPackedType() != Ogre::BP_TYPE_TEX )
				{
					*commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
						PixelShader, 2, static_cast<Ogre::ReadOnlyBufferPacked *>( mGlyphAtlasBuffer ),
						0, 0 );
				}
				else
#endif
				{
					*commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
						PixelShader, 2, static_cast<Ogre::TexBufferPacked *>( mGlyphAtlasBuffer ), 0,
						0 );
				}
			}

			rebindTexBuffer( commandBuffer );

#if OGRE_VERSION_MAJOR == 2 && OGRE_VERSION_MINOR <= 2
			mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock );
#else
			mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock, 2 );
#endif
		}

		// Don't bind the material buffer on caster passes (important to keep
		// MDI & auto-instancing running on shadow map passes)
		if( mLastBoundPool != datablock->getAssignedPool() && !casterPass )
		{
			// layout(binding = 1) uniform MaterialBuf {} materialArray
			const ConstBufferPool::BufferPool *newPool = datablock->getAssignedPool();
			*commandBuffer->addCommand<CbShaderBuffer>() =
				CbShaderBuffer( VertexShader, 1, newPool->materialBuffer, 0,
								(uint32)newPool->materialBuffer->getTotalSizeBytes() );
			*commandBuffer->addCommand<CbShaderBuffer>() =
				CbShaderBuffer( PixelShader, 1, newPool->materialBuffer, 0,
								(uint32)newPool->materialBuffer->getTotalSizeBytes() );
			if( newPool->extraBuffer )
			{
				TexBufferPacked *extraBuffer = static_cast<TexBufferPacked *>( newPool->extraBuffer );
				*commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
					VertexShader, 1, extraBuffer, 0, (uint32)extraBuffer->getTotalSizeBytes() );
			}

			mLastBoundPool = newPool;
		}

		uint32 *RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;
		// float * RESTRICT_ALIAS currentMappedTexBuffer       = mCurrentMappedTexBuffer;

		bool exceedsConstBuffer = (size_t)( ( currentMappedConstBuffer - mStartMappedConstBuffer ) +
											4 ) > mCurrentConstBufferSize;

		const size_t minimumTexBufferSize = 16;
		bool exceedsTexBuffer = false /*(currentMappedTexBuffer - mStartMappedTexBuffer) +
									  minimumTexBufferSize >= mCurrentTexBufferSize*/
			;

		if( exceedsConstBuffer || exceedsTexBuffer )
		{
			currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

			if( exceedsTexBuffer )
				mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof( float ) );
			else
				rebindTexBuffer( commandBuffer, true, minimumTexBufferSize * sizeof( float ) );

			// currentMappedTexBuffer = mCurrentMappedTexBuffer;
		}

		//---------------------------------------------------------------------------
		//                          ---- VERTEX SHADER ----
		//---------------------------------------------------------------------------
		bool useIdentityProjection = queuedRenderable.renderable->getUseIdentityProjection();

		// uint materialIdx[]
		*currentMappedConstBuffer = datablock->getAssignedSlot();
		*reinterpret_cast<float * RESTRICT_ALIAS>( currentMappedConstBuffer + 1 ) =
			datablock->mShadowConstantBias;
		*( currentMappedConstBuffer + 2 ) = useIdentityProjection;
		*( currentMappedConstBuffer + 3 ) = baseVertex;
		currentMappedConstBuffer += 4;

		//---------------------------------------------------------------------------
		//                          ---- PIXEL SHADER ----
		//---------------------------------------------------------------------------

		if( !casterPass )
		{
			if( datablock->mTexturesDescSet != mLastDescTexture )
			{
				// Bind textures
				size_t texUnit = mTexUnitSlotStart;

				if( datablock->mTexturesDescSet )
				{
					*commandBuffer->addCommand<CbTextures>() =
						CbTextures( (uint16)texUnit, std::numeric_limits<uint16>::max(),
									datablock->mTexturesDescSet );

					if( !mHasSeparateSamplers )
					{
						*commandBuffer->addCommand<CbSamplers>() =
							CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
					}

					texUnit += datablock->mTexturesDescSet->mTextures.size();
				}

				mLastDescTexture = datablock->mTexturesDescSet;
			}

			if( datablock->mSamplersDescSet != mLastDescSampler && mHasSeparateSamplers )
			{
				if( datablock->mSamplersDescSet )
				{
					// Bind samplers
					size_t texUnit = mSamplerUnitSlotStart;
					*commandBuffer->addCommand<CbSamplers>() =
						CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
					mLastDescSampler = datablock->mSamplersDescSet;
				}
			}
		}

		mCurrentMappedConstBuffer = currentMappedConstBuffer;
		// mCurrentMappedTexBuffer     = currentMappedTexBuffer;

		return uint32( ( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) >> 2u ) - 1u );
	}
	//-----------------------------------------------------------------------------------
	void HlmsColibri::getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths )
	{
		// We need to know what RenderSystem is currently in use, as the
		// name of the compatible shading language is part of the path
		RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
		String shaderSyntax = "GLSL";
		if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
			shaderSyntax = "HLSL";
		else if( renderSystem->getName() == "Metal Rendering Subsystem" )
			shaderSyntax = "Metal";

		// Fill the library folder paths with the relevant folders
		outLibraryFoldersPaths.clear();
		outLibraryFoldersPaths.emplace_back( "Hlms/Common/" + shaderSyntax );
		outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );
		outLibraryFoldersPaths.push_back( "Hlms/Colibri/Any" );
		outLibraryFoldersPaths.push_back( "Hlms/Unlit/Any" );

		// Fill the data folder path
		outDataFolderPath = "Hlms/Unlit/" + shaderSyntax;
	}
	//-----------------------------------------------------------------------------------
	HlmsDatablock *HlmsColibri::createDatablockImpl( IdString datablockName,
													 const HlmsMacroblock *macroblock,
													 const HlmsBlendblock *blendblock,
													 const HlmsParamVec &paramVec )
	{
		return OGRE_NEW HlmsColibriDatablock( datablockName, this, macroblock, blendblock, paramVec );
	}
}  // namespace Ogre
