
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"
#include "OgreVector2.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Layout
	@class LayoutCell
		Base class that can be used by the Layout system to place and size cells around.
		Derived class must implement several functions that will be called by the Layout system.

		Layout cells are used by other layout systems like LayoutLine.

	@remarks
		Many of the variables are public, because once you're done modifying them all you should
		call (e.g.) LayoutLine::layout()

		Many of the variables are arrays two, i.e. m_proportion[2]. One entry for each axis.
		i.e. m_proportion[0] refers to .x (horizontal) and m_proportion[1] to .y (vertical)
	*/
	class LayoutCell
	{
	public:
		LayoutCell();

		/// The layout system works by proportionally distributing the
		/// available width and or height to each cell using a weighted blend
		/// based on m_proportion value.
		///
		/// Example: LayoutLine is used, with horizontal layout,
		/// thus all cells are layed out in a row
		///
		///		* We have 4 cells and the maximum width is 1000
		///		* If all cells in an entry have m_proportion = 1, then the each cell
		///		  will be width = 250
		///		* If one of those cells had m_proportion = 2 instead, then 3 cells
		///		  would have width = 200 and 1 cell would have width = 400
		///		  This is because the total proportion is 5 (1+1+1+2=5) and
		///		  then the proportions would be 1/5 1/5 1/5 2/5
		///		  Thus it's a weighted blend
		///
		/// @see	LayoutCell::m_priority
		uint16_t	m_proportion[2];

		/// The priority for this cell. 0 = lowest priority; 255 = highest priority
		/// This value only makes sense when the available space is not enough
		/// to fit all cells.
		///
		/// When that happens, some cells may have free space (because the space prepared
		/// for them is bigger than LayoutCell::getCellMinSize) while others are lacking it.
		///
		/// Cells that need space will begin "stealing" space from other cells that can
		/// shrink. This will cause misalignments, but the contents should be readable.
		///
		/// A cell is only allowed to steal from other cells of equal or lower priority.
		uint8_t		m_priority;

		/// When true, the size of the cell will expanded to fit the entire cell.
		/// Otherwise, it will be equal to the cell size or smaller
		bool		m_expand[2];

		/** When these conditions are met:
				1. m_proportion = false; m_expand = false and LayoutLine::m_vertical = true
				2. m_proportion = true; m_expand = true and LayoutLine::m_vertical = false

			then the following work:
				1. *Left
				2. *Center
				3. *Right

			When these conditions are met:
				1. m_proportion = false; m_expand = false and LayoutLine::m_vertical = false
				2. m_proportion = true; m_expand = true and LayoutLine::m_vertical = true

			then the following work:
				1. Top*
				2. Center*
				3. Bottom*
		*/
		GridLocations::GridLocations	m_gridLocation;

		/// Empty space between each cell. In virtual canvas units.
		Ogre::Vector2	m_margin;

		virtual void notifyLayoutUpdated() {}

		virtual void setCellOffset( const Ogre::Vector2 &topLeft ) = 0;
		virtual void setCellSize( const Ogre::Vector2 &size ) = 0;
		virtual Ogre::Vector2 getCellSize() const = 0;
		virtual Ogre::Vector2 getCellMinSize() const = 0;
	};

	typedef std::vector<LayoutCell*> LayoutCellVec;

	/** @ingroup Layout
	@class LayoutSpacer
		Class whose only purpose is to leave blank space between other cells
		with proportions in mind
	*/
	class LayoutSpacer : public LayoutCell
	{
	public:
		static LayoutSpacer c_DefaultBlankSpacer;

		LayoutSpacer();

		void setCellOffset( const Ogre::Vector2 &topLeft ) colibri_final;
		void setCellSize( const Ogre::Vector2 &size ) colibri_final;
		Ogre::Vector2 getCellSize() const colibri_final;
		Ogre::Vector2 getCellMinSize() const colibri_final;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
