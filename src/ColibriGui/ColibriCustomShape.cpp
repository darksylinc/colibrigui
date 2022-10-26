
#include "ColibriGui/ColibriCustomShape.h"

#include "ColibriRenderable.inl"

using namespace Colibri;

CustomShape::CustomShape( ColibriManager *manager ) :
	Renderable( manager ),
	m_sizeMode( CustomShapeSizeMode::Ndc )
{
	setVao( m_manager->getVao() );

	m_numVertices = 0;
}
//-------------------------------------------------------------------------
void CustomShape::_destroy()
{
	m_manager->_addCustomShapesVertexCountChange( -static_cast<int32_t>( m_vertices.size() ) );
	Renderable::_destroy();
}
//-------------------------------------------------------------------------
void CustomShape::setNumTriangles( const size_t numTriangles )
{
	const size_t oldVertexCount = m_vertices.size();
	const size_t newVertexCount = numTriangles * 3u;
	if( oldVertexCount != newVertexCount )
	{
		m_vertices.resizePOD( newVertexCount );
		m_numVertices = static_cast<uint32_t>( newVertexCount );

		m_manager->_addCustomShapesVertexCountChange(
			static_cast<int32_t>( newVertexCount - oldVertexCount ) );
	}
}
//-------------------------------------------------------------------------
void CustomShape::setTriangle( const size_t idx, const Ogre::Vector2 &v0, const Ogre::Vector2 &v1,
							   const Ogre::Vector2 &v2, const Ogre::ColourValue &colour )
{
	COLIBRI_ASSERT_HIGH( idx % 3u && "idx must be multiple of 3" );
	m_vertices[idx].x = static_cast<float>( v0.x );
	m_vertices[idx].y = static_cast<float>( v0.y );
	m_vertices[idx + 1u].x = static_cast<float>( v1.x );
	m_vertices[idx + 1u].y = static_cast<float>( v1.y );
	m_vertices[idx + 2u].x = static_cast<float>( v2.x );
	m_vertices[idx + 2u].y = static_cast<float>( v2.y );

	const uint8_t rgbaColour[4] = { static_cast<uint8_t>( colour.r * 255.0f + 0.5f ),
									static_cast<uint8_t>( colour.g * 255.0f + 0.5f ),
									static_cast<uint8_t>( colour.b * 255.0f + 0.5f ),
									static_cast<uint8_t>( colour.a * 255.0f + 0.5f ) };

	for( size_t i = 0u; i < 3u; ++i )
	{
		m_vertices[idx + i].u = 0u;
		m_vertices[idx + i].y = 0u;
		m_vertices[idx + i].rgbaColour[0] = rgbaColour[0];
		m_vertices[idx + i].rgbaColour[1] = rgbaColour[1];
		m_vertices[idx + i].rgbaColour[2] = rgbaColour[2];
		m_vertices[idx + i].rgbaColour[3] = rgbaColour[3];
	}
}
//-------------------------------------------------------------------------
void CustomShape::setQuad( size_t idx, const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size,
						   const Ogre::ColourValue &colour, const Ogre::Vector2 &uvStart,
						   const Ogre::Vector2 &uvSize )
{
	COLIBRI_ASSERT_HIGH( idx % 3u && "idx must be multiple of 3" );
	const uint8_t rgbaColour[4] = { static_cast<uint8_t>( colour.r * 255.0f + 0.5f ),
									static_cast<uint8_t>( colour.g * 255.0f + 0.5f ),
									static_cast<uint8_t>( colour.b * 255.0f + 0.5f ),
									static_cast<uint8_t>( colour.a * 255.0f + 0.5f ) };

	const Ogre::Vector2 bottomRight( topLeft + size );
	const Ogre::Vector2 uvEnd( uvStart + uvSize );

	COLIBRI_ASSERT_HIGH( uvStart.x >= 0.0f && uvStart.x <= 1.0f &&  //
						 uvStart.y >= 0.0f && uvStart.y <= 1.0f &&  //
						 uvEnd.x >= 0.0f && uvEnd.x <= 1.0f &&      //
						 uvEnd.y >= 0.0f && uvEnd.y <= 1.0f &&      //
						 "UVs must be in range [0; 1]" );
	COLIBRI_ASSERT_HIGH( colour.r >= 0.0f && colour.r <= 1.0f &&  //
						 colour.g >= 0.0f && colour.g <= 1.0f &&  //
						 colour.b >= 0.0f && colour.b <= 1.0f &&  //
						 colour.a >= 0.0f && colour.a <= 1.0f &&  //
						 "colour must be in range [0; 1]" );

	size_t currVertIdx = idx;
#define COLIBRI_ADD_VERTEX( _x, _y, _u, _v ) \
	m_vertices[currVertIdx].x = static_cast<float>( _x ); \
	m_vertices[currVertIdx].y = static_cast<float>( _y ); \
	m_vertices[currVertIdx].u = static_cast<uint16_t>( _u * 65535.0f ); \
	m_vertices[currVertIdx].v = static_cast<uint16_t>( _v * 65535.0f ); \
	m_vertices[currVertIdx].rgbaColour[0] = rgbaColour[0]; \
	m_vertices[currVertIdx].rgbaColour[1] = rgbaColour[1]; \
	m_vertices[currVertIdx].rgbaColour[2] = rgbaColour[2]; \
	m_vertices[currVertIdx].rgbaColour[3] = rgbaColour[3]; \
	++currVertIdx

	COLIBRI_ADD_VERTEX( topLeft.x, topLeft.y, uvStart.x, uvStart.y );
	COLIBRI_ADD_VERTEX( topLeft.x, bottomRight.y, uvStart.x, uvEnd.y );
	COLIBRI_ADD_VERTEX( bottomRight.x, bottomRight.y, uvEnd.x, uvEnd.y );

	COLIBRI_ADD_VERTEX( bottomRight.x, bottomRight.y, uvEnd.x, uvEnd.y );
	COLIBRI_ADD_VERTEX( bottomRight.x, topLeft.y, uvEnd.x, uvStart.y );
	COLIBRI_ADD_VERTEX( topLeft.x, topLeft.y, uvStart.x, uvStart.y );

#undef COLIBRI_ADD_VERTEX
}
//-------------------------------------------------------------------------
void CustomShape::setVertex( size_t idx, const Ogre::Vector2 &pos, const Ogre::Vector2 &uv,
							 const Ogre::ColourValue &colour )
{
	COLIBRI_ASSERT_HIGH( uv.x >= 0.0f && uv.x <= 1.0f &&  //
						 uv.y >= 0.0f && uv.y <= 1.0f &&  //
						 "UVs must be in range [0; 1]" );
	COLIBRI_ASSERT_HIGH( colour.r >= 0.0f && colour.r <= 1.0f &&  //
						 colour.g >= 0.0f && colour.g <= 1.0f &&  //
						 colour.b >= 0.0f && colour.b <= 1.0f &&  //
						 colour.a >= 0.0f && colour.a <= 1.0f &&  //
						 "colour must be in range [0; 1]" );

	m_vertices[idx].x = static_cast<float>( pos.x );
	m_vertices[idx].y = static_cast<float>( pos.y );
	m_vertices[idx].u = static_cast<uint16_t>( uv.x * 65535.0f );
	m_vertices[idx].y = static_cast<uint16_t>( uv.y * 65535.0f );
	m_vertices[idx].rgbaColour[0] = static_cast<uint8_t>( colour.r * 255.0f + 0.5f );
	m_vertices[idx].rgbaColour[1] = static_cast<uint8_t>( colour.g * 255.0f + 0.5f );
	m_vertices[idx].rgbaColour[2] = static_cast<uint8_t>( colour.b * 255.0f + 0.5f );
	m_vertices[idx].rgbaColour[3] = static_cast<uint8_t>( colour.a * 255.0f + 0.5f );
}

//-------------------------------------------------------------------------
void CustomShape::_fillBuffersAndCommands(
	UiVertex *colibri_nonnull *colibri_nonnull RESTRICT_ALIAS _vertexBuffer,
	GlyphVertex *colibri_nonnull *colibri_nonnull RESTRICT_ALIAS _textVertBuffer,
	const Ogre::Vector2 &parentPos, const Ogre::Vector2 &parentScrollPos, const Matrix2x3 &parentRot )
{
	UiVertex *RESTRICT_ALIAS vertexBuffer = *_vertexBuffer;

	updateDerivedTransform( parentPos, parentRot );

	m_culled = true;

	if( !m_parent->intersectsChild( this, parentScrollPos ) || m_hidden )
		return;

	m_culled = false;

	Ogre::Vector2 parentDerivedTL;
	Ogre::Vector2 parentDerivedBR;

	const Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();

	parentDerivedTL = m_parent->m_derivedTopLeft + m_parent->m_clipBorderTL * invCanvasSize2x;
	parentDerivedBR = m_parent->m_derivedBottomRight - m_parent->m_clipBorderBR * invCanvasSize2x;

	parentDerivedTL.makeCeil( m_parent->m_accumMinClipTL );
	parentDerivedBR.makeFloor( m_parent->m_accumMaxClipBR );
	m_accumMinClipTL = parentDerivedTL;
	m_accumMaxClipBR = parentDerivedBR;

	const Ogre::Vector2 widgetOffset =
		m_sizeMode == CustomShapeSizeMode::Ndc ? Ogre::Vector2( 0.5f ) : Ogre::Vector2::ZERO;
	const Ogre::Vector2 widgetHalfSize =
		m_sizeMode == CustomShapeSizeMode::Ndc ? ( m_size * 0.5f ) : Ogre::Vector2::UNIT_SCALE;

	if( m_visualsEnabled )
	{
		m_currVertexBufferOffset =
			static_cast<uint32_t>( vertexBuffer - m_manager->_getVertexBufferBase() );

		const uint8_t rgbaColour[4] = { static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f ),
										static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f ),
										static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f ),
										static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f ) };

		const Ogre::Vector2 invSize = 1.0f / ( parentDerivedBR - parentDerivedTL );
		const float canvasAspectRatio = m_manager->getCanvasAspectRatio();
		const float invCanvasAspectRatio = m_manager->getCanvasInvAspectRatio();

		const Matrix2x3 derivedRot = m_derivedOrientation;

		for( const UiVertex &vertex : m_vertices )
		{
			Ogre::Vector2 finalPos( Ogre::Real( vertex.x ), Ogre::Real( vertex.y ) );
			finalPos = ( finalPos + widgetOffset ) * widgetHalfSize;

			finalPos = Widget::mul( derivedRot, finalPos.x, finalPos.y * invCanvasAspectRatio );
			finalPos.y *= canvasAspectRatio;

			vertexBuffer->x = static_cast<float>( finalPos.x );
			vertexBuffer->y = static_cast<float>( -finalPos.y );

			vertexBuffer->u = vertex.u;
			vertexBuffer->y = vertex.y;
			for( int i = 0; i < 4; ++i )
				vertexBuffer->rgbaColour[0] = ( vertex.rgbaColour[0] * rgbaColour[0] ) / 255u;

			// Calculate clipping
			vertexBuffer->clipDistance[Borders::Top] =
				static_cast<float>( ( finalPos.y - parentDerivedTL.y ) * invSize.y );
			vertexBuffer->clipDistance[Borders::Left] =
				static_cast<float>( ( finalPos.x - parentDerivedTL.x ) * invSize.x );
			vertexBuffer->clipDistance[Borders::Right] =
				static_cast<float>( ( parentDerivedBR.x - finalPos.x ) * invSize.x );
			vertexBuffer->clipDistance[Borders::Bottom] =
				static_cast<float>( ( parentDerivedBR.y - finalPos.y ) * invSize.y );
			++vertexBuffer;
		}

		*_vertexBuffer = vertexBuffer;

		const Matrix2x3 &finalRot = this->m_derivedOrientation;
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end = m_children.end();

		const Ogre::Vector2 outerTopLeftWithClipping =
			m_derivedTopLeft + ( m_clipBorderTL - parentScrollPos ) * invCanvasSize2x;

		while( itor != end )
		{
			( *itor )->_fillBuffersAndCommands( _vertexBuffer, _textVertBuffer, outerTopLeftWithClipping,
												parentScrollPos, finalRot );
			++itor;
		}
	}
}
