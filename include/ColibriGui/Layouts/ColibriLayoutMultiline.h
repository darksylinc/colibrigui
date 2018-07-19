
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

	public:
		LayoutMultiline( ColibriManager *colibriManager );

		void addCell( LayoutCell *cell );

		/// Moves and resizes all cells to be layed out as either a row or a column.
		void layout();

		virtual void notifyLayoutUpdated() colibri_override;

		virtual Ogre::Vector2 getCellSize() const colibri_override;
		virtual Ogre::Vector2 getCellMinSize() const colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
