
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "ColibriGui/ColibriRenderable.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	namespace CustomShapeSizeMode
	{
		enum CustomShapeSizeMode
		{
			/// Vertices must be provided in NDC range [-1; 1] and they will be scaled
			/// to fit the widget's size. Vertices beyond this range will be clipped.
			Ndc,

			/// Vertices must be provided in Virtual Canvas units. Widget's size
			/// is ignored and will only affect clipping.
			VirtualCanvas
		};
	}

	/** @ingroup Controls
	@class CustomShape
		A widget used to draw arbitrary 2D shapes.
	*/
	class CustomShape : public Renderable
	{
	protected:
		Ogre::FastArray<UiVertex> m_vertices;

		CustomShapeSizeMode::CustomShapeSizeMode m_sizeMode;

	public:
		CustomShape( ColibriManager *manager );

		void _destroy() override;

		/** Must be called first. Can be called again any time to grow or shrink the buffer.
		@param numTriangles
			Number of triangles you intend to use.
		*/
		void setNumTriangles( size_t numTriangles );

		/// Returns the number of triangles.
		size_t getNumTriangles() const { return m_vertices.size() / 3u; }

		/// Returns the number of vertices.
		size_t getNumVertices() const { return m_vertices.size(); }

		/** Sets the triangle starting at 'vertexIdx' to the given positions, no UVs and a solid colour
		@remarks
			Winding order of v0, v1 & v2 is important for backface culling
			if the material has culling enabled.
		@param vertexIdx
			Vertex Idx to set. In range [0; getNumVertices())
			Must be multiple of 3.
		@param v0
			Position of the 1st vertex.
			See CustomShapeSizeMode::CustomShapeSizeMode for valid range.
		@param v1
			Position of the 2nd vertex
		@param v2
			Position of the 3rd vertex
		@param colour
			Solid colour to set. Values outside range [0; 1] is undefined behavior.
		*/
		void setTriangle( size_t vertexIdx, const Ogre::Vector2 &v0, const Ogre::Vector2 &v1,
						  const Ogre::Vector2 &v2, const Ogre::ColourValue &colour );

		/** Sets the quad (2 triangles) starting at 'vertexIdx' to the given dimensions
		@param vertexIdx
			Vertex Idx to set. In range [0; getNumVertices())
			Must be multiple of 3.
		@param topLeft
			Top Left position.
			See CustomShapeSizeMode::CustomShapeSizeMode for valid range.
		@param size
			Size of the quad.
			bottomRight = topLeft + size;
			See CustomShapeSizeMode::CustomShapeSizeMode for valid range.
		@param colour
			Solid colour to set. Values outside range [0; 1] is undefined behavior.
		@param uvStart
			UV of the topLeft. Values outside range [0; 1] is undefined behavior.
		@param uvSize
			Size of the UVs.
			(uvStart + uvSize) must be in range [0; 1] otherwise it's undefined behavior.
		*/
		void setQuad( size_t vertexIdx, const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size,
					  const Ogre::ColourValue &colour,
					  const Ogre::Vector2     &uvStart = Ogre::Vector2::ZERO,
					  const Ogre::Vector2     &uvSize = Ogre::Vector2::UNIT_SCALE );

		/** Sets the vertex 'idx' to the given position, UV and colour
		@param idx
			Vertex idx to start setting. In range [0; getNumVertices())
			Must be multiple of 3.
		@param pos
			Position of the vertex.
			See CustomShapeSizeMode::CustomShapeSizeMode for valid range.
		@param uv
			UV of the vertex. Values outside range [0; 1] is undefined behavior.
		@param colour
			Colour to set. Values outside range [0; 1] is undefined behavior.
		*/
		void setVertex( size_t idx, const Ogre::Vector2 &pos, const Ogre::Vector2 &uv,
						const Ogre::ColourValue &colour );

		/// Returns raw vertex pointer for direct manipulation.
		/// See getNumVertices()
		UiVertex *getVertices() { return m_vertices.begin(); }

		void _fillBuffersAndCommands(
			UiVertex *colibri_nonnull *colibri_nonnull RESTRICT_ALIAS    vertexBuffer,
			GlyphVertex *colibri_nonnull *colibri_nonnull RESTRICT_ALIAS textVertBuffer,
			const Ogre::Vector2 &parentPos, const Ogre::Vector2 &parentCurrentScrollPos,
			const Matrix2x3 &parentRot ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
