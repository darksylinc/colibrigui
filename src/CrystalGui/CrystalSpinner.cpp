
#include "CrystalGui/CrystalSpinner.h"
#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/CrystalButton.h"
#include "CrystalGui/CrystalLabel.h"

#include "OgreLwString.h"

namespace Crystal
{
	Spinner::Spinner( CrystalManager *manager ) :
		Renderable( manager ),
		m_optionLabel( 0 ),
		m_label( 0 ),
		m_decrement( 0 ),
		m_increment( 0 ),
		m_currentValue( 0 ),
		m_minValue( 0 ),
		m_maxValue( 10 ),
		m_denominator( 1 ),
		m_valueLocationFraction( 0.8f, 0.5f ),
		m_arrowButtonSize( -1 ),
		m_horizontal( true ),
		m_horizDir( HorizWidgetDir::AutoRTL )
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
		m_optionLabel->setTextVertAlignment( TextVertAlignment::Top );

		m_decrement->setKeyboardNavigable( false );
		m_increment->setKeyboardNavigable( false );

		updateOptionLabel();

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
	void Spinner::updateOptionLabel()
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

		const bool rightToLeft = m_manager->shouldSwapRTL( m_horizDir );

		if( rightToLeft )
		{
			Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping();
			m_optionLabel->sizeToFit( States::Idle, sizeAfterClipping.x * m_valueLocationFraction.x );
			Ogre::Vector2 newCenter( sizeAfterClipping * Ogre::Vector2( 1.0f - m_valueLocationFraction.x,
																		m_valueLocationFraction.y ) );
			m_optionLabel->setCenter( newCenter );
		}
		else
		{
			Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping();
			m_optionLabel->sizeToFit( States::Idle, sizeAfterClipping.x * (1.0f - m_valueLocationFraction.x) );
			Ogre::Vector2 newCenter( sizeAfterClipping * m_valueLocationFraction );
			m_optionLabel->setCenter( newCenter );
		}

		Ogre::Vector2 arrowButtonSize = m_arrowButtonSize;
		if( arrowButtonSize.y < 0.0f )
			arrowButtonSize.y = std::min( getSize().x, getSize().y ) * 0.05f;
		if( arrowButtonSize.x < 0.0f )
			arrowButtonSize.x = arrowButtonSize.y;
		m_decrement->setSize( arrowButtonSize );
		m_increment->setSize( arrowButtonSize );

		m_decrement->setTopLeft( m_optionLabel->getLocalTopLeft() - m_decrement->getSize() );
		m_decrement->setCenter( Ogre::Vector2( m_decrement->getCenter().x, m_optionLabel->getCenter().y ) );
//		m_decrement->setTopLeft( m_optionLabel->getLocalTopLeft() );
//		m_decrement->setSize( m_optionLabel->getSize() );

		m_increment->setTopLeft( m_optionLabel->getLocalBottomRight() );
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
			m_label->setTextHorizAlignment( TextHorizAlignment::Center );
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
			updateOptionLabel();
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
				CRYSTAL_ASSERT_LOW( minValue <= maxValue );
			}
		}
		else
		{
			logListener->log( "Invalid call Spinner::setRange. Cannot call this function while "
							  "in list-mode (setOption was not empty)", LogSeverity::Warning );
			CRYSTAL_ASSERT_LOW( false );
		}

		updateOptionLabel();
	}
	//-------------------------------------------------------------------------
	void Spinner::setOptions( const std::vector<std::string> &options )
	{
		m_options = options;
		updateOptionLabel();
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
	void Spinner::setTransformDirty()
	{
		if( m_label )
		{
			if( m_label->getSize() != m_size )
				m_label->setSize( m_size );
		}

		updateOptionLabel();

		Renderable::setTransformDirty();
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
				callActionListeners( Action::ValueChanged );
			}
			else if( widget == m_increment )
			{
				if( m_horizontal && m_manager->swapRTLControls() )
					--m_currentValue;
				else
					++m_currentValue;
				updateOptionLabel();
				callActionListeners( Action::ValueChanged );
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
			callActionListeners( Action::ValueChanged );
		}
		else if( (direction == Borders::Right && m_horizontal) ||
				 (direction == Borders::Top && !m_horizontal) )
		{
			if( m_horizontal && m_manager->swapRTLControls() )
				--m_currentValue;
			else
				++m_currentValue;
			updateOptionLabel();
			callActionListeners( Action::ValueChanged );
		}
	}
}
