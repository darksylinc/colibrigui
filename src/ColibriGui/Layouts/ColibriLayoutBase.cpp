
#include "ColibriGui/Layouts/ColibriLayoutBase.h"
#include "ColibriGui/ColibriWindow.h"

namespace Colibri
{
	LayoutBase::LayoutBase( ColibriManager *colibriManager ) :
		m_manager( colibriManager ),
		m_currentSize( Ogre::Vector2::ZERO ),
		m_adjustableWindow( 0 ),
		m_preventScrolling( false ),
		m_topLeft( Ogre::Vector2::ZERO ),
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
	void LayoutBase::syncFromWindowSize()
	{
		if( m_adjustableWindow )
		{
			m_topLeft = m_adjustableWindow->getLocalTopLeft();
			m_currentSize = m_adjustableWindow->getSize();
			m_currentSize.makeCeil( m_minSize );
			m_currentSize.makeFloor( m_hardMaxSize );
			m_adjustableWindow->setSize( m_currentSize );
		}
	}
	//-------------------------------------------------------------------------
	void LayoutBase::syncToWindowSize()
	{
		if( !m_adjustableWindow )
			return;

		const Ogre::Vector2 layoutMargin = m_margin * 0.5f;
		Ogre::Vector2 windowSize = m_currentSize + layoutMargin;

		m_adjustableWindow->setTopLeft( m_topLeft );

		m_adjustableWindow->setSizeAfterClipping( windowSize );
		windowSize = m_adjustableWindow->getSize();
		windowSize.makeFloor( m_hardMaxSize );
		m_adjustableWindow->setSize( windowSize );
		m_adjustableWindow->sizeScrollToFit();
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setAdjustableWindow( Window * colibrigui_nullable window )
	{
		m_adjustableWindow = window;
		m_currentSize = m_adjustableWindow->getSize();
	}
	//-------------------------------------------------------------------------
	Widget * colibrigui_nullable LayoutBase::getAdjustableWindow() const
	{
		return m_adjustableWindow;
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setCellOffset( const Ogre::Vector2 &topLeft )
	{
		m_topLeft = topLeft;
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setCellSize( const Ogre::Vector2 &size )
	{
		m_currentSize = size;
		m_hardMaxSize = size;
		m_currentSize.makeCeil( m_minSize );
		if( m_adjustableWindow )
			syncToWindowSize();
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setCellSize( const Ogre::Vector2 &size, const Ogre::Vector2 &hardSize )
	{
		m_currentSize = size;
		m_hardMaxSize = hardSize;
		m_currentSize.makeCeil( m_minSize );
		if( m_adjustableWindow )
			syncToWindowSize();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutBase::getCellSize() const
	{
		return m_adjustableWindow ? m_adjustableWindow->getSize() : m_currentSize;
	}
}
