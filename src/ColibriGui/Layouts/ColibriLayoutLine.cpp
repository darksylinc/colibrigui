
#include "ColibriGui/Layouts/ColibriLayoutLine.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriWindow.h"

namespace Colibri
{
	LayoutLine::LayoutLine( ColibriManager *colibriManager ) :
		LayoutBase( colibriManager ),
		m_vertical( true ),
		m_evenMarginSpaceAtEdges( true )
	{
	}
	//-------------------------------------------------------------------------
	const LayoutCellVec& LayoutLine::getCells() const
	{
		return m_cells;
	}
	//-------------------------------------------------------------------------
	void LayoutLine::addCell( LayoutCell *cell )
	{
		m_cells.push_back( cell );
	}
	//-------------------------------------------------------------------------
	inline Ogre::Vector2 LayoutLine::getTopLeft( bool bVertical,
												 GridLocations::GridLocations gridLoc,
												 float accumOffset, float cellSize,
												 float maxOtherSize,
												 const Ogre::Vector2 &finalCellSize,
												 const Ogre::Vector2 &halfMargin )
	{
		Ogre::Vector2 topLeft;

		if( !bVertical )
		{
			switch( gridLoc )
			{
			case GridLocations::TopLeft:
			case GridLocations::CenterLeft:
			case GridLocations::BottomLeft:
			case GridLocations::NumGridLocations:
				topLeft.x = accumOffset + halfMargin.x;
				break;
			case GridLocations::Top:
			case GridLocations::Center:
			case GridLocations::Bottom:
				topLeft.x = accumOffset + (cellSize - finalCellSize.x) * 0.5f + halfMargin.x;
				break;
			case GridLocations::TopRight:
			case GridLocations::CenterRight:
			case GridLocations::BottomRight:
				topLeft.x = accumOffset + (cellSize - finalCellSize.x) - halfMargin.x;
				break;
			}

			switch( gridLoc )
			{
			case GridLocations::TopLeft:
			case GridLocations::Top:
			case GridLocations::TopRight:
			case GridLocations::NumGridLocations:
				topLeft.y = halfMargin.y;
				break;
			case GridLocations::CenterLeft:
			case GridLocations::Center:
			case GridLocations::CenterRight:
				topLeft.y = (maxOtherSize - finalCellSize.y) * 0.5f;
				break;
			case GridLocations::BottomLeft:
			case GridLocations::Bottom:
			case GridLocations::BottomRight:
				topLeft.y = maxOtherSize - finalCellSize.y - halfMargin.y;
				break;
			}
		}
		else
		{
			switch( gridLoc )
			{
			case GridLocations::TopLeft:
			case GridLocations::Top:
			case GridLocations::TopRight:
			case GridLocations::NumGridLocations:
				topLeft.y = accumOffset + halfMargin.y;
				break;
			case GridLocations::CenterLeft:
			case GridLocations::Center:
			case GridLocations::CenterRight:
				topLeft.y = accumOffset + (cellSize - finalCellSize.y) * 0.5f + halfMargin.y;
				break;
			case GridLocations::BottomLeft:
			case GridLocations::Bottom:
			case GridLocations::BottomRight:
				topLeft.y = accumOffset + (cellSize - finalCellSize.y) - halfMargin.y;
				break;
			}

			switch( gridLoc )
			{
			case GridLocations::TopLeft:
			case GridLocations::CenterLeft:
			case GridLocations::BottomLeft:
			case GridLocations::NumGridLocations:
				topLeft.x = halfMargin.x;
				break;
			case GridLocations::Top:
			case GridLocations::Center:
			case GridLocations::Bottom:
				topLeft.x = (maxOtherSize - finalCellSize.x) * 0.5f;
				break;
			case GridLocations::TopRight:
			case GridLocations::CenterRight:
			case GridLocations::BottomRight:
				topLeft.x = maxOtherSize - finalCellSize.x - halfMargin.x;
				break;
			}
		}

		return topLeft;
	}
	//-------------------------------------------------------------------------
	struct SortByPriorityLowToHigh
	{
		const LayoutCellVec &cells;

		SortByPriorityLowToHigh( const LayoutCellVec &_cells ) :
			cells( _cells ) {}

		bool operator () ( size_t _a, size_t _b ) const
		{
			if( cells[_a]->m_priority != cells[_b]->m_priority )
				return cells[_a]->m_priority < cells[_b]->m_priority;

			return _a < _b;
		}
	};
	struct SortByPriorityHighToLow
	{
		const LayoutCellVec &cells;

		SortByPriorityHighToLow( const LayoutCellVec &_cells ) :
			cells( _cells ) {}

		bool operator () ( size_t _a, size_t _b ) const
		{
			if( cells[_a]->m_priority != cells[_b]->m_priority )
				return cells[_a]->m_priority > cells[_b]->m_priority;

			return _a > _b;
		}
	};
	void LayoutLine::layout()
	{
		if( m_cells.empty() )
			return;

		syncFromWindowSize();

		const bool bVertical = m_vertical;

		std::vector<float> cellSizes;
		cellSizes.resize( m_cells.size(), 0.0f );
		std::vector<size_t> freeCells;
		freeCells.reserve( m_cells.size() );
		std::vector<size_t> exceededCells;
		exceededCells.reserve( m_cells.size() );

		const Ogre::Vector2 layoutMargin = m_adjustableWindow ? (m_margin * 0.5f) : Ogre::Vector2::ZERO;

		//Sum all proportions
		size_t maxProportion = 0;
		float sizeToDistribute = 0;		//Vertical / Horizontal size
		float maxOtherSize = layoutMargin[!m_vertical];	//Horizontal / Vertical size
														//(opposite axis of minMaxSize)
		float nonProportionalSize = 0;
		float accumMarginSize = layoutMargin[m_vertical];

		{
			LayoutCellVec::const_iterator itor = m_cells.begin();
			LayoutCellVec::const_iterator end  = m_cells.end();

			while( itor != end )
			{
				const LayoutCell *cell = *itor;
				maxProportion	+= cell->m_proportion[bVertical];
				const Ogre::Vector2 cellMinSize = cell->getCellMinSize();
				maxOtherSize = Ogre::max( maxOtherSize,
										  cellMinSize[!bVertical] + cell->m_margin[!bVertical] );
				if( !cell->m_proportion[bVertical] )
					nonProportionalSize += cellMinSize[bVertical];
				else
					sizeToDistribute += cellMinSize[bVertical];
				accumMarginSize += cell->m_margin[bVertical];
				++itor;
			}

			if( m_evenMarginSpaceAtEdges )
			{
				if( !m_cells.front()->m_expand[bVertical] )
					accumMarginSize += m_cells.front()->m_margin[bVertical] * 0.5f;

				if( !m_cells.back()->m_expand[bVertical] )
					accumMarginSize += m_cells.back()->m_margin[bVertical] * 0.5f;
			}
		}

		Ogre::Vector2 adjWindowBorders( Ogre::Vector2::ZERO );
		if( m_adjustableWindow )
			adjWindowBorders = m_adjustableWindow->getBorderCombined();

		//Calculate all cell sizes as if there were no size restrictions
		const bool canScroll = m_adjustableWindow != 0 && !m_preventScrolling;
		const size_t numCells = m_cells.size();
		const Ogre::Vector2 softMaxSize = m_currentSize;
		const Ogre::Vector2 hardMaxSize = m_hardMaxSize - adjWindowBorders;
		sizeToDistribute = std::max( softMaxSize[bVertical] - accumMarginSize - nonProportionalSize,
									 sizeToDistribute );
		if( !canScroll )
		{
			sizeToDistribute = std::min( sizeToDistribute,
										 hardMaxSize[bVertical] - nonProportionalSize );
		}
		sizeToDistribute = std::max( sizeToDistribute, 0.0f );
		const float invMaxProportion = 1.0f / static_cast<float>( maxProportion );
		maxOtherSize = Ogre::max( maxOtherSize, softMaxSize[!bVertical] );
		float nonProportionalFactor = 1.0f;
		if( !canScroll )
		{
			maxOtherSize = Ogre::min( maxOtherSize, hardMaxSize[!bVertical] );
			//If nonProportionalSize is bigger than hardMaxSize, widgets just don't fit.
			//Make them all proportionally smaller.
			nonProportionalFactor = std::min( hardMaxSize[bVertical] / nonProportionalSize, 1.0f );
		}

		const float spaceLeftForMargins = Ogre::Math::Clamp( hardMaxSize[bVertical] -
															 sizeToDistribute -
															 nonProportionalSize,
															 0.0f, accumMarginSize );
		//marginFactor will be in range [0; 1] because
		//spaceLeftForMargins is in range [0; accumMarginSize]
		const float marginFactor =
				(!canScroll && accumMarginSize > 1e-6f) ? (spaceLeftForMargins / accumMarginSize) : 1.0f;

		//Now check if there are proportional cells which will
		//be assigned a size lower than they can shrink
		for( size_t i=0; i<numCells; ++i )
		{
			const LayoutCell *cell = m_cells[i];

			uint16_t proportion = cell->m_proportion[bVertical];

			float minCellSize = cell->getCellMinSize()[bVertical];
			if( proportion > 0 )
			{
				float cellLineSize = proportion * (sizeToDistribute * invMaxProportion);

				//Push this cell as being able to shrink if needed
				if( cellLineSize > minCellSize )
					freeCells.push_back( i );
				else
					exceededCells.push_back( i );

				cellSizes[i] = cellLineSize;
			}
			else
			{
				cellSizes[i] = minCellSize * nonProportionalFactor;
			}
		}

		if( !exceededCells.empty() )
		{
			//Some proportional cells must steal size from other proportional cells
			bool hasUnresolvedCells = false;

			//Sort by priority, so low priority cells steal from other low priority before the
			//high priority ones steal away all the available space that could've been available
			//for the low priority cells.
			std::sort( exceededCells.begin(), exceededCells.end(), SortByPriorityLowToHigh( m_cells ) );
			//Sort by reverse priority, so that they're cheaper to pop_back from vector
			std::sort( freeCells.begin(), freeCells.end(), SortByPriorityHighToLow( m_cells ) );

			std::vector<size_t>::const_iterator itExceeded = exceededCells.begin();
			std::vector<size_t>::const_iterator enExceeded = exceededCells.end();

			while( itExceeded != enExceeded )
			{
				const size_t idx = *itExceeded;

				const LayoutCell *cell = m_cells[idx];

				const float minCellSize = cell->getCellMinSize()[bVertical];

				const uint8_t priority = cell->m_priority;
				float missingSize = minCellSize - cellSizes[idx];

				while( !freeCells.empty() && missingSize > 0 )
				{
					const size_t freeIdx = freeCells.back();
					const LayoutCell *freeCell = m_cells[freeIdx];
					if( priority < freeCell->m_priority )
						break; //Early out

					const float minFreeCellSize = freeCell->getCellMinSize()[bVertical];
					float excessSize = cellSizes[freeIdx] - minFreeCellSize;
					const float sizeToSteal = std::min( excessSize, missingSize );

					//Shrink cell being stolen, enlarge the thief
					cellSizes[freeIdx]	-= sizeToSteal;
					cellSizes[idx]		+= sizeToSteal;

					//Keep track of how much we still need to steal
					missingSize	-= sizeToSteal;
					excessSize	-= sizeToSteal;

					if( excessSize <= 1e-6f )
					{
						//There nothing left to steal from this cell. Remove it
						freeCells.pop_back();
					}
					else
					{
						//If there was excessSize left to steal, then missingSize should now be 0.
						//Filter out floating point precision issues.
						missingSize = 0;
					}
				}

				if( missingSize > 1e-6f )
					hasUnresolvedCells = true;

				++itExceeded;
			}

			if( hasUnresolvedCells )
			{
				//We must honour the entire line being <= hardMaxSize
				//So far we only had cells with m_proportion != 0 in exceededCells since the
				//ones w/ m_proportion = 0 had been set to getMinCellSize.
				//If we are here, both proportional and non-proportional cells can no longer
				//be shrunk any further (or if they can, their priority is too high)
				//We need to shrink all cells proportionally to fit inside hardMaxSize
				float currentLineSize = 0;
				for( size_t i=0; i<numCells; ++i )
					currentLineSize += cellSizes[i];

				const float correctionRatio = hardMaxSize[bVertical] / currentLineSize;
				for( size_t i=0; i<numCells; ++i )
					cellSizes[i] *= correctionRatio;
			}
		}

		float accumOffset = 0;

		if( m_evenMarginSpaceAtEdges && !m_cells.front()->m_expand[bVertical] )
			accumOffset += m_cells.front()->m_margin[bVertical] * (0.5f * marginFactor);

		const Ogre::Vector2 layoutTopLeft = m_adjustableWindow ? Ogre::Vector2::ZERO : m_topLeft;

		//Now apply sizes and offsets
		for( size_t i=0; i<numCells; ++i )
		{
			LayoutCell *cell = m_cells[i];

			Ogre::Vector2 finalCellSize = cell->getCellSize();

			Ogre::Vector2 hardCellSize;
			hardCellSize[bVertical]		= cellSizes[i];
			hardCellSize[!bVertical]	= maxOtherSize;

			if( cell->m_expand[bVertical] )
			{
				finalCellSize[bVertical] = cellSizes[i];
			}
			else
			{
				//Cell can't be bigger than calculated in cellSizes[i]
				finalCellSize[bVertical] = std::min( finalCellSize[bVertical], cellSizes[i] );
			}

			const Ogre::Vector2 cellMinSize = cell->getCellMinSize();
			if( cell->m_expand[!bVertical] )
			{
				float otherAvailableSize = maxOtherSize - cellMinSize[!bVertical];
				otherAvailableSize = Ogre::max( otherAvailableSize, 0.0f );

				finalCellSize[!bVertical] = maxOtherSize - Ogre::min( otherAvailableSize,
																	  cell->m_margin[!bVertical] );
			}

			finalCellSize.makeCeil( cellMinSize );

			const Ogre::Vector2 halfMargin = cell->m_margin * (0.5f * marginFactor);

			GridLocations::GridLocations gridLoc =
					m_manager->getSwappedGridLocation( cell->m_gridLocation );

			Ogre::Vector2 topLeft = getTopLeft( bVertical, gridLoc, accumOffset, cellSizes[i],
												maxOtherSize, finalCellSize, halfMargin );

			cell->setCellOffset( layoutTopLeft + topLeft + (layoutMargin * 0.5f) );
			cell->setCellSize( finalCellSize, hardCellSize );

			accumOffset += cellSizes[i];
			accumOffset += cell->m_margin[bVertical] * marginFactor;
		}

		const Ogre::Vector2 oldSize = m_currentSize;
		calculateCurrentSize();
		m_currentSize.makeCeil( oldSize );
		if( m_adjustableWindow )
			syncToWindowSize();

		tellChildrenToUpdateLayout( m_cells );
	}
	//-------------------------------------------------------------------------
	void LayoutLine::notifyLayoutUpdated()
	{
		layout();
	}
	//-------------------------------------------------------------------------
	void LayoutLine::calculateCurrentSize()
	{
		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end  = m_cells.end();

		while( itor != end )
		{
			Ogre::Vector2 cellSize = (*itor)->getCellSize() + (*itor)->m_margin;
			maxedVal.makeCeil( cellSize );
			accumVal += cellSize;
			++itor;
		}

		if( m_evenMarginSpaceAtEdges && !m_cells.empty() )
		{
			if( !m_cells.front()->m_expand[m_vertical] )
				accumVal += m_cells.front()->m_margin * 0.5f;

			if( !m_cells.back()->m_expand[m_vertical] )
				accumVal += m_cells.back()->m_margin * 0.5f;
		}

		m_currentSize = Ogre::Vector2( m_vertical ? maxedVal.x : accumVal.x,
									   m_vertical ? accumVal.y : maxedVal.y );
		m_currentSize.makeFloor( m_hardMaxSize );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutLine::getCellMinSize() const
	{
		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end  = m_cells.end();

		while( itor != end )
		{
			Ogre::Vector2 minCellSize = (*itor)->getCellMinSize();
			maxedVal.makeCeil( minCellSize + (*itor)->m_margin );
			accumVal += minCellSize + (*itor)->m_margin;
			++itor;
		}

		if( m_evenMarginSpaceAtEdges && !m_cells.empty() )
		{
			if( !m_cells.front()->m_expand[m_vertical] )
				accumVal += m_cells.front()->m_margin * 0.5f;

			if( !m_cells.back()->m_expand[m_vertical] )
				accumVal += m_cells.back()->m_margin * 0.5f;
		}

		if( m_adjustableWindow )
		{
			maxedVal += m_adjustableWindow->getBorderCombined();
			accumVal += m_adjustableWindow->getBorderCombined();
		}

		Ogre::Vector2 retVal( m_vertical ? maxedVal.x : accumVal.x,
							  m_vertical ? accumVal.y : maxedVal.y );
		retVal.makeCeil( m_minSize );
		retVal.makeFloor( m_hardMaxSize );
		return retVal;
	}
}
