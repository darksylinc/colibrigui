#include "ColibriGui/Layouts/ColibriLayoutBase.h"
#include "ColibriGui/ColibriWindow.h"
#include "ColibriGui/Layouts/ColibriLayoutLine.h"
#include "ColibriGui/Layouts/ColibriLayoutMultiline.h"
#include "ColibriGui/Layouts/ColibriLayoutTableSameSize.h"

#include <typeinfo>
#include "OgreStringConverter.h"

namespace Colibri
{
	LayoutBase::LayoutBase( ColibriManager *colibriManager ) :
		m_manager( colibriManager ),
		m_currentSize( Ogre::Vector2::ZERO ),
		m_adjustableWindow( 0 ),
		m_preventScrolling( false ),
		m_ignoreRTLSwap( false ),
		m_topLeft( Ogre::Vector2::ZERO ),
		m_hardMaxSize( Ogre::Vector2( std::numeric_limits<float>::max() ) )
	{
	}
	//-------------------------------------------------------------------------
	void LayoutBase::tellChildrenToUpdateLayout( const LayoutCellVec &childrenCells )
	{
		for( LayoutCell *cell : childrenCells )
			cell->notifyLayoutUpdated();
	}
	//-------------------------------------------------------------------------
	void LayoutBase::syncFromWindowSize()
	{
		if( m_adjustableWindow )
		{
			m_topLeft = m_adjustableWindow->getLocalTopLeft();
			m_currentSize = m_adjustableWindow->getSizeAfterClipping();
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

		Ogre::Vector2 windowSize = m_currentSize;

		m_adjustableWindow->setTopLeft( m_topLeft );

		m_adjustableWindow->setSizeAfterClipping( windowSize );
		windowSize = m_adjustableWindow->getSize();
		windowSize.makeFloor( m_hardMaxSize );
		m_adjustableWindow->setSize( windowSize );
		m_adjustableWindow->sizeScrollToFit();
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setAdjustableWindow( Widget *widget )
	{
		m_adjustableWindow = widget;
		m_currentSize = m_adjustableWindow->getSize();
	}
	//-------------------------------------------------------------------------
	Widget *colibri_nullable LayoutBase::getAdjustableWindow() const
	{
		return m_adjustableWindow;
	}
	//-------------------------------------------------------------------------
	void LayoutBase::setMarginToAllCells( const LayoutCellVec &cells, const Ogre::Vector2 &margin )
	{
		for( LayoutCell *cell : cells )
			cell->m_margin = margin;
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
	//-------------------------------------------------------------------------
	/// Helper to get the children cells from a LayoutCell, returns nullptr if no children
	static const LayoutCellVec *getChildren( const LayoutCell *cell )
	{
		if( const LayoutLine *line = dynamic_cast<const LayoutLine *>( cell ) )
			return &line->getCells();
		if( const LayoutMultiline *multiline = dynamic_cast<const LayoutMultiline *>( cell ) )
			return &multiline->getCells();
		// LayoutTableSameSize doesn't have a public getCells() method
		return nullptr;
	}
	//-------------------------------------------------------------------------
	/// Helper to format a single layout cell's info string
	static std::string formatCellInfo( const LayoutCell *cell )
	{
		std::string posInfo = "pos=(?,?)";
		std::string sizeInfo = "size=(?,?)";
		std::string minInfo = "min=(?,?)";
		std::string hardMaxInfo = "hardMax=(?,?)";

		// For Widgets, get actual position and size; for Layouts, use min size info
		if( const Widget *widget = dynamic_cast<const Widget *>( cell ) )
		{
			posInfo = "pos=" + Ogre::StringConverter::toString( widget->getLocalTopLeft() );
			sizeInfo = "size=" + Ogre::StringConverter::toString( widget->getSize() );
			// Widgets don't have getMinSize(), use cell->getCellMinSize() instead
			const Ogre::Vector2 minSize = cell->getCellMinSize();
			minInfo = "getCellMinSize=" + Ogre::StringConverter::toString( minSize );
		}
		else if( const LayoutBase *layout = dynamic_cast<const LayoutBase *>( cell ) )
		{
			// LayoutBase has m_minSize and m_hardMaxSize members
			minInfo = "m_minSize=" + Ogre::StringConverter::toString( layout->m_minSize );
			hardMaxInfo = "m_hardMaxSize=" + Ogre::StringConverter::toString( layout->m_hardMaxSize );

			// Also include getCellMinSize() for consistency
			minInfo += ", getCellMinSize=" + Ogre::StringConverter::toString( cell->getCellMinSize() );

			posInfo = "pos=" + Ogre::StringConverter::toString( layout->m_topLeft );

			// For layout cells, size shows the min size (since actual size not available)
			sizeInfo = "size=" + Ogre::StringConverter::toString( cell->getCellMinSize() );
		}
		else
		{
			// LayoutSpacer or other layout cell types
			const Ogre::Vector2 minSize = cell->getCellMinSize();
			minInfo = "getCellMinSize=" + Ogre::StringConverter::toString( minSize );
			sizeInfo = "size=" + Ogre::StringConverter::toString( minSize );
		}

		if( const LayoutLine *line = dynamic_cast<const LayoutLine *>( cell ) )
		{
			return "[LayoutLine m_vertical=" + Ogre::StringConverter::toString( line->m_vertical ) +
				   " | " + posInfo + " | " + sizeInfo + " | " + minInfo +
				   ( hardMaxInfo.empty() ? "" : " | " + hardMaxInfo ) + "]";
		}
		else if( const LayoutMultiline *multiline = dynamic_cast<const LayoutMultiline *>( cell ) )
		{
			return "[LayoutMultiline m_vertical=" +
				   Ogre::StringConverter::toString( multiline->m_vertical ) + " | " + posInfo + " | " +
				   sizeInfo + " | " + minInfo + ( hardMaxInfo.empty() ? "" : " | " + hardMaxInfo ) + "]";
		}
		else if( const LayoutTableSameSize *table = dynamic_cast<const LayoutTableSameSize *>( cell ) )
		{
			return "[LayoutTableSameSize m_numCols=" +
				   Ogre::StringConverter::toString( table->m_numColumns ) + " | " + posInfo + " | " +
				   sizeInfo + " | " + minInfo + ( hardMaxInfo.empty() ? "" : " | " + hardMaxInfo ) + "]";
		}
		else if( const Widget *widget = dynamic_cast<const Widget *>( cell ) )
		{
			const std::string typeName = typeid( *widget ).name();
			const std::string &debugName = widget->_getDebugName();

			if( !debugName.empty() )
				return "[Colibri::" + typeName + " | " + debugName + " | " + posInfo + " | " + sizeInfo +
					   " | " + minInfo + "]";
			return "[Colibri::" + typeName + " | " + posInfo + " | " + sizeInfo + " | " + minInfo + "]";
		}
		else if( dynamic_cast<const LayoutSpacer *>( cell ) )
		{
			return "[LayoutSpacer | " + posInfo + " | " + sizeInfo +
				   ( minInfo.empty() ? "" : " | " + minInfo ) + "]";
		}
		return "[LayoutCell | " + posInfo + " | " + sizeInfo +
			   ( minInfo.empty() ? "" : " | " + minInfo ) +
			   ( hardMaxInfo.empty() ? "" : " | " + hardMaxInfo ) + "]";
	}
	//-------------------------------------------------------------------------
	/// Helper to dump a layout cell recursively
	static void dumpLayoutCellRecursive( std::string &outStr, const LayoutCell *cell,
										 const std::string &prefix, bool isLast )
	{
		if( cell == nullptr )
			return;

		// Connector: "└── " for last child, "├── " otherwise
		const std::string connector =
			isLast ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 " : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";
		outStr += prefix + connector + formatCellInfo( cell ) + "\n";

		// Build the child prefix: "│   " if more siblings follow, "    " if last
		const std::string childPrefix = isLast ? prefix + "    " : prefix + "\xe2\x94\x82   ";

		// If this layout has an adjustable window, dump its info
		if( const LayoutBase *layout = dynamic_cast<const LayoutBase *>( cell ) )
		{
			Widget *adjustableWindow = layout->getAdjustableWindow();
			if( adjustableWindow )
			{
				const std::string &debugName = adjustableWindow->_getDebugName();
				const std::string posStr =
					Ogre::StringConverter::toString( adjustableWindow->getLocalTopLeft() );
				const std::string sizeStr =
					Ogre::StringConverter::toString( adjustableWindow->getSize() );

				if( !debugName.empty() )
					outStr += prefix + "    " + "[Window " + debugName + " | pos=" + posStr +
							  " | size=" + sizeStr + "]\n";
				else
					outStr += prefix + "    " + "[Window | pos=" + posStr + " | size=" + sizeStr + "]\n";
			}
		}

		// Recursively dump children if this is a layout
		const LayoutCellVec *children = getChildren( cell );
		if( children && !children->empty() )
		{
			for( size_t i = 0; i < children->size(); ++i )
			{
				dumpLayoutCellRecursive( outStr, ( *children )[i], childPrefix,
										 i == children->size() - 1u );
			}
		}
	}
	//-------------------------------------------------------------------------
	void LayoutBase::debugDump( std::string &outStr )
	{
		// Root node: no prefix, no connector
		outStr += formatCellInfo( this ) + "\n";

		// Dump children
		const LayoutCellVec *children = getChildren( this );

		// Dump adjustable window at root level
		if( m_adjustableWindow )
		{
			const std::string &debugName = m_adjustableWindow->_getDebugName();
			const std::string posStr =
				Ogre::StringConverter::toString( m_adjustableWindow->getLocalTopLeft() );
			const std::string sizeStr = Ogre::StringConverter::toString( m_adjustableWindow->getSize() );

			outStr += "    ";
			if( !debugName.empty() )
				outStr += "[Window " + debugName + " | pos=" + posStr + " | size=" + sizeStr + "]\n";
			else
				outStr += "[Window | pos=" + posStr + " | size=" + sizeStr + "]\n";
		}

		if( children && !children->empty() )
		{
			const size_t numChildren = children->size();
			for( size_t i = 0; i < numChildren; ++i )
				dumpLayoutCellRecursive( outStr, ( *children )[i], "", i == numChildren - 1u );
		}
	}
}  // namespace Colibri
