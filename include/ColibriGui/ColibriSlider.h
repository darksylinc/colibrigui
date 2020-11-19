
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgreId.h"
#include "OgreIdString.h"

namespace Ogre
{
	class HlmsUnlitDatablock;
}

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Slider
		Displays an interactive slider widget using two Renderables

		Layer 0 is drawn behind layer 1
	*/
	class Slider colibri_final : public Widget, Ogre::IdObject
	{
	protected:
		Renderable *colibrigui_nullable m_layers[2];

		float m_sliderValue;
		/// When directional actions (keyboard buttons) are applied,
		/// this is how much the slider value be increased or decreased.
		float m_directionChangeAmount;

		float         m_cursorOffset;
		float         m_lineSize;
		Ogre::Vector2 m_handleProportion;
		bool          m_vertical;
		bool          m_alwaysInside;

	protected:
		void updateSlider();

		void processCursorPosition( const Ogre::Vector2 &pos, bool cursorBegin = false );

	public:
		Slider( ColibriManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		/// Sets a different skin pack (than default) for line and/or handle
		///
		/// Leave an empty Ogre::IdString() if you wish to retain the previous skin pack,
		/// i.e. only changing one of them
		void setSkinPack( Ogre::IdString linePackName, Ogre::IdString handlePackName );

		/// @copydoc Renderable::setVisualsEnabled
		void         setVisualsEnabled( bool bEnabled );
		virtual bool isVisualsEnabled() const colibri_final;

		virtual void setState( States::States state, bool smartHighlight = true,
							   bool broadcastEnable = false );

		/** Sets the size of the handle relative to the full
			width (m_vertical = true) or height (m_vertical = false) of the Slider.

			And sets the size of the handle, fixed in virtual canvas units

		@param handleProportion
			Should be in range [0; 1].
			Values outside that range are accepted but may cause visual artifacts
			If both .x and .y are the same value, then the handle will be square
		@param lineSize
			Should be in range [0; inf)
			In virtual canvas units
		*/
		void setElementsSize( const Ogre::Vector2 &handleProportion, const float lineSize );
		const Ogre::Vector2 &getHandleProportion() const { return m_handleProportion; }
		float                getLineSize() const { return m_lineSize; }

		// Set the value of the slider. Right now this is between 0 and 1 only.
		void  setValue( float value );
		float getValue() const { return m_sliderValue; }

		/// When false, the handle reaches half outside when at 0% and 100%
		/// When true, the handle is always contained inside the background
		///
		/// e.g. at 0%
		///
		///	@code
		/// m_alwaysInside = false:
		///
		///		  |                  |
		///     -----                |
		///		| |-|----------------|
		///		-----                |
		///		  |                  |
		///
		/// m_alwaysInside = true:
		///
		///		  |                  |
		///       -----              |
		///		  |---|--------------|
		///		  -----              |
		///		  |                  |
		/// @endcode
		void setAlwaysInside( bool bAlwaysInside );
		bool getAlwaysInside() const { return m_alwaysInside; }

		void setVertical( bool bVertical );
		bool getVertical() const { return m_vertical; }

		Renderable *getSliderLine();
		Renderable *getSliderHandle();

		virtual void setTransformDirty( uint32_t dirtyReason ) colibri_final;

		virtual void notifyCursorMoved( const Ogre::Vector2 &posNDC );
		virtual void _notifyActionKeyMovement( Borders::Borders direction );

		void  setDirectionChangeAmount( float amount ) { m_directionChangeAmount = amount; }
		float getDirectionChangeAmount() const { return m_directionChangeAmount; }
	};
}  // namespace Colibri

COLIBRIGUI_ASSUME_NONNULL_END
