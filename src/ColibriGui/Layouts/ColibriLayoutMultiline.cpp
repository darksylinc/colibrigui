
#include "ColibriGui/Layouts/ColibriLayoutMultiline.h"
#include "ColibriGui/ColibriManager.h"

namespace Colibri
{
	LayoutMultiline::LayoutMultiline( ColibriManager *colibriManager ) :
		LayoutBase( colibriManager ),
		m_vertical( true ),
		m_evenMarginSpaceAtEdges( true ),
		m_numLines( 1u )
	{
	}
	//-------------------------------------------------------------------------
	void LayoutMultiline::addCell( LayoutCell *cell )
	{
		m_cells.push_back( cell );
	}
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

		const size_t numLines = std::max<size_t>( m_numLines, 1u );
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
		minCellSizes.reserve( numCellsPerLine );
		std::vector<float> cellMargins;
		minCellSizes.reserve( numCellsPerLine );

		const Ogre::Vector2 softMaxSize = m_softMaxSize;
		const Ogre::Vector2 hardMaxSize = m_hardMaxSize;

		//Sum all proportions
		size_t maxProportion = 0;
		float minMaxSize = 0;		//Vertical / Horizontal size
		float maxOtherSize = 0;		//Horizontal / Vertical size (opposite axis of minMaxSize)
		float nonProportionalSize = 0;
		float accumMarginSize = 0;

		{
			LayoutCellVec::const_iterator itor = m_cells.begin();
			LayoutCellVec::const_iterator end  = m_cells.begin() + numCellsPerLine;

			while( itor != end )
			{
				const LayoutCell *cell0 = *itor;
				maxProportion += cell0->m_proportion[bVertical];

				Ogre::Vector2 biggestMinSizeInColumn = cell0->getCellMinSize();
				Ogre::Vector2 biggestCellSize = cell0->getCellSize();
				Ogre::Vector2 biggestMargin = cell0->m_margin;

				for( size_t line=1u; line<numLines; ++line )
				{
					if( itor + line * numCellsPerLine >= m_cells.end() )
						break;

					const LayoutCell *cell = *(itor + line * numCellsPerLine);

					biggestMinSizeInColumn.makeCeil( cell->getCellMinSize() );
					const Ogre::Vector2 cellSize = cell->getCellSize();
					biggestCellSize.makeCeil( cellSize );
					biggestMargin.makeCeil( cell->m_margin );
				}

				minMaxSize += biggestMinSizeInColumn[bVertical];

				maxOtherSize = Ogre::max( maxOtherSize,
										  biggestCellSize[!bVertical] + biggestMargin[!bVertical] );
				if( !cell0->m_proportion[bVertical] )
					nonProportionalSize += biggestCellSize[bVertical];
				accumMarginSize += biggestMargin[bVertical];

				minCellSizes.push_back( biggestMinSizeInColumn );
				stdCellSizes.push_back( biggestCellSize );
				cellMargins.push_back( biggestMargin[bVertical] );

				++itor;
			}

			if( m_evenMarginSpaceAtEdges )
			{
				if( !m_cells.front()->m_expand[bVertical] )
					accumMarginSize += cellMargins.front() * 0.5f;

				const LayoutCell *cell = *(m_cells.begin() + numCellsPerLine - 1u);
				if( !cell->m_expand[bVertical] )
					accumMarginSize += cellMargins.back() * 0.5f;
			}
		}

		//Calculate all cell sizes as if there were no size restrictions
		const float maxLineSize = std::min( std::max( softMaxSize[bVertical] - accumMarginSize,
													  minMaxSize ),
											hardMaxSize[bVertical] );
		const float sizeToDistribute = Ogre::max( maxLineSize - nonProportionalSize, 0.0f );
		const float invMaxProportion = 1.0f / static_cast<float>( maxProportion );
		maxOtherSize = Ogre::min( Ogre::max( maxOtherSize, softMaxSize[!bVertical] ),
								  hardMaxSize[!bVertical] );

		const float spaceLeftForMargins =
				fabsf( maxLineSize - std::min( hardMaxSize[bVertical],
											   (maxLineSize + accumMarginSize) ) );
		//marginFactor will be in range [0; 1] because
		//spaceLeftForMargins is in range [0; accumMarginSize]
		const float marginFactor =
				accumMarginSize > 1e-6f ? (spaceLeftForMargins / accumMarginSize) : 1.0f;

//		m_size[bVertical] = maxLineSize;
//		m_size[!bVertical] = maxOtherSize;

		for( size_t i=0; i<numCellsPerLine; ++i )
		{
			const LayoutCell *cell = m_cells[i];

			uint16_t proportion = cell->m_proportion[bVertical];

			float cellLineSize;
			if( proportion > 0 )
				cellLineSize = proportion * (sizeToDistribute * invMaxProportion);
			else
				cellLineSize = std::max( stdCellSizes[i][bVertical], minCellSizes[i][bVertical] );

			float minCellSize = minCellSizes[i][bVertical];

			//Push this cell as being able to shrink if needed
			if( cellLineSize > minCellSize )
				freeCells.push_back( i );
			else
				exceededCells.push_back( i );

			cellSizes[i] = cellLineSize;
		}

		if( !exceededCells.empty() )
		{
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
				//ones w/ m_proportion = 0 had been set to max( getCellSize, getMinCellSize ).
				//They may have shrunk though, since they were in freeCells.
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

		//Now apply sizes and offsets
		for( size_t y=0; y<numLines; ++y )
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

				if( cell->m_expand[bVertical] )
				{
					finalCellSize[bVertical] = cellSizes[x];
				}
				else
				{
					//Cell can't be bigger than calculated in cellSizes[i]
					finalCellSize[bVertical] = std::min( finalCellSize[bVertical], cellSizes[x] );
				}

				if( cell->m_expand[!bVertical] )
				{
					const Ogre::Vector2 cellMinSize = cell->getCellMinSize();
					float otherAvailableSize = maxOtherSize - cellMinSize[!bVertical];
					otherAvailableSize = Ogre::max( otherAvailableSize, 0.0f );

					finalCellSize[!bVertical] = maxOtherSize - Ogre::min( otherAvailableSize,
																		  cell->m_margin[!bVertical] );
				}

				const Ogre::Vector2 halfMargin = cell->m_margin * (0.5f * marginFactor);

				GridLocations::GridLocations gridLoc =
						m_manager->getSwappedGridLocation( cell->m_gridLocation );

				Ogre::Vector2 topLeft = getTopLeft( bVertical, gridLoc, accumOffset, cellSizes[x],
													maxOtherSize, finalCellSize, halfMargin );

				topLeft[!bVertical] += y * maxOtherSize;

				cell->setCellOffset( m_topLeft + topLeft );
				cell->setCellSize( finalCellSize );

				accumOffset += cellSizes[x];
				accumOffset += cellMargins[x] * marginFactor;
			}
		}

		tellChildrenToUpdateLayout( m_cells );
	}
	//-------------------------------------------------------------------------
	void LayoutMultiline::notifyLayoutUpdated()
	{
		layout();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutMultiline::getCellSize() const
	{
		const size_t numLines = std::max<size_t>( m_numLines, 1u );
		const size_t numCellsPerLine = Ogre::alignToNextMultiple( m_cells.size(), numLines ) / numLines;

		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end  = m_cells.begin() + numCellsPerLine;

		while( itor != end )
		{
			const LayoutCell *cell0 = *itor;

			Ogre::Vector2 biggestMinSizeInColumn = cell0->getCellMinSize();
			Ogre::Vector2 biggestCellSize = cell0->getCellSize();
			Ogre::Vector2 biggestMargin = cell0->m_margin;

			for( size_t line=1u; line<numLines; ++line )
			{
				if( itor + line * numCellsPerLine >= m_cells.end() )
					break;

				const LayoutCell *cell = *(itor + line * numCellsPerLine);

				biggestMinSizeInColumn.makeCeil( cell->getCellMinSize() );
				const Ogre::Vector2 cellSize = cell->getCellSize();
				biggestCellSize.makeCeil( cellSize );
				biggestMargin.makeCeil( cell->m_margin );
			}

			accumVal += biggestMinSizeInColumn + biggestMargin;
			maxedVal.makeCeil( biggestCellSize + biggestMargin );

			++itor;
		}

		if( m_evenMarginSpaceAtEdges && !m_cells.empty() )
		{
			if( !m_cells.front()->m_expand[m_vertical] )
			{
				itor = m_cells.begin();
				Ogre::Vector2 biggestMargin = Ogre::Vector2::ZERO;
				for( size_t line=0u; line<numLines; ++line )
				{
					if( itor + line * numCellsPerLine >= m_cells.end() )
						break;
					const LayoutCell *cell = *(itor + line * numCellsPerLine);
					biggestMargin.makeCeil( cell->m_margin );
				}
				accumVal += biggestMargin;
			}

			const LayoutCell *cell = *(m_cells.begin() + numCellsPerLine - 1u);
			if( !cell->m_expand[m_vertical] )
			{
				itor = m_cells.begin() + numCellsPerLine - 1u;
				Ogre::Vector2 biggestMargin = Ogre::Vector2::ZERO;
				for( size_t line=0u; line<numLines; ++line )
				{
					if( itor + line * numCellsPerLine >= m_cells.end() )
						break;
					const LayoutCell *cell = *(itor + line * numCellsPerLine);
					biggestMargin.makeCeil( cell->m_margin );
				}
				accumVal += biggestMargin;
			}
		}

		Ogre::Vector2 retVal( m_vertical ? maxedVal.x : accumVal.x,
							  m_vertical ? accumVal.y : maxedVal.y );
		retVal.makeFloor( m_hardMaxSize );
		return retVal;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutMultiline::getCellMinSize() const
	{
		const size_t numLines = std::max<size_t>( m_numLines, 1u );
		const size_t numCellsPerLine = Ogre::alignToNextMultiple( m_cells.size(), numLines ) / numLines;

		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end  = m_cells.begin() + numCellsPerLine;

		while( itor != end )
		{
			const LayoutCell *cell0 = *itor;

			Ogre::Vector2 biggestMinSizeInColumn = cell0->getCellMinSize();

			for( size_t line=1u; line<numLines; ++line )
			{
				if( itor + line * numCellsPerLine >= m_cells.end() )
					break;

				const LayoutCell *cell = *(itor + line * numCellsPerLine);

				biggestMinSizeInColumn.makeCeil( cell->getCellMinSize() );
			}

			accumVal += biggestMinSizeInColumn;
			maxedVal.makeCeil( biggestMinSizeInColumn );

			++itor;
		}

		Ogre::Vector2 retVal( m_vertical ? maxedVal.x : accumVal.x,
							  m_vertical ? accumVal.y : maxedVal.y );
		retVal.makeFloor( m_hardMaxSize );
		return retVal;
	}
}
