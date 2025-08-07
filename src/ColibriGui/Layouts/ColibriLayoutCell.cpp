
#include "ColibriGui/Layouts/ColibriLayoutCell.h"

namespace Colibri
{
	LayoutCell::LayoutCell() :
		m_priority( 0 ),
		m_gridLocation( GridLocations::TopLeft ),
		m_margin( Ogre::Vector2::ZERO ),
		m_minSize( Ogre::Vector2::ZERO )
	{
		m_proportion[0] = 0;
		m_proportion[1] = 0;

		m_expand[0] = false;
		m_expand[1] = false;
	}
	//-------------------------------------------------------------------------
	LayoutCell::~LayoutCell()
	{
	}
	//-------------------------------------------------------------------------
	void LayoutCell::setCellSize( const Ogre::Vector2 &size, const Ogre::Vector2 &hardSize )
	{
		setCellSize( size );
	}
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	LayoutSpacer LayoutSpacer::c_DefaultBlankSpacer;

	LayoutSpacer::LayoutSpacer()
	{
		m_proportion[0] = 1;
		m_proportion[1] = 1;
		m_expand[0] = true;
		m_expand[1] = true;
	}
	void LayoutSpacer::setCellOffset( const Ogre::Vector2 &topLeft )
	{
	}
	//-------------------------------------------------------------------------
	void LayoutSpacer::setCellSize( const Ogre::Vector2 &size )
	{
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutSpacer::getCellSize() const
	{
		return m_minSize;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutSpacer::getCellMinSize() const
	{
		return m_minSize;
	}
}
