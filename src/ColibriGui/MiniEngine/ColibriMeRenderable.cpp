
#include "ColibriGui/MiniEngine/ColibriMeRenderable.h"

#include "ColibriGui/MiniEngine/ColibriMeBuffer.h"
#include "ColibriGui/MiniEngine/ColibriMeBufferManager.h"
#include "ColibriGui/MiniEngine/ColibriMeVertexArrayObject.h"
#include "ColibriGui/MiniEngine/ColibriMeVertexBuffer.h"

#include "ColibriGui/ColibriManager.h"

#define TODO_PORT

namespace Ogre
{
	ColibriMeRenderable::ColibriMeRenderable( Colibri::ColibriManager *colibriManager ) : m_vao( 0 )
	{
#ifndef TODO_PORT
		setVao( colibriManager->getVao() );
#endif
	}
	//-----------------------------------------------------------------------------------
	ColibriMeRenderable::~ColibriMeRenderable() {}
	//-----------------------------------------------------------------------------------
	MeVertexArrayObject *ColibriMeRenderable::createVao( uint32_t vertexCount,
														 MeBufferManager *bufferManager )
	{
		// Vertex declaration
		MeVertexElementVec vertexElements;
		vertexElements.push_back( MeVertexElement( MVF_FLOAT2, MeVertexSemantic::Position ) );
		vertexElements.push_back( MeVertexElement( MVF_USHORT2_UNORM, MeVertexSemantic::TexCoord ) );
		vertexElements.push_back( MeVertexElement( MVF_UBYTE4_UNORM, MeVertexSemantic::Diffuse ) );
		vertexElements.push_back( MeVertexElement( MVF_FLOAT4, MeVertexSemantic::Normal ) );

		// Create the actual vertex buffer.
		Ogre::MeBuffer *vertexBuffer = 0;
		vertexBuffer = bufferManager->createVertexBuffer( vertexElements, vertexCount );

		MeVertexArrayObject *vao = bufferManager->createVertexArrayObject(
			vertexBuffer, 0, vertexElements, PrimitiveType::TriangleList );

		return vao;
	}
	//-----------------------------------------------------------------------------------
	MeVertexArrayObject *ColibriMeRenderable::createTextVao( uint32_t vertexCount,
															 MeBufferManager *bufferManager )
	{
		// Vertex declaration
		MeVertexElementVec vertexElements;
		vertexElements.push_back( MeVertexElement( MVF_FLOAT2, MeVertexSemantic::Position ) );
		vertexElements.push_back( MeVertexElement( MVF_USHORT2_UNORM, MeVertexSemantic::Indices ) );
		vertexElements.push_back( MeVertexElement( MVF_UINT1, MeVertexSemantic::Tangent ) );
		vertexElements.push_back( MeVertexElement( MVF_UBYTE4_UNORM, MeVertexSemantic::Diffuse ) );
		vertexElements.push_back( MeVertexElement( MVF_FLOAT4, MeVertexSemantic::Normal ) );

		// Create the actual vertex buffer.
		Ogre::MeBuffer *vertexBuffer = 0;
		vertexBuffer = bufferManager->createVertexBuffer( vertexElements, vertexCount );

		MeVertexArrayObject *vao = bufferManager->createVertexArrayObject(
			vertexBuffer, 0, vertexElements, PrimitiveType::TriangleList );

		return vao;
	}
	//-----------------------------------------------------------------------------------
	void ColibriMeRenderable::destroyVao( MeVertexArrayObject *vao, MeBufferManager *bufferManager )
	{
		MeBuffer *vertexBuffer = vao->getVertexBuffer();

		if( vertexBuffer->getMappingState() != MS_UNMAPPED )
			vertexBuffer->unmap( UO_UNMAP_ALL );

		bufferManager->destroyBuffer( vertexBuffer );

		/* Do not destroy the index buffer. We do not own it.
		if( vao->getIndexBuffer() )
			bufferManager->destroyIndexBuffer( vao->getIndexBuffer() );*/
		bufferManager->destroyVertexArrayObject( vao );
	}
	//-----------------------------------------------------------------------------------
	void ColibriMeRenderable::setVao( MeVertexArrayObject *vao ) { m_vao = vao; }
}  // namespace Ogre
