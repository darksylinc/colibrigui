
#pragma once

#include "ColibriGui/Layouts/ColibriLayoutBase.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Layout
	@class LayoutTableSameSize
		LayoutTableSameSize can be used to specify order widgets like a table, where
		all cells are equal in size, based on the biggest cell there is.

		LayoutCell::m_proportion will be ignored

		This table works best when all cells are set LayoutCell::m_expand to true, or already
		have the same size and LayoutBase::m_softMaxSize set to zero.

		This Layout was written to serve a very specific need: Multi-column user options.

		Sometimes you want to present your users with options like this:

	@code
		.
			Option A	Option D
			Option B	Option E
			Option C
	@endcode

		This can be easily achieved by any means, except when you get problems like the following:

	@code
		.
			Option A	Option D
			Option B	Very Long Multiline
			Option C		option E
	@endcode

		Therefore this Layout will enlarge all options so they are evenly spaced again:

	@code
		.
				Option A				Option D

				Option B			Very Long Multiline
										option E
				Option C
	@endcode

		That's all there is to it. Do not think too hard of this Layout, it serves a very
		specific need. If it doesn't get it to look the way you want, it's not your fault,
		and use a different one.

	@remarks
		Many of the variables are public, because once you're done modifying them all you should
		call LayoutTableSameSize::layout()<br/>
		Thus there are few or no setter / getters.
	*/
	class LayoutTableSameSize : public LayoutBase
	{
		LayoutCellVec	m_cells;
	public:
		/// When true, a 4x3 table becomes 3x4, with elements tranposed.
		/// i.e. the m_numColumns will still be the same value 4, but actually contains the rows,
		/// and getNumRows will still return 3, but actually contains the columns.
		bool m_transpose;

		/// Number of columns. A value of 0 is not valid and is clamped to 1.
		size_t m_numColumns;

		size_t getNumRows() const;

		/** Returns the top left location for the widget that is inside the cell
		@param gridLoc
			Alignment in the grid
		@param accumOffset
			Accumulated offset so far in the row & column
		@param cellSize
			The calculated width for the cell
		@param maxOtherSize
			The calculated height for the cell
		@param finalCellSize
			The actual width & height for the cell. Must be:<br/>
				<= Vector2( cellSize, maxOtherSize ) for bVertical == false and<br/>
				<= Vector2( maxOtherSize, cellSize ) for bVertical == true
		@param halfMargin
			Half of LayoutCell::m_margin
		@return
		*/
		inline static Ogre::Vector2 getTopLeft( GridLocations::GridLocations gridLoc,
												const Ogre::Vector2 &accumOffset,
												const Ogre::Vector2 &biggestSize,
												const Ogre::Vector2 &finalCellSize,
												const Ogre::Vector2 &halfMargin );

	public:
		LayoutTableSameSize( ColibriManager *colibriManager );

		void addCell( LayoutCell *cell );
		void clearCells();

		/// Moves and resizes all cells to be layed out as either a row or a column.
		void layout();

		virtual void notifyLayoutUpdated() colibri_override;

		virtual Ogre::Vector2 getCellMinSize() const colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
