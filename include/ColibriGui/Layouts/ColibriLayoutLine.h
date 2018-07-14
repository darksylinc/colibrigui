
#pragma once

#include "ColibriGui/Layouts/ColibriLayoutBase.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Layout
	@class LayoutLine
	@remarks
		Many of the variables are public, because once you're done modifying them all you should
		call LayoutLine::layout()
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
