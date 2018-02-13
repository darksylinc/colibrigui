
#pragma once

#include "CrystalGui/CrystalWidget.h"
#include "CrystalGui/Ogre/CrystalOgreRenderable.h"

#include "OgreColourValue.h"

namespace Ogre
{
	struct CbDrawCallIndexed;
	struct CbDrawIndexed;
	class HlmsCrystalGui;
}

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	struct StateInformation
	{
		Ogre::Vector2		uvPos;
		Ogre::Vector2		uvSize;
		Ogre::Vector2		borderUvStartSize[Borders::NumBorders];
		Ogre::Vector2		borderStartSize[Borders::NumBorders];
		Ogre::HlmsDatablock	*material;
	};

	struct UiVertex
	{
		float x;
		float y;
		uint16_t u;
		uint16_t v;
		uint8_t rgbaColour[4];
		float clipDistance[Borders::NumBorders];
	};

	struct ApiEncapsulatedObjects
	{
		Ogre::HlmsCrystalGui		*hlms;
		Ogre::CommandBuffer			*commandBuffer;
		Ogre::IndirectBufferPacked	*indirectBuffer;
		uint8_t						*indirectDraw;
		uint8_t						*startIndirectDraw;
		int							baseInstanceAndIndirectBuffers;
		Ogre::VertexArrayObject		*vao;
		Ogre::CbDrawCallIndexed		*drawCmd;
		Ogre::CbDrawIndexed			*drawCountPtr;
		uint16_t primCount;
	};

	/**
		Renderable must derive from Ogre::CrystalOgreRenderable (or encapsulated class)
		We can only share Vaos & Vertex Buffers, which will be owned by Window.
	*/
	class Renderable : public Widget, public Ogre::CrystalOgreRenderable
	{
	protected:
		StateInformation m_stateInformation[States::NumStates];

		Ogre::ColourValue	m_colour;

		void addCommands( ApiEncapsulatedObjects &apiObject,
						  Ogre::CrystalOgreRenderable *ogreRenderable );

	public:
		Renderable( CrystalManager *manager, bool ownsVao=false );

		/// See Widget::_destroy
		virtual void _destroy();

		/// See Widget::_setParent
		virtual void _setParent( Widget *parent );

		virtual UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
