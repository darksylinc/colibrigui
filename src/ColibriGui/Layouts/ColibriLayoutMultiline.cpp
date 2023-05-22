
#include "ColibriGui/Layouts/ColibriLayoutMultiline.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriWindow.h"

namespace Colibri
{
	LayoutMultiline::LayoutMultiline( ColibriManager *colibriManager ) :
		LayoutBase( colibriManager ),
		m_vertical( true ),
		m_evenMarginSpaceAtEdges( true ),
		m_expandToCoverSoftMaxSize( false ),
		m_numLines( 1u )
	{
	}
	//-------------------------------------------------------------------------
	const LayoutCellVec& LayoutMultiline::getCells() const
	{
		return m_cells;
	}
	//-------------------------------------------------------------------------
	void LayoutMultiline::addCell( LayoutCell *cell )
	{
		m_cells.push_back( cell );
	}
	//-------------------------------------------------------------------------
	void LayoutMultiline::clearCells() { m_cells.clear(); }
	//-------------------------------------------------------------------------
	inline Ogre::Vector2 LayoutMultiline::getTopLeft( bool bVertical,
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
	void LayoutMultiline::layout()
	{
		if( m_cells.empty() )
			return;

		syncFromWindowSize();

		const size_t numLines = std::max<size_t>( m_numLines, 1u );
		const Ogre::Real fNumLines = Ogre::Real( numLines );
		const size_t numCellsPerLine = Ogre::alignToNextMultiple( m_cells.size(), numLines ) / numLines;

		if( m_cells.size() % numLines )
		{
			LogListener *logListener = m_manager->getLogListener();
			logListener->log( "LayoutMultiline::layout: For best results m_cells.size() "
							  "should be multiple of m_numLines!",
							  LogSeverity::Warning );
		}

		const bool bVertical = m_vertical;

		std::vector<float> cellSizes;
		cellSizes.resize( numCellsPerLine, 0.0f );
		std::vector<size_t> freeCells;
		freeCells.reserve( numCellsPerLine );
		std::vector<size_t> exceededCells;
		exceededCells.reserve( numCellsPerLine );

		std::vector<Ogre::Vector2> minCellSizes;
		minCellSizes.reserve( numCellsPerLine );
		std::vector<Ogre::Vector2> stdCellSizes;
		stdCellSizes.reserve( numCellsPerLine );
		std::vector<float> cellMargins;
		cellMargins.reserve( numCellsPerLine );

		const Ogre::Vector2 layoutMargin = m_adjustableWindow ? m_margin : Ogre::Vector2::ZERO;

		//Sum all proportions
		size_t maxProportion = 0;
		float sizeToDistribute = 0;		//Vertical / Horizontal size
		float maxOtherSize = layoutMargin[!m_vertical];	//Horizontal / Vertical size
														//(opposite axis of minMaxSize)
		float nonProportionalSize = 0;
		float accumMarginSize = layoutMargin[m_vertical];

		{
			LayoutCellVec::const_iterator itor = m_cells.begin();
			LayoutCellVec::const_iterator endt = m_cells.begin() + ptrdiff_t( numCellsPerLine );

			while( itor != endt )
			{
				const LayoutCell *cell0 = *itor;
				maxProportion += cell0->m_proportion[bVertical];

				Ogre::Vector2 biggestMinSizeInColumn = cell0->getCellMinSize();
				Ogre::Vector2 biggestMargin = cell0->m_margin;

				for( size_t line = 1u; line < numLines; ++line )
				{
					if( itor + ptrdiff_t( line * numCellsPerLine ) >= m_cells.end() )
						break;

					const LayoutCell *cell = *( itor + ptrdiff_t( line * numCellsPerLine ) );

					biggestMinSizeInColumn.makeCeil( cell->getCellMinSize() );
					biggestMargin.makeCeil( cell->m_margin );
				}

				maxOtherSize = std::max( maxOtherSize, biggestMinSizeInColumn[!bVertical] +
													   biggestMargin[!bVertical] );
				if( !cell0->m_proportion[bVertical] )
					nonProportionalSize += biggestMinSizeInColumn[bVertical];
				else
					sizeToDistribute += biggestMinSizeInColumn[bVertical];
				accumMarginSize += biggestMargin[bVertical];

				minCellSizes.push_back( biggestMinSizeInColumn );
				cellMargins.push_back( biggestMargin[bVertical] );

				++itor;
			}

			if( m_evenMarginSpaceAtEdges )
			{
				if( !m_cells.front()->m_expand[bVertical] )
					accumMarginSize += cellMargins.front() * 0.5f;

				const LayoutCell *cell = *( m_cells.begin() + ptrdiff_t( numCellsPerLine - 1u ) );
				if( !cell->m_expand[bVertical] )
					accumMarginSize += cellMargins.back() * 0.5f;
			}
		}

		Ogre::Vector2 adjWindowBorders( Ogre::Vector2::ZERO );
		if( m_adjustableWindow )
			adjWindowBorders = m_adjustableWindow->getBorderCombined();

		m_currentSize.makeCeil( m_minSize );

		//Calculate all cell sizes as if there were no size restrictions
		const bool canScroll = m_adjustableWindow != 0 && !m_preventScrolling;
		const Ogre::Vector2 softMaxSize = m_currentSize - layoutMargin;
		const Ogre::Vector2 hardMaxSize = m_hardMaxSize - adjWindowBorders - layoutMargin;
		sizeToDistribute = std::max( softMaxSize[bVertical] - accumMarginSize - nonProportionalSize,
									 sizeToDistribute );
		if( !canScroll )
		{
			sizeToDistribute = std::min( sizeToDistribute,
										 hardMaxSize[bVertical] - nonProportionalSize );
		}
		sizeToDistribute = std::max( sizeToDistribute, 0.0f );
		const float invMaxProportion = 1.0f / static_cast<float>( maxProportion );
		if( m_expandToCoverSoftMaxSize )
			maxOtherSize = std::max( maxOtherSize, softMaxSize[!bVertical] / fNumLines );
		float nonProportionalFactor = 1.0f;
		if( !canScroll )
		{
			maxOtherSize = std::min( maxOtherSize, hardMaxSize[!bVertical] / fNumLines );
			//If nonProportionalSize is bigger than hardMaxSize, widgets just don't fit.
			//Make them all proportionally smaller.
			nonProportionalFactor = std::min( hardMaxSize[bVertical] / nonProportionalSize, 1.0f );
		}

		// Protect against users using negative margin
		accumMarginSize = std::max( accumMarginSize, 0.0f );

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
		for( size_t i=0; i<numCellsPerLine; ++i )
		{
			const LayoutCell *cell = m_cells[i];

			const uint16_t proportion = cell->m_proportion[bVertical];

			const float minCellSize = minCellSizes[i][bVertical];
			if( proportion > 0 )
			{
				float cellLineSize = proportion * (sizeToDistribute * invMaxProportion);

				//Push this cell as being able to shrink if needed
				if( cellLineSize >= minCellSize )
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

				const float minCellSize = minCellSizes[idx][bVertical];

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
				for( size_t i=0; i<numCellsPerLine; ++i )
					currentLineSize += cellSizes[i];

				const float correctionRatio = hardMaxSize[bVertical] / currentLineSize;
				for( size_t i=0; i<numCellsPerLine; ++i )
					cellSizes[i] *= correctionRatio;
			}
		}

		const size_t numCells = m_cells.size();
		const Ogre::Vector2 layoutTopLeft = m_adjustableWindow ? Ogre::Vector2::ZERO : m_topLeft;

		//Now apply sizes and offsets
		for( size_t y = 0; y < numLines; ++y )
		{
			float accumOffset = 0;

			if( m_evenMarginSpaceAtEdges && !m_cells.front()->m_expand[bVertical] )
				accumOffset += cellMargins.front() * (0.5f * marginFactor);

			for( size_t x=0; x<numCellsPerLine; ++x )
			{
				const size_t idx = y * numCellsPerLine + x;
				if( y * numCellsPerLine + x >= numCells )
					break;

				LayoutCell *cell = m_cells[idx];

				Ogre::Vector2 finalCellSize =  cell->getCellSize();

				Ogre::Vector2 hardCellSize;
				hardCellSize[bVertical]		= cellSizes[x];
				hardCellSize[!bVertical]	= maxOtherSize;

				if( cell->m_expand[bVertical] )
				{
					finalCellSize[bVertical] = cellSizes[x];
				}
				else
				{
					//Cell can't be bigger than calculated in cellSizes[i]
					finalCellSize[bVertical] = std::min( finalCellSize[bVertical], cellSizes[x] );
				}

				const Ogre::Vector2 cellMinSize = cell->getCellMinSize();
				if( cell->m_expand[!bVertical] )
				{
					float otherAvailableSize = maxOtherSize - cellMinSize[!bVertical];
					otherAvailableSize = std::max( otherAvailableSize, 0.0f );

					finalCellSize[!bVertical] = maxOtherSize - std::min( otherAvailableSize,
																		 cell->m_margin[!bVertical] );
				}

				finalCellSize.makeCeil( cellMinSize );

				const Ogre::Vector2 halfMargin = cell->m_margin * (0.5f * marginFactor);

				GridLocations::GridLocations gridLoc =
						m_manager->getSwappedGridLocation( cell->m_gridLocation );

				Ogre::Vector2 topLeft = getTopLeft( bVertical, gridLoc, accumOffset, cellSizes[x],
													maxOtherSize, finalCellSize, halfMargin );

				topLeft[!bVertical] += Ogre::Real( y ) * maxOtherSize;

				cell->setCellOffset( layoutTopLeft + topLeft + (layoutMargin * 0.5f) );
				cell->setCellSize( finalCellSize, hardCellSize );

				accumOffset += cellSizes[x];
				accumOffset += cellMargins[x] * marginFactor;
			}
		}

		const Ogre::Vector2 oldSize = m_currentSize;
		calculateCurrentSize();
		m_currentSize.makeCeil( oldSize );
		if( m_adjustableWindow )
			syncToWindowSize();

		tellChildrenToUpdateLayout( m_cells );

		if( m_adjustableWindow )
			m_adjustableWindow->sizeScrollToFit();
	}
	//-------------------------------------------------------------------------
	void LayoutMultiline::notifyLayoutUpdated()
	{
		layout();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutMultiline::getBiggestMargin( size_t columnIdx ) const
	{
		Ogre::Vector2 biggestMargin( Ogre::Vector2::ZERO );

		columnIdx = std::min( columnIdx - 1u, m_cells.size() );

		if( columnIdx < m_cells.size() )
		{
			const size_t numLines = std::max<size_t>( m_numLines, 1u );
			const size_t numCellsPerLine = Ogre::alignToNextMultiple( m_cells.size(), numLines ) /
										   numLines;

			LayoutCellVec::const_iterator itor = m_cells.begin() + ptrdiff_t( columnIdx );
			for( size_t line = 0u; line < numLines; ++line )
			{
				if( itor + ptrdiff_t( line * numCellsPerLine ) >= m_cells.end() )
					break;
				const LayoutCell *cell = *( itor + ptrdiff_t( line * numCellsPerLine ) );
				biggestMargin.makeCeil( cell->m_margin );
			}
		}

		return biggestMargin;
	}
	//-------------------------------------------------------------------------
	void LayoutMultiline::calculateCurrentSize()
	{
		const size_t numLines = std::max<size_t>( m_numLines, 1u );
		const size_t numCellsPerLine = Ogre::alignToNextMultiple( m_cells.size(), numLines ) / numLines;

		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		//Calculate the size of a single line based on the biggest line
		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator endt = m_cells.begin() + ptrdiff_t( numCellsPerLine );

		while( itor != endt )
		{
			const LayoutCell *cell0 = *itor;

			Ogre::Vector2 biggestCellSize = cell0->getCellSize() + cell0->m_margin;

			for( size_t line=1u; line<numLines; ++line )
			{
				if( itor + ptrdiff_t( line * numCellsPerLine ) >= m_cells.end() )
					break;

				const LayoutCell *cell = *( itor + ptrdiff_t( line * numCellsPerLine ) );

				const Ogre::Vector2 cellSize = cell->getCellSize() + cell->m_margin;
				biggestCellSize.makeCeil( cellSize );
			}

			accumVal += biggestCellSize;
			maxedVal.makeCeil( biggestCellSize );

			++itor;
		}

		if( m_evenMarginSpaceAtEdges && !m_cells.empty() )
		{
			if( !m_cells.front()->m_expand[m_vertical] )
			{
				Ogre::Vector2 biggestMargin = getBiggestMargin( 0 );
				accumVal += biggestMargin;
			}

			const LayoutCell *cell = *( m_cells.begin() + ptrdiff_t( numCellsPerLine - 1u ) );
			if( !cell->m_expand[m_vertical] )
			{
				Ogre::Vector2 biggestMargin = getBiggestMargin( numCellsPerLine - 1u );
				accumVal += biggestMargin;
			}
		}

		const Ogre::Vector2 layoutMargin = m_adjustableWindow ? m_margin : Ogre::Vector2::ZERO;
		const Ogre::Real fNumLines = Ogre::Real( numLines );

		if( m_expandToCoverSoftMaxSize )
		{
			const Ogre::Vector2 softMaxSize = m_currentSize - layoutMargin;
			maxedVal[!m_vertical] =
				std::max( maxedVal[!m_vertical], softMaxSize[!m_vertical] / fNumLines );
		}

		m_currentSize = Ogre::Vector2( m_vertical ? ( maxedVal.x * fNumLines ) : accumVal.x,
									   m_vertical ? accumVal.y : ( maxedVal.y * fNumLines ) );
		m_currentSize += layoutMargin;
		m_currentSize.makeCeil( m_minSize );
		m_currentSize.makeFloor( m_hardMaxSize );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutMultiline::getCellMinSize() const
	{
		const size_t numLines = std::max<size_t>( m_numLines, 1u );
		const size_t numCellsPerLine = Ogre::alignToNextMultiple( m_cells.size(), numLines ) / numLines;

		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end = m_cells.begin() + ptrdiff_t( numCellsPerLine );

		while( itor != end )
		{
			const LayoutCell *cell0 = *itor;

			Ogre::Vector2 biggestMinSizeInColumn = cell0->getCellMinSize();
			Ogre::Vector2 biggestMarginInColumn = cell0->m_margin;

			for( size_t line=1u; line<numLines; ++line )
			{
				if( itor + ptrdiff_t( line * numCellsPerLine ) >= m_cells.end() )
					break;

				const LayoutCell *cell = *( itor + ptrdiff_t( line * numCellsPerLine ) );

				biggestMinSizeInColumn.makeCeil( cell->getCellMinSize() );
				biggestMarginInColumn.makeCeil( cell->m_margin );
			}

			maxedVal.makeCeil( biggestMinSizeInColumn + biggestMarginInColumn );
			accumVal += biggestMinSizeInColumn + biggestMarginInColumn;

			++itor;
		}

		const Ogre::Vector2 layoutMargin = m_adjustableWindow ? m_margin : Ogre::Vector2::ZERO;
		const Ogre::Real fNumLines = Ogre::Real( numLines );

		if( m_expandToCoverSoftMaxSize )
		{
			const Ogre::Vector2 softMaxSize = m_currentSize - layoutMargin;
			maxedVal[!m_vertical] =
				std::max( maxedVal[!m_vertical], softMaxSize[!m_vertical] / fNumLines );
		}

		if( m_evenMarginSpaceAtEdges && !m_cells.empty() )
		{
			if( !m_cells.front()->m_expand[m_vertical] )
			{
				Ogre::Vector2 biggestMargin = getBiggestMargin( 0 );
				accumVal += biggestMargin;
			}

			const LayoutCell *cell = *( m_cells.begin() + ptrdiff_t( numCellsPerLine - 1u ) );
			if( !cell->m_expand[m_vertical] )
			{
				Ogre::Vector2 biggestMargin = getBiggestMargin( numCellsPerLine - 1u );
				accumVal += biggestMargin;
			}
		}


		if( m_adjustableWindow )
		{
			maxedVal += m_adjustableWindow->getBorderCombined();
			accumVal += m_adjustableWindow->getBorderCombined();
		}

		Ogre::Vector2 retVal( m_vertical ? ( maxedVal.x * fNumLines ) : accumVal.x,
							  m_vertical ? accumVal.y : ( maxedVal.y * fNumLines ) );
		retVal += layoutMargin;
		retVal.makeCeil( m_minSize );
		retVal.makeFloor( m_hardMaxSize );
		return retVal;
	}
}
