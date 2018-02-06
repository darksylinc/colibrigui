
#pragma once

#include "CrystalGui/CrystalWidget.h"

#include "OgreColourValue.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	struct StateInformation
	{
		Ogre::Vector2		uvPos;
		Ogre::Vector2		uvSize;
		float				borderSize[Borders::NumBorders];
		Ogre::HlmsDatablock	*material;
	};

	struct UiVertex
	{
		float x;
		float y;
		float clipDistance[Borders::NumBorders];
		uint8_t rgbaColour[4];
		uint16_t u;
		uint16_t v;
	};

	class Renderable : public Widget
	{
	protected:
		StateInformation m_stateInformation[States::NumStates];

		Ogre::ColourValue	m_colour;

	public:
		Renderable( CrystalManager *manager );

		virtual UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
