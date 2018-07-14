
#include "ColibriGui/Layouts/ColibriLayoutLine.h"
#include "ColibriGui/ColibriManager.h"

#define TODO_missing

namespace Colibri
{
	LayoutLine::LayoutLine( ColibriManager *colibriManager ) :
		LayoutBase( colibriManager ),
		m_vertical( true )
	{
	}
	//-------------------------------------------------------------------------
	void LayoutLine::addCell( LayoutCell *cell )
	{
		m_cells.push_back( cell );
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
		const bool bVertical = m_vertical;

		std::vector<float> cellSizes;
		cellSizes.resize( m_cells.size() );
		std::vector<size_t> freeCells;
		freeCells.reserve( m_cells.size() );
		std::vector<size_t> exceededCells;
		exceededCells.reserve( m_cells.size() );

		const Ogre::Vector2 softMaxSize = m_softMaxSize;
		const Ogre::Vector2 hardMaxSize = m_hardMaxSize;

		TODO_missing;
		/*
		 * Border size
		 * Table grids so everyone gets the same rows
		 * Table grids so everyone gets same distance
		*/

		//Sum all proportions
		size_t maxProportion = 0;
		float minMaxSize = 0;		//Vertical / Horizontal size
		float maxOtherSize = 0;		//Horizontal / Vertical size (opposite axis of minMaxSize)
		float nonProportionalSize = 0;

		{
			LayoutCellVec::const_iterator itor = m_cells.begin();
			LayoutCellVec::const_iterator end  = m_cells.end();

			while( itor != end )
			{
				const LayoutCell *cell = *itor;
				maxProportion += cell->m_proportion[bVertical];
				minMaxSize += cell->getCellMinSize()[bVertical];
				const Ogre::Vector2 cellSize = cell->getCellSize();
				maxOtherSize = std::max( maxOtherSize, cellSize[!bVertical] );
				if( !cell->m_proportion[bVertical] )
					nonProportionalSize += cellSize[bVertical];
				++itor;
			}
		}

		//Calculate all cell sizes as if there were no size restrictions
		const size_t numCells = m_cells.size();
		const float maxLineSize = std::min( std::max( softMaxSize[bVertical], minMaxSize ),
											hardMaxSize[bVertical] );
		const float sizeToDistribute = maxLineSize - nonProportionalSize;
		const float invMaxProportion = 1.0f / static_cast<float>( maxProportion );

//		m_size[bVertical] = maxLineSize;
//		m_size[!bVertical] = maxOtherSize;

		for( size_t i=0; i<numCells; ++i )
		{
			LayoutCell *cell = m_cells[i];

			uint16_t proportion = cell->m_proportion[bVertical];

			float cellLineSize;
			if( proportion > 0 )
				cellLineSize = proportion * (sizeToDistribute * invMaxProportion);
			else
			{
				Ogre::Vector2 cellSize = cell->getCellSize();
				cellSize.makeCeil( cell->getCellMinSize() );
				cellLineSize = bVertical ? cellSize.x : cellSize.y;
			}

			float minCellSize = cell->getCellMinSize()[bVertical];

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

				LayoutCell *cell = m_cells[idx];

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
				//ones w/ m_proportion = 0 had been set to max( getCellSize, getMinCellSize ).
				//They may have shrunk though, since they were in freeCells.
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

		//Now apply sizes and offsets
		for( size_t i=0; i<numCells; ++i )
		{
			LayoutCell *cell = m_cells[i];

			Ogre::Vector2 finalCellSize = cell->getCellSize();

			if( cell->m_expand[bVertical] )
			{
				finalCellSize[bVertical] = cellSizes[i];
			}
			else
			{
				//Cell can't be bigger than calculated in cellSizes[i]
				finalCellSize[bVertical] = std::min( finalCellSize[bVertical], cellSizes[i] );
			}

			if( cell->m_expand[!bVertical] )
				finalCellSize[!bVertical] = maxOtherSize;

			Ogre::Vector2 topLeft;
			GridLocations::GridLocations gridLoc =
					m_manager->getSwappedGridLocation( m_gridLocation );
			switch( gridLoc )
			{
			case GridLocations::TopLeft:
			case GridLocations::NumGridLocations:
				topLeft[!bVertical]	= 0;
				topLeft[bVertical]	= accumOffset;
				break;
			case GridLocations::Top:
				topLeft[!bVertical]	= 0;
				topLeft[bVertical]	= accumOffset + (cellSizes[i] - finalCellSize[bVertical]) * 0.5f;
				break;
			case GridLocations::TopRight:
				topLeft[!bVertical]	= 0;
				topLeft[bVertical]	= accumOffset + (cellSizes[i] - finalCellSize[bVertical]);
				break;
			case GridLocations::CenterLeft:
				topLeft[!bVertical]	= (maxOtherSize - finalCellSize[!bVertical]) * 0.5f;
				topLeft[bVertical]	= accumOffset;
				break;
			case GridLocations::Center:
				topLeft[!bVertical]	= (maxOtherSize - finalCellSize[!bVertical]) * 0.5f;
				topLeft[bVertical]	= accumOffset + (cellSizes[i] - finalCellSize[bVertical]) * 0.5f;
				break;
			case GridLocations::CenterRight:
				topLeft[!bVertical]	= (maxOtherSize - finalCellSize[!bVertical]) * 0.5f;
				topLeft[bVertical]	= accumOffset + (cellSizes[i] - finalCellSize[bVertical]);
				break;
			case GridLocations::BottomLeft:
				topLeft[!bVertical]	= maxOtherSize - finalCellSize[!bVertical];
				topLeft[bVertical]	= accumOffset;
				break;
			case GridLocations::Bottom:
				topLeft[!bVertical]	= maxOtherSize - finalCellSize[!bVertical];
				topLeft[bVertical]	= accumOffset + (cellSizes[i] - finalCellSize[bVertical]) * 0.5f;
				break;
			case GridLocations::BottomRight:
				topLeft[!bVertical]	= maxOtherSize - finalCellSize[!bVertical];
				topLeft[bVertical]	= accumOffset + (cellSizes[i] - finalCellSize[bVertical]);
				break;
			}

			cell->setCellOffset( m_topLeft + topLeft );
			cell->setCellSize( finalCellSize );

			accumOffset += cellSizes[i];
		}
	}
	//-------------------------------------------------------------------------
	void LayoutLine::notifyLayoutUpdated()
	{
		layout();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 LayoutLine::getCellSize() const
	{
		Ogre::Vector2 maxedVal( Ogre::Vector2::ZERO );
		Ogre::Vector2 accumVal( Ogre::Vector2::ZERO );

		LayoutCellVec::const_iterator itor = m_cells.begin();
		LayoutCellVec::const_iterator end  = m_cells.end();

		while( itor != end )
		{
			Ogre::Vector2 minCellSize = (*itor)->getCellMinSize();
			Ogre::Vector2 cellSize = (*itor)->getCellSize();
			maxedVal.makeCeil( cellSize );
			accumVal += minCellSize;
			++itor;
		}

		Ogre::Vector2 retVal( m_vertical ? maxedVal.x : accumVal.x,
							  m_vertical ? accumVal.y : maxedVal.y );
		retVal.makeFloor( m_hardMaxSize );
		return retVal;
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
			maxedVal.makeCeil( minCellSize );
			accumVal += minCellSize;
			++itor;
		}

		Ogre::Vector2 retVal( m_vertical ? maxedVal.x : accumVal.x,
							  m_vertical ? accumVal.y : maxedVal.y );
		retVal.makeFloor( m_hardMaxSize );
		return retVal;
	}
}
