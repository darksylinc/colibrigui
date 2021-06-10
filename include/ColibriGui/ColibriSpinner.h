
#pragma once

#include "ColibriGui/ColibriRenderable.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Spinner
		Value input control which has small up and down buttons
		to step through a range of values'

		This implementation has two modes of operation:
	@par
		Via list:
			The choices are in m_option, and the current one is in m_option[m_currentValue]
			If m_currentValue for some reason is out of range, it gets clamped to become in range.
			m_minValue must be 0.
			m_maxValue must be m_options.size() - 1u
			m_denominator must be 1
	@par
		List-less (numbers only):
			m_options is empty, and m_currentValue is in range [m_minValue; m_maxValue]. The actual
			value displayed is m_currentValue / m_denominator; thus supporting fractional numbers
			without dealing with floating point precision issues when decrementing, incrementing
			and comparing.
	*/
	class Spinner : public Renderable, public WidgetActionListener
	{
		/// Displays the currently selected option
		Label * colibrigui_nullable m_optionLabel;
		/// Displays user-driven text. May be null
		Label * colibrigui_nullable	m_label;
		Button						*m_decrement;
		Button						*m_increment;

		int32_t	m_currentValue;

		int32_t	m_minValue;
		int32_t	m_maxValue;
		int32_t	m_denominator;

		float			m_arrowMargin;
		/// This size is always for horizontal arrows. The .xy gets swapped as .yx for vertical spinners
		Ogre::Vector2	m_arrowSize;

		bool			m_calcFixedSizeFromMaxWidth;
		bool			m_horizontal;
		HorizWidgetDir::HorizWidgetDir	m_horizDir;

		float						m_fixedWidth;
		std::vector<std::string>	m_options;

		void calculateMaximumWidth();
		Ogre::Vector2 calculateMaximumSize();
		void updateOptionLabel( const bool sizeOrAvailableOptionsChanged = false,
								const bool bSkipOptionLabelSize = false );

	public:
		Spinner( ColibriManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		Label* getLabel();

		/** Sets the current value of the spinner. If the value is outside of range, it gets clamped
			See Spinner::setRange, Spinner::setOptions
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
			This call is ignored if operating in list mode. See Spinner::setOptions.
		@remarks
			See Spinner::setCurrentValue remarks
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
			See Spinner::setCurrentValue remarks
		@param minValue
			Must be minValue <= maxValue
		@param maxValue
			Must be minValue <= maxValue
		*/
		void setRange( int32_t minValue, int32_t maxValue );

		int32_t getMinValue() const { return m_minValue; }
		int32_t getMaxValue() const { return m_maxValue; }

		/** Sets the options to cycle through. If the vector is empty, the Spinner goes
			into list-less mode (numeric mode).

			If the vector is not empty, the Spinner goes into List mode, min & max values
			get altered to match that of the list (min = 0, max = options.size() - 1)
			and current value gets clamped to valid range
		@remarks
			See Spinner::setCurrentValue remarks
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
			if ColibriManager::swapRTLControls() returns true, then pushing left will increase
			instead of decrease, regardless of the value of horizWidgetDir.
		@param horizWidgetDir
		*/
		void setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir );
		HorizWidgetDir::HorizWidgetDir getHorizWidgetDir() const	{ return m_horizDir; }

		/** Sets whether the distance between the arrows should remain constant regardless
			of the option currently being selected.

			When autoCalculateFromMaxWidth == false and fixedWidth <= 0, the distance between
			the arrows will vary depending on the width of the string of the current option.

			Otherwise, the arrows will remain at a fixed distance, thus the arrows will
			stop moving (and sometimes, the text too) every time an option is changed.
		@remarks
			When both autoCalculateFromMaxWidth = false and fixedWidth <= 0,
			this setting is disabled.
		@param autoCalculateFromMaxWidth
			True to autocalculate fixedWidth from all available options,
			thus fixedWidth argument will be ignored.

			Note: If the spinner is in numeric mode and m_denominator != 1,
			the autocalculation may not be accurate.
		@param fixedWidth
			When autoCalculateFromMaxWidth is false; this value lets you manually
			specify the fixed size (in canvas units).
			When autoCalculateFromMaxWidth is true, this value is ignored
		*/
		void setFixedWidth( bool autoCalculateFromMaxWidth, float fixedWidth );
		bool getCalcFixedSizeFromMaxWidth() const				{ return m_calcFixedSizeFromMaxWidth; }
		float getFixedWidth() const								{ return m_fixedWidth; }

		/** This version has no params since there are multiple children labels
			involved.
		@see Label::sizeToFit
		*/
		void sizeToFit();

		virtual void setTransformDirty( uint32_t dirtyReason ) colibri_final;

		virtual void notifyWidgetAction( Widget *widget, Action::Action action );
		virtual void _notifyActionKeyMovement( Borders::Borders direction );
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
