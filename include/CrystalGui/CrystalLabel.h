
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	class Label : public Renderable
	{
		std::string		m_text[States::NumStates];

		//Renderable	*m_background;

	public:
		Label( CrystalManager *manager );

		/**
		@param text
			Text must be UTF8
		@param forState
			Use NumStates to affect all states
		*/
		void setText( const std::string &text, States::States forState=States::NumStates );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
