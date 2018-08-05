
#include "ColibriGui/Layouts/ColibriLayoutBase.h"

namespace Colibri
{
	LayoutBase::LayoutBase( ColibriManager *colibriManager ) :
		m_manager( colibriManager ),
		m_topLeft( Ogre::Vector2::ZERO ),
		m_softMaxSize( Ogre::Vector2::ZERO ),
		m_hardMaxSize( Ogre::Vector2( std::numeric_limits<float>::max() ) )
	{
	}
	//-------------------------------------------------------------------------
	void LayoutBase::tellChildrenToUpdateLayout( const LayoutCellVec &childrenCells )
	{
		LayoutCellVec::const_iterator itor = childrenCells.begin();
		LayoutCellVec::const_iterator end  = childrenCells.end();

		while( itor != end )
		{
			(*itor)->notifyLayoutUpdated();
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setCellOffset( const Ogre::Vector2 &topLeft )
	{
		m_topLeft = topLeft;
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setCellSize( const Ogre::Vector2 &size )
	{
		m_softMaxSize = size;
		m_hardMaxSize = size;
	}
}