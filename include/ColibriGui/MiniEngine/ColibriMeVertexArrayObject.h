
#pragma once

#include "ColibriGui/MiniEngine/ColibriMiniEnginePrerequisites.h"

#include "ColibriGui/MiniEngine/ColibriMeVertexBuffer.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	class MeVertexArrayObject
	{
	protected:
		/// ID of the internal vertex and index buffer layouts. If this ID
		/// doesn't change between two draw calls, then there is no need
		/// to reset the VAO (i.e. same vertex and index buffers are used)
		/// This ID may be shared by many VertexArrayObject instances.
		uint32_t mVaoName;

		/// Used to generate the PSO (@see HlmsPso). RenderSystem-specific quirks:
		///     In OpenGL mInputLayoutId is always 0. mVaoName changes when either the
		///     layout or the internal vertex/index buffer changes
		///
		///     In the rest of the systems, same vertex layouts share the same ID,
		///     mVaoName changes when the vertex/index buffer needs to change.
		uint16_t mInputLayoutId;

		uint32_t  m_primStart;
		uint32_t  m_primCount;
		MeBuffer *colibrigui_nullable m_vertexBuffer;
		MeBuffer *colibrigui_nullable m_indexBuffer;

		MeVertexElementVec const *colibrigui_nullable m_vertexElements;

		/// The type of operation to perform
		PrimitiveType::PrimitiveType m_primitiveType;

	public:
		MeVertexArrayObject( uint32_t vaoName, uint32_t renderQueueId, uint16_t inputLayoutId,
							 MeBuffer *colibrigui_nullable vertexBuffer,
							 MeBuffer *colibrigui_nullable indexBuffer,
							 const MeVertexElementVec *    vertexElements,
							 PrimitiveType::PrimitiveType  operationType );

		uint32_t getVaoName() const { return mVaoName; }
		uint16_t getInputLayoutId() const { return mInputLayoutId; }

		MeBuffer *colibrigui_nullable getVertexBuffer() const { return m_vertexBuffer; }
		MeBuffer *colibrigui_nullable getIndexBuffer() const { return m_indexBuffer; }

		PrimitiveType::PrimitiveType getPrimitiveType() const { return m_primitiveType; }

		uint32_t getPrimitiveStart() const { return m_primStart; }
		uint32_t getPrimitiveCount() const { return m_primCount; }

		/** Limits the range of triangle primitives that is rendered.
			For VAOs with index buffers, this controls the index start & count,
			akin to indexStart & indexCount from the v1 objects.
		@par
			For VAOs with no index buffers, this controls the vertex start & count,
			akin to vertexStart & vertexCount from the v1 objects.
		@remarks
			An exception is thrown if primStart, or primStart + primCount are
			out of the half open range:
			[0; mIndexBuffer->getNumElements()) or [0; mVertexBuffers[0]->getNumElements())
		@par
			Parameters are always in elements (indices or vertices)
		*/
		void setPrimitiveRange( uint32_t primStart, uint32_t primCount );
	};
}  // namespace Ogre

COLIBRIGUI_ASSUME_NONNULL_END
