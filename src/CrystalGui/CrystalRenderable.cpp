
#include "CrystalGui/CrystalRenderable.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbDrawCall.h"
#include "CommandBuffer/OgreCbPipelineStateObject.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"

#include "CrystalGui/CrystalWindow.h"
#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/Ogre/CrystalOgreRenderable.h"

namespace Crystal
{
	Renderable::Renderable( CrystalManager *manager, bool ownsVao ) :
		Widget( manager ),
		CrystalOgreRenderable( Ogre::Id::generateNewId<Ogre::CrystalOgreRenderable>(),
							   manager->getOgreObjectMemoryManager(),
							   manager->getOgreSceneManager(), 0u,
							   manager->getIndexBuffer(),
							   ownsVao ),
		m_colour( Ogre::ColourValue::White )
	{
		memset( m_stateInformation, 0, sizeof(m_stateInformation) );
	}
	//-------------------------------------------------------------------------
	void Renderable::addCommands( ApiEncapsulatedObjects &apiObject,
								  Ogre::CrystalOgreRenderable *ogreRenderable )
	{
		using namespace Ogre;

		CommandBuffer *commandBuffer = apiObject.commandBuffer;

		/*const HlmsCache *hlmsCache = apiObject.hlms->getMaterial( lastHlmsCache,
																  passCache[datablock->mType],
																  queuedRenderable,
																  vao->getInputLayoutId(),
																  casterPass );
		if( lastHlmsCacheHash != hlmsCache->hash )
		{
			CbPipelineStateObject *psoCmd = commandBuffer->addCommand<CbPipelineStateObject>();
			*psoCmd = CbPipelineStateObject( &hlmsCache->pso );
			lastHlmsCache = hlmsCache;

			//Flush the Vao when changing shaders. Needed by D3D11/12 & possibly Vulkan
			lastVaoName = 0;
		}

		apiObject.hlms->fillBuffersFor( this, apiObject.commandBuffer );*/

		if( apiObject.drawCmd != commandBuffer->getLastCommand() )
		{
			void *offset = reinterpret_cast<void*>( apiObject.indirectBuffer->_getFinalBufferStart() +
													(apiObject.indirectDraw -
													 apiObject.startIndirectDraw) );

			CbDrawCallIndexed *drawCall = commandBuffer->addCommand<CbDrawCallIndexed>();
			*drawCall = CbDrawCallIndexed( apiObject.baseInstanceAndIndirectBuffers,
										   apiObject.vao, offset );
			drawCall->numDraws = 1u;
			apiObject.drawCmd = drawCall;
			apiObject.primCount = 0;

			apiObject.drawCountPtr = reinterpret_cast<CbDrawIndexed*>( apiObject.indirectDraw );
			apiObject.drawCountPtr->primCount		= 0;
			apiObject.drawCountPtr->instanceCount	= 1u;
			apiObject.drawCountPtr->firstVertexIndex=
					apiObject.vao->getIndexBuffer()->_getFinalBufferStart();
			apiObject.drawCountPtr->baseVertex		=
					apiObject.vao->getBaseVertexBuffer()->_getFinalBufferStart();
			apiObject.drawCountPtr->baseInstance	= 0;
			apiObject.indirectDraw += sizeof( CbDrawIndexed );
		}

		apiObject.primCount += 6u;
		apiObject.drawCountPtr->primCount = apiObject.primCount;
	}
	//-------------------------------------------------------------------------
	void Renderable::_destroy()
	{
		destroyBuffers( false );
		Widget::_destroy();
	}
	//-------------------------------------------------------------------------
	void Renderable::_setParent( Widget *parent )
	{
		Widget::_setParent( parent );

		if( !isWindow() )
		{
			Window *window = parent->getFirstParentWindow();
			getSharedBuffersFromParent( window );
		}
	}
	//-------------------------------------------------------------------------
	UiVertex* Renderable::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot )
	{
		if( !m_parent->intersects( this ) )
			return vertexBuffer;

		updateDerivedTransform( parentPos, parentRot );

		Ogre::Vector2 parentDerivedTL = m_parent->m_derivedTopLeft;
		Ogre::Vector2 parentDerivedBR = m_parent->m_derivedBottomRight;

		Ogre::Vector2 invSize = 1.0f / (parentDerivedBR - parentDerivedTL);

		const Ogre::Vector2 topLeft		= this->m_derivedTopLeft;
		const Ogre::Vector2 bottomRight	= this->m_derivedBottomRight;

		vertexBuffer->x = topLeft.x;
		vertexBuffer->y = topLeft.y;
		vertexBuffer->clipDistance[Borders::Top]	= (topLeft.y - parentDerivedTL.y) * invSize.y;
		vertexBuffer->clipDistance[Borders::Left]	= (topLeft.x - parentDerivedTL.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Right]	= (topLeft.x - parentDerivedBR.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Bottom]	= (topLeft.y - parentDerivedBR.y) * invSize.y;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		vertexBuffer->x = topLeft.x;
		vertexBuffer->y = bottomRight.y;
		vertexBuffer->clipDistance[Borders::Top]	= (bottomRight.y - parentDerivedTL.y) * invSize.y;
		vertexBuffer->clipDistance[Borders::Left]	= (topLeft.x - parentDerivedTL.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Right]	= (topLeft.x - parentDerivedBR.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Bottom]	= (bottomRight.y - parentDerivedBR.y) * invSize.y;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		vertexBuffer->x = bottomRight.x;
		vertexBuffer->y = bottomRight.y;
		vertexBuffer->clipDistance[Borders::Top]	= (bottomRight.y - parentDerivedTL.y) * invSize.y;
		vertexBuffer->clipDistance[Borders::Left]	= (bottomRight.x - parentDerivedTL.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Right]	= (bottomRight.x - parentDerivedBR.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Bottom]	= (bottomRight.y - parentDerivedBR.y) * invSize.y;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		vertexBuffer->x = bottomRight.x;
		vertexBuffer->y = topLeft.y;
		vertexBuffer->clipDistance[Borders::Top]	= (topLeft.y - parentDerivedTL.y) * invSize.y;
		vertexBuffer->clipDistance[Borders::Left]	= (bottomRight.x - parentDerivedTL.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Right]	= (bottomRight.x - parentDerivedBR.x) * invSize.x;
		vertexBuffer->clipDistance[Borders::Bottom]	= (topLeft.y - parentDerivedBR.y) * invSize.y;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

//		Ogre::CommandBuffer *commandBuffer = TODO;
//		Ogre::CbDrawCallIndexed *drawCall = mCommandBuffer->addCommand<Ogre::CbDrawCallIndexed>();
//		*drawCall = Ogre::CbDrawCallIndexed( baseInstanceAndIndirectBuffers, vao, offset );

		const Ogre::Matrix3 finalRot = this->m_derivedOrientation;
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			vertexBuffer = (*itor)->fillBuffersAndCommands( vertexBuffer, topLeft, finalRot );
			++itor;
		}

		return vertexBuffer;
	}
}
