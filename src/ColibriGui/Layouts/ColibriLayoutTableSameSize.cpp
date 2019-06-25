
#include "ColibriGui/Layouts/ColibriLayoutTableSameSize.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriWindow.h"

namespace Colibri
{
	LayoutTableSameSize::LayoutTableSameSize( ColibriManager *colibriManager ) :
		LayoutBase( colibriManager ),
		m_transpose( true ),
		m_numColumns( 1u )
	{
	}
	//-------------------------------------------------------------------------
	size_t LayoutTableSameSize::getNumRows() const
	{
		const size_t numColumns	= std::max( m_numColumns, (size_t)1u );
		return Ogre::alignToNextMultiple( m_cells.size(), numColumns ) / numColumns;
	}
	//-------------------------------------------------------------------------
	void LayoutTableSameSize::addCell( LayoutCell *cell )
	{
		m_cells.push_back( cell );
	}
	//-------------------------------------------------------------------------
	inline Ogre::Vector2 LayoutTableSameSize::getTopLeft( GridLocations::GridLocations gridLoc,
														  const Ogre::Vector2 &accumOffset,
														  const Ogre::Vector2 &biggestSize,
														  const Ogre::Vector2 &finalCellSize,
														  const Ogre::Vector2 &halfMargin )
	{
		Ogre::Vector2 topLeft;

		switch( gridLoc )
		{
		case GridLocations::TopLeft:
		case GridLocations::CenterLeft:
		case GridLocations::BottomLeft:
		case GridLocations::NumGridLocations:
			topLeft.x = accumOffset.x + halfMargin.x;
			break;
		case GridLocations::Top:
		case GridLocations::Center:
		case GridLocations::Bottom:
			topLeft.x = accumOffset.x + (biggestSize.x - finalCellSize.x) * 0.5f + halfMargin.x;
			break;
		case GridLocations::TopRight:
		case GridLocations::CenterRight:
		case GridLocations::BottomRight:
			topLeft.x = accumOffset.x + (biggestSize.x - finalCellSize.x) - halfMargin.x;
			break;
		}

		switch( gridLoc )
		{
		case GridLocations::TopLeft:
		case GridLocations::Top:
		case GridLocations::TopRight:
		case GridLocations::NumGridLocations:
			topLeft.y = accumOffset.y + halfMargin.y;
			break;
		case GridLocations::CenterLeft:
		case GridLocations::Center:
		case GridLocations::CenterRight:
			topLeft.y = accumOffset.y + (biggestSize.y - finalCellSize.y) * 0.5f + halfMargin.y;
			break;
		case GridLocations::BottomLeft:
		case GridLocations::Bottom:
		case GridLocations::BottomRight:
			topLeft.y = accumOffset.y + biggestSize.y - finalCellSize.y - halfMargin.y;
			break;
		}

		return topLeft;
	}
	//-------------------------------------------------------------------------
	void LayoutTableSameSize::layout( bool isRootLayout )
	{
		if( m_cells.empty() )
			return;

		const size_t numCells	= m_cells.size();
		const size_t numColumns	= m_transpose ? std::max( m_numColumns, (size_t)1u ) : getNumRows();
		const size_t numRows	= !m_transpose ? std::max( m_numColumns, (size_t)1u ) : getNumRows();

		const Ogre::Vector2 hardMaxSize = m_hardMaxSize;

		const Ogre::Vector2 layoutMargin = isRootLayout ? m_margin : Ogre::Vector2::ZERO;

		//Sum all proportions
		Ogre::Vector2 biggestSize( 0.0f );

		{
			Ogre::Vector2 softMaxSize = m_currentSize;
			softMaxSize.makeCeil( m_minSize );
			Ogre::Vector2 softMaxSizePerCell = softMaxSize / Ogre::Vector2( numColumns, numRows );
			const Ogre::Vector2 hardMaxSizePerCell = (hardMaxSize - layoutMargin) /
													 Ogre::Vector2( numColumns, numRows );

			LayoutCellVec::const_iterator itor = m_cells.begin();
			LayoutCellVec::const_iterator end  = m_cells.end();

			while( itor != end )
			{
				const LayoutCell *cell = *itor;

				Ogre::Vector2 cellMinSize = cell->getCellMinSize() + cell->m_margin;

				softMaxSizePerCell.makeCeil( cellMinSize );
				cellMinSize.makeCeil( softMaxSizePerCell );
				cellMinSize.makeFloor( hardMaxSizePerCell );

				biggestSize.makeCeil( cellMinSize );

				++itor;
			}
		}

		Ogre::Vector2 accumOffset( 0.0f );
		const Ogre::Vector2 layoutTopLeft = m_adjustableWindow ? Ogre::Vector2::ZERO : m_topLeft;

		//Now apply sizes and offsets
		for( size_t y=0; y<numRows; ++y )
		{
			for( size_t x=0; x<numColumns; ++x )
			{
				const size_t idx = m_transpose ? (y * numColumns + x) :  (x * numRows + y);
				if( idx >= numCells )
					continue;

				LayoutCell *cell = m_cells[idx];

				Ogre::Vector2 availableSize = biggestSize - cell->getCellMinSize();
				availableSize.makeCeil( Ogre::Vector2::ZERO );

				Ogre::Vector2 finalMargin = cell->m_margin;
				finalMargin.makeFloor( availableSize );
				availableSize -= finalMargin;

				Ogre::Vector2 cellSize = biggestSize - finalMargin;

				Ogre::Vector2 hardCellSize = cellSize;

				Ogre::Vector2 finalCellSize = cell->getCellSize();

				//Cells can't be bigger than calculated
				if( cell->m_expand[0] )
					finalCellSize.x = cellSize.x;
				else
					finalCellSize.x = std::min( finalCellSize.x, cellSize.x );

				if( cell->m_expand[1] )
					finalCellSize.y = cellSize.y;
				else
					finalCellSize.y = std::min( finalCellSize.y, cellSize.y );

				const Ogre::Vector2 halfMargin = finalMargin * 0.5f;

				GridLocations::GridLocations gridLoc =
						m_manager->getSwappedGridLocation( cell->m_gridLocation );

				const Ogre::Vector2 topLeft = getTopLeft( gridLoc, accumOffset, cellSize,
														  finalCellSize, halfMargin );

				cell->setCellOffset( layoutTopLeft + topLeft + (layoutMargin * 0.5f) );
				cell->setCellSize( finalCellSize, hardCellSize );

				accumOffset.x += biggestSize.x;
			}

			accumOffset.x = 0.0f;
			accumOffset.y += biggestSize.y;
		}

		m_currentSize = biggestSize * Ogre::Vector2( numColumns, numRows );
		if( m_adjustableWindow )
			syncToWindowSize();

		tellChildrenToUpdateLayout( m_cells );
	}
	//-------------------------------------------------------------------------
	void LayoutTableSameSize::notifyLayoutUpdated()
	{
		layout();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutTableSameSize::getCellMinSize() const
	{
		Ogre::Vector2 biggestSize( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end  = m_cells.end();

		while( itor != end )
		{
			Ogre::Vector2 minCellSize = (*itor)->getCellMinSize() + (*itor)->m_margin;
			biggestSize.makeCeil( minCellSize );
			++itor;
		}

		const size_t numColumns	= m_transpose ? std::max( m_numColumns, (size_t)1u ) : getNumRows();
		const size_t numRows	= !m_transpose ? std::max( m_numColumns, (size_t)1u ) : getNumRows();

		biggestSize = biggestSize * Ogre::Vector2( numColumns, numRows );

		biggestSize.makeCeil( m_minSize );
		biggestSize.makeFloor( m_hardMaxSize );
		return biggestSize;
	}
}
