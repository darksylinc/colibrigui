
#include "ColibriGui/ColibriSlider.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"

namespace Colibri
{
	Slider::Slider( ColibriManager *manager ) :
		Widget( manager ),
		IdObject( Ogre::Id::generateNewId<Progressbar>() ),
		m_sliderValue( 0.0f ),
		m_directionChangeAmount( 0.1f ),
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

			const float targetSliderValue = rightToLeft ? ( 1.0f - m_sliderValue ) : m_sliderValue;

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

			const float targetSliderValue = 1.0f - m_sliderValue;
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
					m_cursorOffset = posX - m_sliderValue;
				}

				setValue( posX - m_cursorOffset );
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
					m_cursorOffset = posY - m_sliderValue;
				}

				setValue( posY - m_cursorOffset );
			}
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
			const float targetDirectionAmount =
				rightToLeft ? -m_directionChangeAmount : m_directionChangeAmount;

			if( direction == Borders::Left )
				setValue( m_sliderValue - targetDirectionAmount );
			else if( direction == Borders::Right )
				setValue( m_sliderValue + targetDirectionAmount );
		}
		else
		{
			const float targetDirectionAmount = m_directionChangeAmount;
			if( direction == Borders::Top )
				setValue( m_sliderValue + targetDirectionAmount );
			else if( direction == Borders::Bottom )
				setValue( m_sliderValue - targetDirectionAmount );
		}
	}
	//-------------------------------------------------------------------------
	void Slider::setElementsSize( const Ogre::Vector2 &handleProportion, const float lineSize )
	{
		m_handleProportion = handleProportion;
		m_lineSize = lineSize;
		updateSlider();
	}
	//-------------------------------------------------------------------------
	void Slider::setValue( float value )
	{
		value = Ogre::Math::saturate( value );

		m_sliderValue = value;

		updateSlider();

		callActionListeners( Action::ValueChanged );
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
