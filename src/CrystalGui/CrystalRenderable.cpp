
#include "CrystalGui/CrystalRenderable.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbDrawCall.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbPipelineStateObject.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#include "OgreRenderQueue.h"
#include "OgreHlms.h"

#include "CrystalGui/CrystalWindow.h"
#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/CrystalSkinManager.h"
#include "CrystalGui/Ogre/CrystalOgreRenderable.h"
#include "CrystalGui/Ogre/OgreHlmsCrystal.h"

#include "OgreBitwise.h"

#include "CrystalRenderable.inl"

namespace Crystal
{
	Renderable::Renderable( CrystalManager *manager ) :
		Widget( manager ),
		CrystalOgreRenderable( Ogre::Id::generateNewId<Ogre::CrystalOgreRenderable>(),
							   manager->getOgreObjectMemoryManager(),
							   manager->getOgreSceneManager(), 0u, manager ),
		m_colour( Ogre::ColourValue::White )
	{
		memset( m_stateInformation, 0, sizeof(m_stateInformation) );
	}
	//-------------------------------------------------------------------------
	void Renderable::stateChanged( States::States newState )
	{
		setDatablock( m_stateInformation[newState].materialName );
	}
	//-------------------------------------------------------------------------
	void Renderable::setSkin( Ogre::IdString skinName, States::States forState )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinInfoMap &skins = skinManager->getSkins();

		SkinInfoMap::const_iterator itor = skins.find( skinName );
		if( itor != skins.end() )
		{
			if( forState == States::NumStates )
			{
				for( size_t i=0; i<States::NumStates; ++i )
					m_stateInformation[i] = itor->second.stateInfo;
				setDatablock( m_stateInformation[0].materialName );
			}
			else
			{
				m_stateInformation[forState] = itor->second.stateInfo;
				if( forState == m_currentState )
					setDatablock( m_stateInformation[forState].materialName );
			}
		}
	}
	//-------------------------------------------------------------------------
	void Renderable::setSkinPack( Ogre::IdString skinName )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinInfoMap &skins = skinManager->getSkins();
		const SkinPackMap &skinPacks = skinManager->getSkinPacks();

		SkinPackMap::const_iterator itor = skinPacks.find( skinName );
		if( itor != skinPacks.end() )
		{
			const SkinPack &pack = itor->second;

			for( size_t i=0; i<States::NumStates; ++i )
			{
				SkinInfoMap::const_iterator itSkinInfo = skins.find( pack.skinInfo[i] );
				if( itSkinInfo != skins.end() )
				{
					m_stateInformation[i] = itSkinInfo->second.stateInfo;
					if( i == m_currentState )
						setDatablock( m_stateInformation[i].materialName );
				}
			}
		}
	}
	//-------------------------------------------------------------------------
	void Renderable::setState( States::States state, bool smartHighlight )
	{
		Widget::setState( state, smartHighlight );

		if( mHlmsDatablock->getName() != m_stateInformation[m_currentState].materialName )
			setDatablock( m_stateInformation[m_currentState].materialName );
	}
	//-------------------------------------------------------------------------
	void Renderable::broadcastNewVao( Ogre::VertexArrayObject *vao )
	{
		setVao( vao );
		Widget::broadcastNewVao( vao );
	}
	//-------------------------------------------------------------------------
	void Renderable::addCommands( ApiEncapsulatedObjects &apiObject )
	{
		using namespace Ogre;

		CommandBuffer *commandBuffer = apiObject.commandBuffer;

		QueuedRenderable queuedRenderable( 0u, this, this );

		uint32 lastHlmsCacheHash = apiObject.lastHlmsCache->hash;
		VertexArrayObject *vao = apiObject.vao;
		const HlmsCache *hlmsCache = apiObject.hlms->getMaterial( apiObject.lastHlmsCache,
																  *apiObject.passCache,
																  queuedRenderable,
																  vao->getInputLayoutId(),
																  false );
		if( lastHlmsCacheHash != hlmsCache->hash )
		{
			CbPipelineStateObject *psoCmd = commandBuffer->addCommand<CbPipelineStateObject>();
			*psoCmd = CbPipelineStateObject( &hlmsCache->pso );
			apiObject.lastHlmsCache = hlmsCache;

			//Flush the Vao when changing shaders. Needed by D3D11/12 & possibly Vulkan
			apiObject.lastVaoName = 0;
		}

		uint32 baseInstance = apiObject.hlms->fillBuffersForCrystal(
								  hlmsCache, queuedRenderable, false, apiObject.accumPrimCount,
								  lastHlmsCacheHash, apiObject.commandBuffer );

		if( apiObject.drawCmd != commandBuffer->getLastCommand() ||
			apiObject.lastVaoName != vao->getVaoName() )
		{
			{
				*commandBuffer->addCommand<CbVao>() = CbVao( vao );
				*commandBuffer->addCommand<CbIndirectBuffer>() =
						CbIndirectBuffer( apiObject.indirectBuffer );
				apiObject.lastVaoName = vao->getVaoName();
			}

			void *offset = reinterpret_cast<void*>( apiObject.indirectBuffer->_getFinalBufferStart() +
													(apiObject.indirectDraw -
													 apiObject.startIndirectDraw) );

			CbDrawCallStrip *drawCall = commandBuffer->addCommand<CbDrawCallStrip>();
			*drawCall = CbDrawCallStrip( apiObject.baseInstanceAndIndirectBuffers,
										 apiObject.vao, offset );
			drawCall->numDraws = 1u;
			apiObject.drawCmd = drawCall;
			apiObject.primCount = 0;

			apiObject.drawCountPtr = reinterpret_cast<CbDrawStrip*>( apiObject.indirectDraw );
			apiObject.drawCountPtr->primCount		= 0;
			apiObject.drawCountPtr->instanceCount	= 1u;
			apiObject.drawCountPtr->firstVertexIndex=apiObject.accumPrimCount;
			apiObject.drawCountPtr->baseInstance	= baseInstance;
			apiObject.indirectDraw += sizeof( CbDrawStrip );
		}

		apiObject.primCount += 6u * 9u;
		apiObject.accumPrimCount += 6u * 9u;
		apiObject.drawCountPtr->primCount = apiObject.primCount;

		WidgetVec::const_iterator itor = m_children.begin() + m_numNonRenderables;
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			CRYSTAL_ASSERT_HIGH( dynamic_cast<Renderable*>( *itor ) );
			Renderable *asRenderable = static_cast<Renderable*>( *itor );
			asRenderable->addCommands( apiObject );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	UiVertex* Renderable::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot )
	{
		return fillBuffersAndCommands( vertexBuffer, parentPos, parentRot, false );
	}
}
