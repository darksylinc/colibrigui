
#include "CrystalGui/CrystalLabel.h"

namespace Crystal
{
	Label::Label( CrystalManager *manager ) :
		Renderable( manager )
	{
	}
	//-------------------------------------------------------------------------
	void Label::setText( const std::string &text, States::States forState )
	{
		if( forState == States::NumStates )
		{
			for( size_t i=0; i<States::NumStates; ++i )
				m_text[i] = text;
		}
		else
		{
			m_text[forState] = text;
		}
	}
}
