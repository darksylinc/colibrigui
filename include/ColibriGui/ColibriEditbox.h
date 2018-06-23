
#pragma once

#include "ColibriGui/ColibriRenderable.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	class Editbox : public Renderable
	{
		Label		*m_label;
		Label		*m_caret;
		uint32_t	m_cursorPos;

	public:
		Editbox( ColibriManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		Label* getLabel();

		void update();

		virtual void setTransformDirty();

		virtual void _notifyActionKeyMovement( Borders::Borders direction );
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
