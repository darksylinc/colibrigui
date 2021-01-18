
#pragma once

#include "ColibriGui/Layouts/ColibriLayoutBase.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Layout
	@class LayoutMultiline
		Multiline version of LayoutLine.
		LayoutLine can be used to implement a table, but it does not guarantee all columns will align
		if you're careless.
		LayoutMultiline guarantees all the columns will align

		For that purpose, LayoutMultiline assumes:
			1. The LayoutCell::m_proportion values from the cells in the first row are considered.
			   All other rows' m_proportion are ignored.
	@remarks
		Many of the variables are public, because once you're done modifying them all you should
		call LayoutMultiline::layout()<br/>
		Thus there are few or no setter / getters.

		@see	LayoutCell
	*/
	class LayoutMultiline : public LayoutBase
	{
		LayoutCellVec	m_cells;
	public:
		/// True to layout all cells in the first line as a column
		/// There will 'm_numLines' columns.
		/// Calling addCell consecutively and asuming B is bigger than the rest and m_numLines = 2 then:
		///		A D
		///		B E
		///		B
		///		C F
		///
		/// Thus m_proportion[1] of each cell in the first column can be honoured but m_proportion[0]
		/// cannot (see LayoutMultiline::m_expandToCoverSoftMaxSize).
		///
		/// False to layout all cells in the first line as a row
		/// There will 'm_numLines' rows.
		/// Calling addCell consecutively and asuming B is bigger than the rest and m_numLines = 2 then:
		///		A B B C
		///		D E   F
		///
		/// Thus m_proportion[0] of each cell in the first row can be honoured but m_proportion[1]
		/// cannot (see LayoutMultiline::m_expandToCoverSoftMaxSize).
		///
		/// If in doubt, imagine what would happen to a single line in LayoutLine::m_vertical
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

			LineLayout: This option only works well when the first and last cell
			have m_proportion = 0 or m_expand = false
		*/
		bool m_evenMarginSpaceAtEdges;

		/** When m_vertical = false, the value of LayoutCell::m_proportional[1] cannot be honoured.
			Likewise when m_vertical = true, LayoutCell::m_proportional[0] cannot be honoured.

			When m_expandToCoverSoftSize = false, all lines are layed out consecutively as
			tight as possible (i.e. as if m_proportional[!m_vertical] = 0).

			When m_expandToCoverSoftSize = true, all lines are spaced out evenly to cover all of
			m_softMaxSize (i.e. as if m_proportional[!m_vertical] = 1)
		*/
		bool m_expandToCoverSoftMaxSize;

		size_t m_numLines;

		/** Returns the top left location for the widget that is inside the cell
		@remarks
			This doc assumes m_vertical = false for explaining variables.
			Else swap the words row and column, and the words width and height.
		@param bVertical
			See LayoutMultiline::m_vertical
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

		Ogre::Vector2 getBiggestMargin( size_t columnIdx ) const;
		void calculateCurrentSize();

	public:
		LayoutMultiline( ColibriManager *colibriManager );

		const LayoutCellVec& getCells() const;

		void addCell( LayoutCell *cell );
		void clearCells();

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
