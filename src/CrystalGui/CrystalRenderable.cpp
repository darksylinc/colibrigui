
#include "CrystalGui/CrystalRenderable.h"

namespace Crystal
{
	Renderable::Renderable( CrystalManager *manager ) :
		Widget( manager ),
		m_colour( Ogre::ColourValue::White )
	{
		memset( m_stateInformation, 0, sizeof(m_stateInformation) );
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
