
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

	class Renderable : public Widget
	{
		StateInformation m_stateInformation[States::NumStates];

		Ogre::ColourValue	m_colour;

	public:
		Renderable( CrystalManager *manager );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
