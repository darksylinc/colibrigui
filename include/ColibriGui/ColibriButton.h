
#pragma once

#include "ColibriGui/ColibriRenderable.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Button
	*/
	class Button : public Renderable
	{
		Label	*m_label;

	public:
		Button( ColibriManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		Label* getLabel();

		virtual void setTransformDirty();
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
