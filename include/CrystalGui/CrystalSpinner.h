
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	/** Spinner – value input control which has small up and down buttons
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

		bool			m_horizontal;
		HorizWidgetDir::HorizWidgetDir	m_horizDir;

		std::vector<std::string>	m_options;

		void updateOptionLabel();

	public:
		Spinner( CrystalManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		Label* getLabel();

		/** Sets the current value of the spinner. If the value is outside of range, it gets clamped
			See setRange, setOptions
		@remarks
			Calling this function won't trigger the listeners, even if the value gets clamped.
			You can do that yourself by doing callActionListeners( Action::ValueChanged )
		*/
		void setCurrentValue( int32_t currentValue );

		/** Sets the denominator in order to be able to represent fractional values.
			for example calling:
			@code
				spinner->setCurrentValue( 5 );
				spinner->setDenominator( 2 );
			@endcode
			Will display the value "2.5" since 5 / 2 = 2.5
			This call is ignored if operating in list mode. See setOptions.
		@remarks
			See setCurrentValue remarks
		*/
		void setDenominator( int32_t denominator );

		/// Returns the current value. If m_options is not empty, then the returned value is
		/// guaranteed to be in range [0; m_options.size())
		int32_t getCurrentValueRaw() const					{ return m_currentValue; }
		int32_t getDenominator() const						{ return m_denominator; }
		/// Returns m_currentValue / m_denominator
		float getCurrentValueProcessed() const;

		/// Returns the current value as a string. If m_options is not empty, it returns m_options[i]
		/// Otherwise it returns the number converted to string, which may not be integral if
		/// m_denominator isn't 1.
		std::string getCurrentValueStr() const;

		/** When operating in list-less mode, sets the minimum & maximum range of integers
			the spinner can go to.
			If m_currentValue is currently outside the range, it will be clamped
			If the Spinner is currently in list mode (you called setOptions with a non-empty list)
			then this call is ignored and an assert and a warning are triggered.
		@remarks
			See setCurrentValue remarks
		@param minValue
			Must be minValue <= maxValue
		@param maxValue
			Must be minValue <= maxValue
		*/
		void setRange( int32_t minValue, int32_t maxValue );

		/** Sets the options to cycle through. If the vector is empty, the Spinner goes
			into list-less mode (numeric mode).

			If the vector is not empty, the Spinner goes into List mode, min & max values
			get altered to match that of the list (min = 0, max = options.size() - 1)
			and current value gets clamped to valid range
		@remarks
			See setCurrentValue remarks
		@param options
		*/
		void setOptions( const std::vector<std::string> &options );
		const std::vector<std::string>& getOptions() const;

		/** Sets the horizontal direction. Only useful when the spinner is horizontal.
			See HorizWidgetDir::HorizWidgetDir
		@remarks
			This controls the location of the UI: whether the options and the increase/decrease
			buttons are to the left or the right of the text.
			However it does not control other localization details. For example,
			if CrystalManager::swapRTLControls() returns true, then pushing left will increase
			instead of decrease, regardless of the value of horizWidgetDir.
		@param horizWidgetDir
		*/
		void setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir );
		HorizWidgetDir::HorizWidgetDir getHorizWidgetDir() const	{ return m_horizDir; }

		virtual void setTransformDirty();

		virtual void notifyWidgetAction( Widget *widget, Action::Action action );
		virtual void _notifyActionKeyMovement( Borders::Borders direction );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
