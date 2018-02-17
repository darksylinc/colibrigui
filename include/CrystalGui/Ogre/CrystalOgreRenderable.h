
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	/** CrystalOgreRenderables receive the shared Vao from their parents
		(when 'this' is not a window).
		Only Windows have their own Vao, which they share with their
		children (except child windows).
	*/
	class CrystalOgreRenderable : public MovableObject, public Renderable
	{
	protected:
		void createBuffers();
		void getSharedBuffersFromParent( CrystalOgreRenderable *parent );
		void destroyBuffers( bool ownsVao );

	public:
		CrystalOgreRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
							   SceneManager* manager, uint8 renderQueueId,
							   bool ownsVao );
		virtual ~CrystalOgreRenderable();

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

CRYSTALGUI_ASSUME_NONNULL_END
