
#include "CrystalGui/CrystalButton.h"
#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/CrystalLabel.h"

namespace Crystal
{
	Button::Button( CrystalManager *manager ) :
		Renderable( manager ),
		m_label( 0 )
	{
		m_navigable = true;
	}
	//-------------------------------------------------------------------------
	void Button::_destroy()
	{
		Renderable::_destroy();

		//m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	Label* Button::getLabel()
	{
		if( !m_label )
			m_label = m_manager->createWidget<Label>( this );

		return m_label;
	}
}
