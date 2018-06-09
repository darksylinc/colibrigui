
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	/** 'Spinner – value input control which has small up and down buttons
		 to step through a range of values'

		 This implementation has two modes of operation:

		 Via list:
			The choices are in m_option, and the current one is in m_option[m_currentValue]
			If m_currentValue for some reason is out of range, it gets clamped to become in range.
			m_minValue must be 0.
			m_maxValue must be m_options.size() - 1u
			m_denominator must be 1

		 List-less (numbers only):
			m_options is empty, and m_currentValue is in range [m_minValue; m_maxValue]. The actual
			value displayed is m_currentValue / m_denominator; thus supporting fractional numbers
			without dealing with floating point precision issues when decrementing, incrementing
			and comparing.
	*/
	class Spinner : public Renderable, public WidgetActionListener
	{
		/// Displays the currently selected option
		Label * crystalgui_nullable m_optionLabel;
		/// Displays user-driven text. May be null
		Label * crystalgui_nullable	m_label;
		Button						*m_decrement;
		Button						*m_increment;

		int32_t	m_currentValue;

		int32_t	m_minValue;
		int32_t	m_maxValue;
		int32_t	m_denominator;

		Ogre::Vector2	m_valueLocationFraction;
		Ogre::Vector2	m_arrowButtonSize;

		std::vector<std::string>	m_options;

		void updateOptionLabel();

	public:
		Spinner( CrystalManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		Label* getLabel();

		virtual void setTransformDirty();

		virtual void notifyWidgetAction( Widget *widget, Action::Action action );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
