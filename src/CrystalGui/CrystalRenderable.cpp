
#include "CrystalGui/CrystalRenderable.h"

namespace Crystal
{
	Renderable::Renderable( CrystalManager *manager ) :
		Widget( manager ),
		m_colour( Ogre::ColourValue::White )
	{
		memset( m_stateInformation, 0, sizeof(m_stateInformation) );
	}
	//-------------------------------------------------------------------------
}
