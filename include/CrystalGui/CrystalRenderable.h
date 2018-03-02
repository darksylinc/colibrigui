
#pragma once

#include "CrystalGui/CrystalWidget.h"
#include "CrystalGui/Ogre/CrystalOgreRenderable.h"

#include "OgreColourValue.h"

namespace Ogre
{
	struct CbDrawCallStrip;
	struct CbDrawStrip;
	class HlmsCrystalGui;
}

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	struct StateInformation
	{
		//One set of UVs for each cell in the 3x3 grid.
		Ogre::Vector4	uvTopLeftBottomRight[GridLocations::NumGridLocations];
		float			borderSize[Borders::NumBorders];
		float			borderRepeatSize[Borders::NumBorders]; /// 0 or negative means disable repeat
		Ogre::IdString	materialName;
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
		//Ogre::HlmsCrystalGui		*hlms;
		Ogre::HlmsCache const		*lastHlmsCache;
		Ogre::HlmsCache const 		*passCache;
		Ogre::Hlms					*hlms;
		Ogre::uint32				lastVaoName;
		Ogre::CommandBuffer			*commandBuffer;
		Ogre::IndirectBufferPacked	*indirectBuffer;
		uint8_t						*indirectDraw;
		uint8_t						*startIndirectDraw;
		int							baseInstanceAndIndirectBuffers;
		Ogre::VertexArrayObject		*vao;
		Ogre::CbDrawCallStrip		*drawCmd;
		Ogre::CbDrawStrip			*drawCountPtr;
		uint16_t primCount;
		uint32_t accumPrimCount;
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

		void addCommands( ApiEncapsulatedObjects &apiObject );

		inline void addQuad( UiVertex * RESTRICT_ALIAS vertexBuffer,
							 Ogre::Vector2 topLeft,
							 Ogre::Vector2 bottomRight,
							 Ogre::Vector4 uvTopLeftBottomRight,
							 uint8_t *rgbaColour,
							 Ogre::Vector2 parentDerivedTL,
							 Ogre::Vector2 parentDerivedBR,
							 Ogre::Vector2 invSize );

		virtual void stateChanged( States::States newState );

	public:
		Renderable( CrystalManager *manager );

		void setSkin( Ogre::IdString skinName, States::States forState );
		void setSkinPack( Ogre::IdString skinPackName );

		virtual void setState( States::States state, bool smartHighlight=true );

		virtual void broadcastNewVao( Ogre::VertexArrayObject *vao );

		virtual bool isRenderable() const	{ return true; }

		inline UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												 const Ogre::Vector2 &parentPos,
												 const Ogre::Matrix3 &parentRot,
												 bool forWindows );
		virtual UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
