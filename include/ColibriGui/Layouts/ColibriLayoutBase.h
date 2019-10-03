
#pragma once

#include "ColibriGui/Layouts/ColibriLayoutCell.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Layout
	@class LayoutBase
		Base class for many layout implementations such as LayoutLine
	@remarks
		Many of the variables are public, because once you're done modifying them all you should
		call layout()
	*/
	class LayoutBase : public LayoutCell
	{
	protected:
		ColibriManager	*m_manager;

		/** Current size.

			For the root layout, cells inside this layout are allowed to grow m_currentSize
			until m_hardMaxSize is reached, but m_currentSize will never shrink.

			A parent layout may shrink m_currentSize of their children until m_minSize is reached
			if constraints deem this necessary.
		*/
		Ogre::Vector2	m_currentSize;

		/** When not null, it will modify the window's client size to fit the objects,
			and modify its scroll area if m_hardMaxSize was exceeded unless m_preventScrolling
			is true.
		*/
		Widget * colibrigui_nullable m_adjustableWindow;

	public:
		bool m_preventScrolling;

	protected:
		void tellChildrenToUpdateLayout( const LayoutCellVec &childrenCells );

		void syncFromWindowSize();
		void syncToWindowSize();

	public:
		Ogre::Vector2	m_topLeft;

		/** Maximum allowed size. Rows/columns are not allowed to exceed this size.

			If all objects combined exceed this size, then cells that are below their min
			size start stealing from other cells who have empty space.

			If the cells are still too big after all empty space has been stolen, then the
			widgets can't be rendered correctly and minimum cell sizes won't be respected.
			The cells simply do not fit the restricted space.

			@see	LayoutBase::m_minSize
			@see	LayoutCell::m_priority
			@see	LayoutCell::getCellSize
			@see	LayoutCell::getCellMinSize
		@remarks
			Layouts which have an m_adjustableWindow are considered scrollable, and thus
			will layout widgets beyond m_hardMaxSize, but the window itself won't get bigger
			than this size.
		*/
		Ogre::Vector2	m_hardMaxSize;

	public:
		LayoutBase( ColibriManager *colibriManager );

		void setAdjustableWindow( Widget * colibrigui_nullable window );
		Widget * colibrigui_nullable getAdjustableWindow() const;

		/// Utility function to batch apply margins to all cells in a list. Not recursive.
		static void setMarginToAllCells( const LayoutCellVec &cells, const Ogre::Vector2 &margin );

		void setCellOffset( const Ogre::Vector2 &topLeft ) colibri_final;
		void setCellSize( const Ogre::Vector2 &size ) colibri_final;
		virtual void setCellSize( const Ogre::Vector2 &size,
								  const Ogre::Vector2 &hardSize ) colibri_final;
		virtual Ogre::Vector2 getCellSize() const colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
