
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
		If m_vertical where to be true, replace swap the words row and column:

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
	*/
	class LayoutLine : public LayoutBase
	{
		LayoutCellVec	m_cells;
	public:
		/// True to layout all cells as a column
		/// False to layout all cells as a row
		bool m_vertical;

	public:
		LayoutLine( ColibriManager *colibriManager );

		void addCell( LayoutCell *cell );

		/// Moves and resizes all cells to be layed out as either a row or a column.
		void layout();

		virtual void notifyLayoutUpdated() colibri_override;

		virtual Ogre::Vector2 getCellSize() const colibri_override;
		virtual Ogre::Vector2 getCellMinSize() const colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
