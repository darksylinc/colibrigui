
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
#include "CrystalGui/Ogre/CrystalOgreRenderable.h"

#include "OgreBitwise.h"

#define TODO_borderRepeatSize

namespace Crystal
{
	Renderable::Renderable( CrystalManager *manager, bool ownsVao ) :
		Widget( manager ),
		CrystalOgreRenderable( Ogre::Id::generateNewId<Ogre::CrystalOgreRenderable>(),
							   manager->getOgreObjectMemoryManager(),
							   manager->getOgreSceneManager(), 0u,
							   ownsVao ),
		m_colour( Ogre::ColourValue::White )
	{
		memset( m_stateInformation, 0, sizeof(m_stateInformation) );
	}
	//-------------------------------------------------------------------------
	void Renderable::addCommands( ApiEncapsulatedObjects &apiObject )
	{
		using namespace Ogre;

		CommandBuffer *commandBuffer = apiObject.commandBuffer;

		QueuedRenderable queuedRenderable( 0u, this, this );

		uint32 lastHlmsCacheHash = apiObject.lastHlmsCache->hash;
		VertexArrayObject *vao = mVaoPerLod[VpNormal][0];
		const HlmsCache *hlmsCache = apiObject.hlms->getMaterial( apiObject.lastHlmsCache,
																  apiObject.passCache,
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

		apiObject.hlms->fillBuffersForV2( hlmsCache, queuedRenderable, false,
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

		apiObject.primCount += 6u * 9u;
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
	inline void Renderable::addQuad( UiVertex * RESTRICT_ALIAS vertexBuffer,
									 Ogre::Vector2 topLeft,
									 Ogre::Vector2 bottomRight,
									 Ogre::Vector4 uvTopLeftBottomRight,
									 uint8_t *rgbaColour,
									 Ogre::Vector2 parentDerivedTL,
									 Ogre::Vector2 parentDerivedBR,
									 Ogre::Vector2 invSize )
	{
		#define CRYSTAL_ADD_VERTEX( _x, _y, _u, _v, clipDistanceTop, clipDistanceLeft, \
									clipDistanceRight, clipDistanceBottom ) \
			vertexBuffer->x = _x; \
			vertexBuffer->y = _y; \
			vertexBuffer->u = static_cast<uint16_t>( _u * 65535.0f ); \
			vertexBuffer->v = static_cast<uint16_t>( _v * 65535.0f ); \
			vertexBuffer->rgbaColour[0] = rgbaColour[0]; \
			vertexBuffer->rgbaColour[1] = rgbaColour[1]; \
			vertexBuffer->rgbaColour[2] = rgbaColour[2]; \
			vertexBuffer->rgbaColour[3] = rgbaColour[3]; \
			vertexBuffer->clipDistance[Borders::Top]	= clipDistanceTop; \
			vertexBuffer->clipDistance[Borders::Left]	= clipDistanceLeft; \
			vertexBuffer->clipDistance[Borders::Right]	= clipDistanceRight; \
			vertexBuffer->clipDistance[Borders::Bottom]	= clipDistanceBottom; \
			++vertexBuffer;

		CRYSTAL_ADD_VERTEX( topLeft.x, topLeft.y,
							uvTopLeftBottomRight.x, uvTopLeftBottomRight.y,
							(topLeft.y - parentDerivedTL.y) * invSize.y,
							(topLeft.x - parentDerivedTL.x) * invSize.x,
							(topLeft.x - parentDerivedBR.x) * invSize.x,
							(topLeft.y - parentDerivedBR.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( topLeft.x, bottomRight.y,
							uvTopLeftBottomRight.x, uvTopLeftBottomRight.z,
							(bottomRight.y - parentDerivedTL.y) * invSize.y,
							(topLeft.x - parentDerivedTL.x) * invSize.x,
							(topLeft.x - parentDerivedBR.x) * invSize.x,
							(bottomRight.y - parentDerivedBR.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( bottomRight.x, bottomRight.y,
							uvTopLeftBottomRight.w, uvTopLeftBottomRight.z,
							(bottomRight.y - parentDerivedTL.y) * invSize.y,
							(bottomRight.x - parentDerivedTL.x) * invSize.x,
							(bottomRight.x - parentDerivedBR.x) * invSize.x,
							(bottomRight.y - parentDerivedBR.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( bottomRight.x, bottomRight.y,
							uvTopLeftBottomRight.w, uvTopLeftBottomRight.z,
							(bottomRight.y - parentDerivedTL.y) * invSize.y,
							(bottomRight.x - parentDerivedTL.x) * invSize.x,
							(bottomRight.x - parentDerivedBR.x) * invSize.x,
							(bottomRight.y - parentDerivedBR.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( bottomRight.x, topLeft.y,
							uvTopLeftBottomRight.w, uvTopLeftBottomRight.y,
							(topLeft.y - parentDerivedTL.y) * invSize.y,
							(bottomRight.x - parentDerivedTL.x) * invSize.x,
							(bottomRight.x - parentDerivedBR.x) * invSize.x,
							(topLeft.y - parentDerivedBR.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( topLeft.x, topLeft.y,
							uvTopLeftBottomRight.x, uvTopLeftBottomRight.y,
							(topLeft.y - parentDerivedTL.y) * invSize.y,
							(topLeft.x - parentDerivedTL.x) * invSize.x,
							(topLeft.x - parentDerivedBR.x) * invSize.x,
							(topLeft.y - parentDerivedBR.y) * invSize.y );

		#undef CRYSTAL_ADD_VERTEX
	}
	//-------------------------------------------------------------------------
	UiVertex* Renderable::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot )
	{
		if( !m_parent->intersects( this ) )
			return vertexBuffer;

		updateDerivedTransform( parentPos, parentRot );

		uint8_t rgbaColour[4];
		rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		Ogre::Vector2 parentDerivedTL = m_parent->m_derivedTopLeft;
		Ogre::Vector2 parentDerivedBR = m_parent->m_derivedBottomRight;

		Ogre::Vector2 invSize = 1.0f / (parentDerivedBR - parentDerivedTL);

		const Ogre::Vector2 outerTopLeft		= this->m_derivedTopLeft;
		const Ogre::Vector2 outerBottomRight	= this->m_derivedBottomRight;

		const StateInformation stateInfo = m_stateInformation[m_currentState];

		const Ogre::Vector2 borderTopLeft( stateInfo.borderSize[Borders::Left],
										   stateInfo.borderSize[Borders::Top] );
		const Ogre::Vector2 borderBottomRight( stateInfo.borderSize[Borders::Right],
											   stateInfo.borderSize[Borders::Bottom] );
		const Ogre::Vector2 innerTopLeft		= outerTopLeft + mul( this->m_derivedOrientation,
																	  borderTopLeft );
		const Ogre::Vector2 innerBottomRight	= outerBottomRight + mul( this->m_derivedOrientation,
																		  borderBottomRight );
		TODO_borderRepeatSize;
//		stateInfo.borderRepeatSize[Borders::Left] / (innerBottomRight.x - innerTopLeft.x);
//		stateInfo.borderRepeatSize[Borders::Right] / (innerBottomRight.x - innerTopLeft.x);
//		stateInfo.borderRepeatSize[Borders::Top] / (innerBottomRight.y - innerTopLeft.y);
//		stateInfo.borderRepeatSize[Borders::Bottom] / (innerBottomRight.y - innerTopLeft.y);

		//1st row
		addQuad( vertexBuffer,
				 outerTopLeft, innerTopLeft,
				 stateInfo.uvTopLeftBottomRight[0],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		addQuad( vertexBuffer,
				 Ogre::Vector2( innerTopLeft.x, outerTopLeft.y ),
				 Ogre::Vector2( innerBottomRight.x, innerTopLeft.y ),
				 stateInfo.uvTopLeftBottomRight[1],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		addQuad( vertexBuffer,
				 Ogre::Vector2( innerBottomRight.x, outerTopLeft.y ),
				 Ogre::Vector2( outerBottomRight.x, innerTopLeft.y ),
				 stateInfo.uvTopLeftBottomRight[2],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		//2nd row
		addQuad( vertexBuffer,
				 Ogre::Vector2( outerTopLeft.x, innerTopLeft.y ),
				 Ogre::Vector2( innerTopLeft.x, innerBottomRight.y ),
				 stateInfo.uvTopLeftBottomRight[3],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		addQuad( vertexBuffer,
				 Ogre::Vector2( innerTopLeft.x, innerTopLeft.y ),
				 Ogre::Vector2( innerBottomRight.x, innerBottomRight.y ),
				 stateInfo.uvTopLeftBottomRight[4],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		addQuad( vertexBuffer,
				 Ogre::Vector2( innerBottomRight.x, innerTopLeft.y ),
				 Ogre::Vector2( outerBottomRight.x, innerBottomRight.y ),
				 stateInfo.uvTopLeftBottomRight[4],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		//3rd row
		addQuad( vertexBuffer,
				 Ogre::Vector2( outerTopLeft.x, innerBottomRight.y ),
				 Ogre::Vector2( innerTopLeft.x, outerBottomRight.y ),
				 stateInfo.uvTopLeftBottomRight[3],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		addQuad( vertexBuffer,
				 Ogre::Vector2( innerTopLeft.x, innerBottomRight.y ),
				 Ogre::Vector2( innerBottomRight.x, outerBottomRight.y ),
				 stateInfo.uvTopLeftBottomRight[4],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;
		addQuad( vertexBuffer,
				 Ogre::Vector2( innerBottomRight.x, innerBottomRight.y ),
				 Ogre::Vector2( outerBottomRight.x, outerBottomRight.y ),
				 stateInfo.uvTopLeftBottomRight[4],
				 rgbaColour, parentDerivedTL, parentDerivedBR, invSize );
		vertexBuffer += 6u;

//		Ogre::CommandBuffer *commandBuffer = TODO;
//		Ogre::CbDrawCallIndexed *drawCall = mCommandBuffer->addCommand<Ogre::CbDrawCallIndexed>();
//		*drawCall = Ogre::CbDrawCallIndexed( baseInstanceAndIndirectBuffers, vao, offset );

		const Ogre::Matrix3 finalRot = this->m_derivedOrientation;
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			vertexBuffer = (*itor)->fillBuffersAndCommands( vertexBuffer, outerTopLeft, finalRot );
			++itor;
		}

		return vertexBuffer;
	}
}
