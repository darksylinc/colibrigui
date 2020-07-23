
#pragma once

#include "ColibriGui/MiniEngine/ColibriMiniEnginePrerequisites.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	/** @ingroup Api_Backend
	@class ColibriMeRenderable
		ColibriMeRenderable receive the shared Vao from their parents
		(when 'this' is not a window).

		Only Windows have their own Vao, which they share with their
		children (except child windows).

		This is an 3D-agnostic class needed to render the widgets on screen.
	*/
	class ColibriMeRenderable
	{
	protected:
		MeVertexArrayObject *m_vao;

	public:
		static MeVertexArrayObject *createVao( uint32_t vertexCount, MeBufferManager *bufferManager );
		static MeVertexArrayObject *createTextVao( uint32_t         vertexCount,
												   MeBufferManager *bufferManager );
		static void destroyVao( MeVertexArrayObject *vao, MeBufferManager *bufferManager );

	protected:
		void setVao( MeVertexArrayObject *vao );

	public:
		ColibriMeRenderable( Colibri::ColibriManager *colibriManager );
		virtual ~ColibriMeRenderable();
	};
}  // namespace Ogre

COLIBRIGUI_ASSUME_NONNULL_END
