
#pragma once

#include "ColibriGui/Layouts/ColibriLayoutBase.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Layout
	@class LayoutLine
		Layout line can be used to specify a set of widgets (cells) in a row, or as a column
		if m_vertical = true.

		In other words it lays out widgets like this:

		@code
			|	A	|	B	|	C	|	D	|
		@endcode

		Or like this:

		@code
			A
			-
			B
			-
			C
			-
			D
		@endcode

		LayoutSpacer can also be used to leave blank space:

		@code
			|	A	|	B	|		|	C	|
		@endcode

		For the sake of documentation, we'll refer to examples as if m_vertical = false; thus
		we'll speak as row and columns, where row = the line.
		If m_vertical were to be true, replace swap the words row and column:

		Multiple LayoutLine can be used to implement a table. However this does not guarantee
		all the columns will have equal width unless all the LayoutCell::m_proportion values in
		each cell from each row are the same, and the minimum size doesn't screw it up either
		because we ran out of space.

		The columns may also be misaligned if you have cells with LayoutCell::m_proportion = 0
		@b unless their sizes are exactly the same or all your cells m_proportion = 0 except
		for blank space which can be used to align the columns.
	@remarks
		Many of the variables are public, because once you're done modifying them all you should
		call LayoutLine::layout()<br/>
		Thus there are few or no setter / getters.

		@see	LayoutCell
	*/
	class LayoutLine : public LayoutBase
	{
		LayoutCellVec	m_cells;
	public:
		/// True to layout all cells as a column
		/// False to layout all cells as a row
		bool m_vertical;

		/** Specifies whether cells at the edges (i.e. left & right or top & bottom)
			are evenly spaced or not.

			There are two ways to lay out cells with their margins:
		@code
			//m_evenMarginSpaceAtEdges = false
				- A -- B -- C -- D -
			//m_evenMarginSpaceAtEdges = true
				-- A -- B -- C -- D --
		@endcode
			Note that when m_evenMarginSpaceAtEdges = false, the distance between A and B is
			(A->m_margin.x + B->m_margin.x) / 2; yet however the distance between A and the
			left edge is only A->m_margin.x / 2

			When m_evenMarginSpaceAtEdges = true, the distance between the left edge and A
			is A->m_margin.x; while the distance between the right edge and D is D->m_margin.x

			LineLayout: This option only works when the first and/or last cell have m_proportion = false
		*/
		bool m_evenMarginSpaceAtEdges;

		/** Returns the top left location for the widget that is inside the cell
		@remarks
			This doc assumes m_vertical = false for explaining variables.
			Else swap the words row and column, and the words width and height.
		@param bVertical
			See LayoutLine::m_vertical
		@param gridLoc
			Alignment in the grid
		@param accumOffset
			Accumulated .x offset so far in the row
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
		inline static Ogre::Vector2 getTopLeft( bool bVertical,
												GridLocations::GridLocations gridLoc,
												float accumOffset, float cellSize,
												float maxOtherSize,
												const Ogre::Vector2 &finalCellSize,
												const Ogre::Vector2 &halfMargin );

		void calculateCurrentSize();

	public:
		LayoutLine( ColibriManager *colibriManager );

		const LayoutCellVec& getCells() const;

		void addCell( LayoutCell *cell );

		/** Moves and resizes all cells to be layed out as either a row or a column.
		@param isRootLayout
			When true, this is the root layout and needs to apply its own margins
			into its children because no one else will
		*/
		void layout();

		virtual void notifyLayoutUpdated() colibri_override;

		virtual Ogre::Vector2 getCellMinSize() const colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
