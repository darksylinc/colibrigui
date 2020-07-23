
#pragma once

#include "ColibriGui/MiniEngine/ColibriMiniEnginePrerequisites.h"

#include "ColibriGui/MiniEngine/ColibriMeVertexBuffer.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	class MeBufferManager
	{
	public:
		MeBuffer *createVertexBuffer( const MeVertexElementVec &vertexElements, size_t vertexCount );

		MeVertexArrayObject *createVertexArrayObject( MeBuffer *colibrigui_nullable vertexBuffer,
													  MeBuffer *colibrigui_nullable      indexBuffer,
													  const MeVertexElementVec &         vertexElements,
													  const PrimitiveType::PrimitiveType primitiveType );

		void destroyVertexArrayObject( MeVertexArrayObject *vao );

		void destroyBuffer( MeBuffer *buffer );
	};
}  // namespace Ogre

COLIBRIGUI_ASSUME_NONNULL_END
