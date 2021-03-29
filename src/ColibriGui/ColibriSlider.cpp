
#include "ColibriGui/ColibriSlider.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"

namespace Colibri
{
	Slider::Slider( ColibriManager *manager ) :
		Widget( manager ),
		IdObject( Ogre::Id::generateNewId<Progressbar>() ),
		m_currentValue( 0 ),
		m_minValue( 0 ),
		m_maxValue( 10 ),
		m_denominator( 1 ),
		m_cursorOffset( 0.0f ),
		m_lineSize( 5.0f ),
		m_handleProportion( 0.8f ),
		m_vertical( false ),
		m_alwaysInside( false )
	{
		memset( m_layers, 0, sizeof( m_layers ) );

		m_clickable = true;
		m_keyboardNavigable = true;
		m_consumesScroll = true;

		m_autoSetNextWidget[Borders::Left] = false;
		m_autoSetNextWidget[Borders::Right] = false;
	}
	//-------------------------------------------------------------------------
	void Slider::_initialize()
	{
		for( size_t i = 0u; i < 2u; ++i )
		{
			m_layers[i] = m_manager->createWidget<Renderable>( this );
			m_layers[i]->_initialize();
		}

		const Ogre::IdString skinPackName =
			m_manager->getDefaultSkinPackName( SkinWidgetTypes::SliderLine );
		const SkinManager *skinManager = m_manager->getSkinManager();

		const SkinPack *defaultSkinPack = skinManager->findSkinPack( skinPackName, LogSeverity::Fatal );

		m_lineSize = defaultSkinPack->sliderLineSize;
		m_handleProportion[0] = defaultSkinPack->sliderHandleProportion[0];
		m_handleProportion[1] = defaultSkinPack->sliderHandleProportion[1];
		m_alwaysInside = defaultSkinPack->sliderAlwaysInside;

		getSliderLine()->_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::SliderLine ) );
		getSliderHandle()->_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::SliderHandle ) );

		Widget::_initialize();

		/*const Ogre::IdString skinPackName =
			m_manager->getDefaultSkinPackName( SkinWidgetTypes::SliderLine );
		const SkinManager *skinManager = m_manager->getSkinManager();

		const SkinPack *defaultSkinPack = skinManager->findSkinPack( skinPackName, LogSeverity::Fatal );

		if( !defaultSkinPack )
			return;*/
	}
	//-------------------------------------------------------------------------
	void Slider::_destroy()
	{
		Widget::_destroy();

		// m_layers[i] are children of us, so they will be destroyed by our super class
		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i] = 0;
	}
	//-------------------------------------------------------------------------
	void Slider::setSkinPack( Ogre::IdString linePackName, Ogre::IdString handlePackName )
	{
		if( linePackName != Ogre::IdString() )
		{
			const SkinManager *skinManager = m_manager->getSkinManager();
			const SkinPack *linePack = skinManager->findSkinPack( linePackName, LogSeverity::Fatal );
			m_lineSize = linePack->sliderLineSize;
			m_handleProportion[0] = linePack->sliderHandleProportion[0];
			m_handleProportion[1] = linePack->sliderHandleProportion[1];
			m_alwaysInside = linePack->sliderAlwaysInside;

			getSliderLine()->setSkinPack( linePackName );
		}

		if( handlePackName != Ogre::IdString() )
			getSliderHandle()->setSkinPack( handlePackName );
	}
	//-------------------------------------------------------------------------
	Renderable *Slider::getSliderLine() { return m_layers[0]; }
	//-------------------------------------------------------------------------
	Renderable *Slider::getSliderHandle() { return m_layers[1]; }
	//-------------------------------------------------------------------------
	void Slider::setVisualsEnabled( bool bEnabled )
	{
		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i]->setVisualsEnabled( bEnabled );
	}
	//-------------------------------------------------------------------------
	bool Slider::isVisualsEnabled() const { return m_layers[0]->isVisualsEnabled(); }
	//-------------------------------------------------------------------------
	void Slider::setState( States::States state, bool smartHighlight, bool broadcastEnable )
	{
		Widget::setState( state, smartHighlight, broadcastEnable );

		if( state == States::Pressed )
		{
			processCursorPosition( m_manager->getMouseCursorPosNdc(), true );
		}
		else
			m_cursorOffset = 0.0f;

		// Widget::setState did not re-enable children we control. Do it manually
		if( !broadcastEnable )
		{
			for( size_t i = 0u; i < 2u; ++i )
			{
				if( m_layers[i]->isDisabled() )
					m_layers[i]->setState( state, smartHighlight, false );
			}
		}
	}
	//-------------------------------------------------------------------------
	void Slider::updateSlider()
	{
		if( !m_layers[0] )
			return;  //_initialize hasn't been called yet

		const Ogre::Vector2 frameSize = getSize();

		// const Ogre::Vector2 frameOrigin = getLocalTopLeft();
		const Ogre::Vector2 frameOrigin = Ogre::Vector2::ZERO;

		if( !m_vertical )
		{
			const bool rightToLeft = m_manager->shouldSwapRTL( HorizWidgetDir::AutoLTR );

			const float sliderLineHeight = m_lineSize;
			const Ogre::Vector2 handleSize = m_handleProportion * frameSize.y;
			const float handlePadding = m_alwaysInside ? 0.0f : handleSize.x;

			// Slider line
			const float lineTop = frameOrigin.y + ( frameSize.y - sliderLineHeight ) * 0.5f;

			// Horizontally: Half a handle is added to the line on each side as padding.
			// Vertically: Center the line
			m_layers[0]->setTopLeft( Ogre::Vector2( frameOrigin.x + handlePadding * 0.5f, lineTop ) );

			// Other than the padding, the width is used to its full, but the height is always constant.
			const float reducedLineWidth = frameSize.x - handlePadding;
			m_layers[0]->setSize( Ogre::Vector2( reducedLineWidth, sliderLineHeight ) );

			const float slideableArea =
				m_alwaysInside ? ( frameSize.x - handleSize.x ) : reducedLineWidth;

			// Slider handle
			m_layers[1]->setSize( handleSize );

			const float sliderValueUnorm = getCurrentValueUnorm();
			const float targetSliderValue = rightToLeft ? ( 1.0f - sliderValueUnorm ) : sliderValueUnorm;

			Ogre::Vector2 handleTopLeft( frameOrigin.x + ( slideableArea * targetSliderValue ),
										 lineTop + ( sliderLineHeight - handleSize.y ) * 0.5f );
			// This snap isn't perfect because it only snaps to local coordinates, not final NDC coord.
			// However it still does a very good job at preventing the handle from "wobbling" as
			// the user moves it
			handleTopLeft = m_manager->snapToPixels( handleTopLeft );
			m_layers[1]->setTopLeft( handleTopLeft );
		}
		else
		{
			const float sliderLineWidth = m_lineSize;
			const Ogre::Vector2 handleSize = m_handleProportion * frameSize.x;
			const float handlePadding = m_alwaysInside ? 0.0f : handleSize.y;

			// Slider line
			const float lineLeft = frameOrigin.x + ( frameSize.x - sliderLineWidth ) * 0.5f;

			// Vertically: Half a handle is added to the line on each side as padding.
			// Horizontally: Center the line
			m_layers[0]->setTopLeft( Ogre::Vector2( lineLeft, frameOrigin.y + handlePadding * 0.5f ) );

			// Other than the padding, the width is used to its full, but the height is always constant.
			const float reducedLineHeight = frameSize.y - handlePadding;
			m_layers[0]->setSize( Ogre::Vector2( sliderLineWidth, reducedLineHeight ) );

			const float slideableArea =
				m_alwaysInside ? ( frameSize.y - handleSize.y ) : reducedLineHeight;

			// Slider handle
			m_layers[1]->setSize( handleSize );

			const float sliderValueUnorm = getCurrentValueUnorm();
			const float targetSliderValue = 1.0f - sliderValueUnorm;
			Ogre::Vector2 handleTopLeft( lineLeft + ( sliderLineWidth - handleSize.x ) * 0.5f,
										 frameOrigin.y + ( slideableArea * targetSliderValue ) );
			// This snap isn't perfect because it only snaps to local coordinates, not final NDC coord.
			// However it still does a very good job at preventing the handle from "wobbling" as
			// the user moves it
			handleTopLeft = m_manager->snapToPixels( handleTopLeft );
			m_layers[1]->setTopLeft( handleTopLeft );
		}

		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i]->updateDerivedTransformFromParent( false );
	}
	//-------------------------------------------------------------------------
	void Slider::setTransformDirty( uint32_t dirtyReason )
	{
		// Only update if our size is directly being changed, not our parent's
		if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) ==
			TransformDirtyScale )
		{
			updateSlider();
		}

		Widget::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Slider::processCursorPosition( const Ogre::Vector2 &pos, bool cursorBegin )
	{
		if( m_currentState == States::Pressed && this->intersects( pos ) )
		{
			const float sliderValueUnorm = getCurrentValueUnorm();

			if( !m_vertical )
			{
				const bool rightToLeft = m_manager->shouldSwapRTL( HorizWidgetDir::AutoLTR );

				const float sliderWidth =
					getSliderLine()->getDerivedBottomRight().x - getSliderLine()->getDerivedTopLeft().x;
				const float mouseRelativeX = pos.x - m_derivedTopLeft.x;
				float posX = mouseRelativeX / sliderWidth;
				if( rightToLeft )
					posX = 1.0f - posX;

				if( cursorBegin && getSliderHandle()->intersects( pos ) )
				{
					// The user actually clicked on the handle, rather than part of the line.
					// If this happens, apply an offset to the mouse movements, so the handle doesn't
					// jump.
					m_cursorOffset = posX - sliderValueUnorm;
				}

				setCurrentValue( static_cast<int32_t>(
					roundf( ( posX - m_cursorOffset ) * ( m_maxValue - m_minValue ) ) + m_minValue ) );
			}
			else
			{
				const float sliderHeight =
					getSliderLine()->getDerivedBottomRight().y - getSliderLine()->getDerivedTopLeft().y;
				const float mouseRelativeY = pos.y - m_derivedTopLeft.y;
				float posY = 1.0f - mouseRelativeY / sliderHeight;

				if( cursorBegin && getSliderHandle()->intersects( pos ) )
				{
					// The user actually clicked on the handle, rather than part of the line.
					// If this happens, apply an offset to the mouse movements, so the handle doesn't
					// jump.
					m_cursorOffset = posY - sliderValueUnorm;
				}

				setCurrentValue( static_cast<int32_t>(
					roundf( ( posY - m_cursorOffset ) * ( m_maxValue - m_minValue ) ) + m_minValue ) );
			}

			m_manager->callActionListeners( this, Action::ValueChanged );
		}
	}
	//-------------------------------------------------------------------------
	void Slider::notifyCursorMoved( const Ogre::Vector2 &posNDC ) { processCursorPosition( posNDC ); }
	//-------------------------------------------------------------------------
	void Slider::_notifyActionKeyMovement( Borders::Borders direction )
	{
		if( !m_vertical )
		{
			const bool rightToLeft = m_manager->shouldSwapRTL( HorizWidgetDir::AutoLTR );
			const int32_t targetDirectionAmount = rightToLeft ? -1 : 1;

			if( direction == Borders::Left )
				setCurrentValue( m_currentValue - targetDirectionAmount );
			else if( direction == Borders::Right )
				setCurrentValue( m_currentValue + targetDirectionAmount );
		}
		else
		{
			const int32_t targetDirectionAmount = 1;
			if( direction == Borders::Top )
				setCurrentValue( m_currentValue + targetDirectionAmount );
			else if( direction == Borders::Bottom )
				setCurrentValue( m_currentValue - targetDirectionAmount );
		}

		m_manager->callActionListeners( this, Action::ValueChanged );
	}
	//-------------------------------------------------------------------------
	void Slider::setElementsSize( const Ogre::Vector2 &handleProportion, const float lineSize )
	{
		m_handleProportion = handleProportion;
		m_lineSize = lineSize;
		updateSlider();
	}
	//-------------------------------------------------------------------------
	void Slider::setCurrentValue( int32_t currentValue )
	{
		if( m_currentValue != currentValue )
		{
			m_currentValue = Ogre::Math::Clamp( currentValue, m_minValue, m_maxValue );
			updateSlider();
		}
	}
	//-------------------------------------------------------------------------
	void Slider::setDenominator( int32_t denominator )
	{
		if( m_denominator != denominator )
		{
			m_denominator = denominator;
			updateSlider();
		}
	}
	//-------------------------------------------------------------------------
	float Slider::getCurrentValueProcessed() const
	{
		return m_currentValue / static_cast<float>( m_denominator );
	}
	//-------------------------------------------------------------------------
	float Slider::getCurrentValueUnorm() const
	{
		if( m_minValue == m_maxValue )
			return 0.0f;
		return ( m_currentValue - m_minValue ) / static_cast<float>( m_maxValue - m_minValue );
	}
	//-------------------------------------------------------------------------
	void Slider::setRange( int32_t minValue, int32_t maxValue )
	{
		if( minValue < maxValue )
		{
			m_minValue = minValue;
			m_maxValue = maxValue;
		}
		else
		{
			LogListener *logListener = m_manager->getLogListener();

			char tmpBuffer[128];
			Ogre::LwString msg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
			msg.a( "Invalid range for Slider::setRange min = ", minValue, " max = ", maxValue );
			logListener->log( msg.c_str(), LogSeverity::Warning );
			COLIBRI_ASSERT_LOW( minValue <= maxValue );
		}

		updateSlider();
	}
	//-------------------------------------------------------------------------
	void Slider::setAlwaysInside( bool bAlwaysInside )
	{
		m_alwaysInside = bAlwaysInside;
		updateSlider();
	}
	//-------------------------------------------------------------------------
	void Slider::setVertical( bool bVertical )
	{
		m_vertical = bVertical;

		m_autoSetNextWidget[Borders::Left] = bVertical;
		m_autoSetNextWidget[Borders::Right] = bVertical;
		m_autoSetNextWidget[Borders::Top] = !bVertical;
		m_autoSetNextWidget[Borders::Bottom] = !bVertical;

		updateSlider();
	}
}  // namespace Colibri
