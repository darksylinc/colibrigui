
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	/** @ingroup Controls
	@class Button
	*/
	class Button : public Renderable
	{
		Label	*m_label;

	public:
		Button( CrystalManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		Label* getLabel();

		virtual void setTransformDirty();
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
