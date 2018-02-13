
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	class CrystalOgreRenderable : public MovableObject, public Renderable
	{
	protected:
		void createBuffers( Ogre::IndexBufferPacked *indexBuffer );
		void destroyBuffers( bool ownsVao );

	public:
		CrystalOgreRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
							   SceneManager* manager, uint8 renderQueueId,
							   Ogre::IndexBufferPacked *indexBuffer,
							   bool ownsVao );
		virtual ~CrystalOgreRenderable();

		/** Creates a prefilled index buffer to be used & reused for rendering.
		@param vaoManager
		@return
		*/
		static Ogre::IndexBufferPacked* createIndexBuffer( VaoManager *vaoManager );

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
