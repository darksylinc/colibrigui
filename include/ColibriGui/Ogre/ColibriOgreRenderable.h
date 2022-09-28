
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreRenderable.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	/** @ingroup Api_Backend
	@class ColibriOgreRenderables
		ColibriOgreRenderables receive the shared Vao from their parents
		(when 'this' is not a window).

		Only Windows have their own Vao, which they share with their
		children (except child windows).

		This is an Ogre3D specifc class needed to render the widgets on screen.
	*/
	class ColibriOgreRenderable : public MovableObject, public Renderable
	{
	public:
		static VertexArrayObject* createVao( uint32 vertexCount, VaoManager *vaoManager );
		static VertexArrayObject* createTextVao( uint32 vertexCount, VaoManager *vaoManager );
		static void destroyVao( VertexArrayObject *vao, VaoManager *vaoManager );
	protected:
		void setVao( VertexArrayObject *vao );

	public:
		ColibriOgreRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
							   SceneManager* manager, uint8 renderQueueId,
							   Colibri::ColibriManager *colibriManager );
		virtual ~ColibriOgreRenderable();

		/** Creates a prefilled index buffer to be used & reused for rendering.
		@param vaoManager
		@return
		*/
		//static Ogre::IndexBufferPacked* createIndexBuffer( VaoManager *vaoManager );

		//Overrides from MovableObject
		virtual const String& getMovableType(void) const;

		//Overrides from Renderable
		virtual const LightList& getLights(void) const;
		virtual void getRenderOperation( v1::RenderOperation& op, bool casterPass );
		virtual void getWorldTransforms( Matrix4* xform ) const;
		virtual bool getCastsShadows(void) const;
	};
}

COLIBRI_ASSUME_NONNULL_END
