
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
	public:
		/// True to layout all cells as a column
		/// False to layout all cells as a row
		Ogre::Vector2	m_topLeft;

		/** Maximum size to distribute the size proportionally. If the all the cells combined
			are bigger than this size, then this maximum size is enlarged until all objects fit.

			If there are proportional cells, these cells will be enlarged to fit softMaxSize.

			This option is meant for scrolling windows:
				* If the window is big enough to fit, then all objects are distributed proportionally
				  to fit the entire window
				* If the window is too small, then scrolling is used

			Only the x component is used if bVertical = false, only the y component is used
			otherwise.

			@see	LayoutBase::m_hardMaxSize
		*/
		Ogre::Vector2	m_softMaxSize;

		/** Maximum allowed size. Rows/columns are not allowed to exceed this size.

			If all objects combined exceed this size, then cells that are below their min
			size start stealing from other cells who have empty space.

			If the cells are still too big after all empty space has been stolen, then they won't
			be rendered correctly and minimum cell sizes won't be respected.
			The cells simply do not fit.

			@see	LayoutBase::m_softMaxSize
			@see	LayoutCell::m_priority
			@see	LayoutCell::getCellSize
			@see	LayoutCell::getCellMinSize
		*/
		Ogre::Vector2	m_hardMaxSize;

	public:
		LayoutBase( ColibriManager *colibriManager );

		void setCellOffset( const Ogre::Vector2 &topLeft ) colibri_final;
		void setCellSize( const Ogre::Vector2 &size ) colibri_final;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
