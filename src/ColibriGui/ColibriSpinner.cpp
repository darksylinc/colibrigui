
#include "ColibriGui/ColibriSpinner.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriLabel.h"

#include "OgreLwString.h"

namespace Colibri
{
	Spinner::Spinner( ColibriManager *manager ) :
		Renderable( manager ),
		m_optionLabel( 0 ),
		m_label( 0 ),
		m_decrement( 0 ),
		m_increment( 0 ),
		m_currentValue( 0 ),
		m_minValue( 0 ),
		m_maxValue( 10 ),
		m_denominator( 1 ),
		m_arrowMargin( manager->m_defaultArrowMargin ),
		m_arrowSize( manager->m_defaultArrowSize ),
		m_calcFixedSizeFromMaxWidth( false ),
		m_horizontal( true ),
		m_horizDir( HorizWidgetDir::AutoLTR ),
		m_fixedWidth( 0 )
	{
		m_clickable = true;
		m_keyboardNavigable = true;
		m_pressable = false;
		m_childrenClickable = true;

		m_autoSetNextWidget[Borders::Left] = false;
		m_autoSetNextWidget[Borders::Right] = false;
	}
	//-------------------------------------------------------------------------
	void Spinner::_initialize()
	{
		this->_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::Spinner ) );

		m_decrement = m_manager->createWidget<Button>( this );
		m_decrement->_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::SpinnerBtnDecrement ) );

		m_increment = m_manager->createWidget<Button>( this );
		m_increment->_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::SpinnerBtnIncrement ) );

		m_optionLabel = m_manager->createWidget<Label>( this );
		m_optionLabel->setSize( getSizeAfterClipping() );
		m_optionLabel->setTextHorizAlignment( TextHorizAlignment::Center );
		m_optionLabel->setTextVertAlignment( TextVertAlignment::Center );

		m_decrement->setKeyboardNavigable( false );
		m_increment->setKeyboardNavigable( false );

		updateOptionLabel( true );

		Renderable::_initialize();

		m_decrement->addActionListener( this, ActionMask::PrimaryActionPerform );
		m_increment->addActionListener( this, ActionMask::PrimaryActionPerform );
	}
	//-------------------------------------------------------------------------
	void Spinner::_destroy()
	{
		Renderable::_destroy();

		//m_optionLabel is a child of us, so it will be destroyed by our super class
		m_optionLabel = 0;
		//m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	void Spinner::calculateMaximumWidth()
	{
		if( !m_calcFixedSizeFromMaxWidth )
			return;

		const Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping();
		const float arrowMargin = m_arrowMargin;
		const Ogre::Vector2 arrowSize( m_arrowSize[!m_horizontal], m_arrowSize[m_horizontal] );

		const float spaceLeftForOptionLabel = sizeAfterClipping.x - arrowSize.x * 2.0f - arrowMargin;

		const std::string oldText = m_optionLabel->getText();

		m_fixedWidth = 0;

		if( !m_options.empty() )
		{
			std::vector<std::string>::const_iterator itor = m_options.begin();
			std::vector<std::string>::const_iterator end  = m_options.end();

			while( itor != end )
			{
				m_optionLabel->setText( *itor );
				m_optionLabel->sizeToFit( spaceLeftForOptionLabel );
				m_fixedWidth = std::max( m_fixedWidth, m_optionLabel->getSize().x );
				++itor;
			}
		}
		else
		{
			char tmpBuffer[64];
			Ogre::LwString numberStr( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
			if( m_denominator == 1 )
				numberStr.a( m_minValue );
			else
				numberStr.a( m_minValue / (float)m_denominator );

			m_optionLabel->setText( numberStr.c_str() );
			m_optionLabel->sizeToFit( spaceLeftForOptionLabel );
			m_fixedWidth = std::max( m_fixedWidth, m_optionLabel->getSize().x );

			numberStr.clear();
			if( m_denominator == 1 )
				numberStr.a( m_maxValue );
			else
				numberStr.a( m_maxValue / (float)m_denominator );

			m_optionLabel->setText( numberStr.c_str() );
			m_optionLabel->sizeToFit( spaceLeftForOptionLabel );
			m_fixedWidth = std::max( m_fixedWidth, m_optionLabel->getSize().x );
		}

		m_optionLabel->setText( oldText );
	}
	//-------------------------------------------------------------------------
	void Spinner::updateOptionLabel( bool sizeOrAvailableOptionsChanged )
	{
		if( !m_optionLabel )
			return; //_initialize hasn't been called yet

		if( !m_options.empty() )
		{
			m_minValue = 0;
			m_maxValue = static_cast<int32_t>( m_options.size() - 1u );
			m_denominator = 1;
		}

		m_currentValue = Ogre::Math::Clamp( m_currentValue, m_minValue, m_maxValue );
		m_optionLabel->setText( this->getCurrentValueStr() );

		if( m_currentValue == m_minValue )
			m_decrement->setHidden( true );
		else
		{
			m_decrement->setHidden( false );
			m_decrement->setState( m_currentState );
		}

		if( m_currentValue == m_maxValue )
			m_increment->setHidden( true );
		else
		{
			m_increment->setHidden( false );
			m_increment->setState( m_currentState );
		}

		if( sizeOrAvailableOptionsChanged )
			calculateMaximumWidth();

		const bool rightToLeft = m_manager->shouldSwapRTL( m_horizDir );

		const Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping();

		const Ogre::Vector2 arrowSize( m_arrowSize[!m_horizontal], m_arrowSize[m_horizontal] );
		m_decrement->setSize( arrowSize );
		m_increment->setSize( arrowSize );

		const float arrowMargin = m_arrowMargin;

		if( !m_calcFixedSizeFromMaxWidth && m_fixedWidth <= 0 )
		{
			const float spaceLeftForOptionLabel = sizeAfterClipping.x - arrowSize.x * 2.0f - arrowMargin;
			m_optionLabel->sizeToFit( spaceLeftForOptionLabel );
			m_optionLabel->setSize( Ogre::Vector2( m_optionLabel->getSize().x, sizeAfterClipping.y ) );
		}
		else
		{
			m_optionLabel->setSize( Ogre::Vector2( m_fixedWidth, sizeAfterClipping.y ) );
		}

		const float spaceLeftForTextLabel = sizeAfterClipping.x - arrowMargin * 4.0f -
											arrowSize.x * 2.0f - m_optionLabel->getSize().x;

		if( rightToLeft )
		{
			m_optionLabel->setTopLeft( Ogre::Vector2( arrowMargin * 2.0f + arrowSize.x, 0.0f ) );
			if( m_label )
			{
				m_label->setTopLeft( Ogre::Vector2( sizeAfterClipping.x - spaceLeftForTextLabel, 0.0f ) );
				m_label->setSize( Ogre::Vector2( spaceLeftForTextLabel, sizeAfterClipping.y ) );
			}
		}
		else
		{
			m_optionLabel->setTopLeft( Ogre::Vector2( sizeAfterClipping.x - arrowMargin * 2.0f -
													  arrowSize.x - m_optionLabel->getSize().x, 0.0f ) );
			if( m_label )
			{
				m_label->setTopLeft( Ogre::Vector2::ZERO );
				m_label->setSize( Ogre::Vector2( spaceLeftForTextLabel, sizeAfterClipping.y ) );
			}
		}

		Ogre::Vector2 margin2( Ogre::Vector2::ZERO );
		margin2[!m_horizontal] = arrowMargin;

		m_decrement->setTopLeft( m_optionLabel->getLocalTopLeft() - m_decrement->getSize() - margin2 );
		m_decrement->setCenter( Ogre::Vector2( m_decrement->getCenter().x, m_optionLabel->getCenter().y ) );

		m_increment->setTopLeft( m_optionLabel->getLocalBottomRight() + margin2 );
		m_increment->setCenter( Ogre::Vector2( m_increment->getCenter().x, m_optionLabel->getCenter().y ) );

		m_optionLabel->updateDerivedTransformFromParent( false );
		m_decrement->updateDerivedTransformFromParent( false );
		m_increment->updateDerivedTransformFromParent( false );
	}
	//-------------------------------------------------------------------------
	Label* Spinner::getLabel()
	{
		if( !m_label )
		{
			m_label = m_manager->createWidget<Label>( this );
			m_label->setSize( getSizeAfterClipping() );
			m_label->setTextHorizAlignment( TextHorizAlignment::Natural );
			m_label->setTextVertAlignment( TextVertAlignment::Center );
		}

		return m_label;
	}
	//-------------------------------------------------------------------------
	void Spinner::setCurrentValue( int32_t currentValue )
	{
		if( m_currentValue != currentValue )
		{
			m_currentValue = currentValue;
			updateOptionLabel();
		}
	}
	//-------------------------------------------------------------------------
	void Spinner::setDenominator( int32_t denominator )
	{
		if( m_denominator != denominator && !m_options.empty() )
		{
			m_denominator = denominator;
			updateOptionLabel( true );
		}
	}
	//-------------------------------------------------------------------------
	float Spinner::getCurrentValueProcessed() const
	{
		return m_currentValue / static_cast<float>( m_denominator );
	}
	//-------------------------------------------------------------------------
	std::string Spinner::getCurrentValueStr() const
	{
		if( !m_options.empty() )
		{
			return m_options[m_currentValue];
		}
		else
		{
			char tmpBuffer[64];
			Ogre::LwString numberStr( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
			if( m_denominator == 1 )
				numberStr.a( m_currentValue );
			else
				numberStr.a( m_currentValue / (float)m_denominator );
			return numberStr.c_str();
		}
	}
	//-------------------------------------------------------------------------
	void Spinner::setRange( int32_t minValue, int32_t maxValue )
	{
		LogListener *logListener = m_manager->getLogListener();

		if( m_options.empty() )
		{
			if( minValue <= maxValue )
			{
				m_minValue = minValue;
				m_maxValue = maxValue;
			}
			else
			{
				char tmpBuffer[128];
				Ogre::LwString msg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
				msg.a( "Invalid range for Spinner::setRange min = ", minValue, " max = ", maxValue  );
				logListener->log( msg.c_str(), LogSeverity::Warning );
				COLIBRI_ASSERT_LOW( minValue <= maxValue );
			}
		}
		else
		{
			logListener->log( "Invalid call Spinner::setRange. Cannot call this function while "
							  "in list-mode (setOption was not empty)", LogSeverity::Warning );
			COLIBRI_ASSERT_LOW( false );
		}

		updateOptionLabel( true );
	}
	//-------------------------------------------------------------------------
	void Spinner::setOptions( const std::vector<std::string> &options )
	{
		m_options = options;
		updateOptionLabel( true );
	}
	//-------------------------------------------------------------------------
	const std::vector<std::string>& Spinner::getOptions() const
	{
		return m_options;
	}
	//-------------------------------------------------------------------------
	void Spinner::setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir )
	{
		m_horizDir = horizWidgetDir;
		updateOptionLabel();
	}
	//-------------------------------------------------------------------------
	void Spinner::setFixedWidth( bool autoCalculateFromMaxWidth, float fixedWidth )
	{
		if( m_calcFixedSizeFromMaxWidth != autoCalculateFromMaxWidth ||
			(autoCalculateFromMaxWidth == false && m_fixedWidth != fixedWidth) )
		{
			m_calcFixedSizeFromMaxWidth = autoCalculateFromMaxWidth;
			m_fixedWidth = fixedWidth;
			updateOptionLabel( true );
		}
	}
	//-------------------------------------------------------------------------
	void Spinner::setTransformDirty( uint32_t dirtyReason )
	{
		if( (dirtyReason & (TransformDirtyParentCaller|TransformDirtyScale)) == TransformDirtyScale )
			updateOptionLabel( true );

		Renderable::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Spinner::notifyWidgetAction( Widget *widget, Action::Action action )
	{
		if( action == Action::PrimaryActionPerform )
		{
			if( widget == m_decrement )
			{
				if( m_horizontal && m_manager->swapRTLControls() )
					++m_currentValue;
				else
					--m_currentValue;
				updateOptionLabel();
				_callActionListeners( Action::ValueChanged );
			}
			else if( widget == m_increment )
			{
				if( m_horizontal && m_manager->swapRTLControls() )
					--m_currentValue;
				else
					++m_currentValue;
				updateOptionLabel();
				_callActionListeners( Action::ValueChanged );
			}
		}
	}
	//-------------------------------------------------------------------------
	void Spinner::_notifyActionKeyMovement( Borders::Borders direction )
	{
		if( (direction == Borders::Left && m_horizontal) ||
			(direction == Borders::Bottom && !m_horizontal) )
		{
			if( m_horizontal && m_manager->swapRTLControls() )
				++m_currentValue;
			else
				--m_currentValue;
			updateOptionLabel();
			_callActionListeners( Action::ValueChanged );
		}
		else if( (direction == Borders::Right && m_horizontal) ||
				 (direction == Borders::Top && !m_horizontal) )
		{
			if( m_horizontal && m_manager->swapRTLControls() )
				--m_currentValue;
			else
				++m_currentValue;
			updateOptionLabel();
			_callActionListeners( Action::ValueChanged );
		}
	}
}
