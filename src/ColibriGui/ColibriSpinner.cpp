
#include "ColibriGui/ColibriSpinner.h"

#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"

#include "OgreLwString.h"

namespace Colibri
{
	Spinner::Spinner( ColibriManager *manager ) :
		Renderable( manager ),
		m_label( 0 ),
		m_optionLabel( 0 ),
		m_decrement( 0 ),
		m_increment( 0 ),
		m_currentValue( 0 ),
		m_minValue( 0 ),
		m_maxValue( 10 ),
		m_denominator( 1 ),
		m_arrowMargin( manager->m_defaultArrowMargin ),
		m_arrowSize( manager->m_defaultArrowSize ),
		m_autoCalcSizes( true ),
		m_sizeLabel( 0.0f ),
		m_sizeOptionLabel( 0.0f ),
		m_horizDir( HorizWidgetDir::AutoLTR )
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

		calculateSizes();
		updateOptionLabel();

		Renderable::_initialize();

		m_decrement->addActionListener( this, ActionMask::PrimaryActionPerform );
		m_increment->addActionListener( this, ActionMask::PrimaryActionPerform );
	}
	//-------------------------------------------------------------------------
	void Spinner::_destroy()
	{
		Renderable::_destroy();

		// m_optionLabel is a child of us, so it will be destroyed by our super class
		m_optionLabel = 0;
		// m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	void Spinner::getSizes( float outSizes[SW_NumSubWidgets] ) const
	{
		outSizes[SW_Decrement] = m_arrowSize.x + m_arrowMargin * 2.0f;
		outSizes[SW_OptionLabel] = m_sizeOptionLabel;
		outSizes[SW_Increment] = m_arrowSize.x + m_arrowMargin * 2.0f;

		// For some unknown reason we have to sub two extra arrow margin from remainingSize
		if( m_label )
		{
			const float remainingSize =
				std::max( 0.0f, m_size.x - outSizes[SW_Decrement] - outSizes[SW_Increment] -
									outSizes[SW_OptionLabel] - m_arrowMargin * 2.0f );
			outSizes[SW_Label] = std::min( m_sizeLabel, remainingSize );
			outSizes[SW_Space] = remainingSize - outSizes[SW_Label];
		}
		else
		{
			const float remainingSize =
				std::max( 0.0f, m_size.x - outSizes[SW_Decrement] - outSizes[SW_Increment] -
									m_arrowMargin * 2.0f );

			outSizes[SW_OptionLabel] = std::max( outSizes[SW_OptionLabel], remainingSize );
			outSizes[SW_Label] = 0.0f;
			outSizes[SW_Space] = 0.0f;
		}
	}
	//-------------------------------------------------------------------------
	void Spinner::calculateSizes( float &outSizeLabel, float &outSizeOptionLabel, float &outHeight )
	{
		// Label
		if( m_label )
		{
			m_label->sizeToFit();
			outSizeLabel = m_label->getSize().x;
		}
		else
			outSizeLabel = 0.0f;

		// Options. We must check them all
		const std::string oldText = m_optionLabel->getText();
		Ogre::Vector2 maxOptionSize( Ogre::Vector2::ZERO );

		if( m_label )
			maxOptionSize.y = m_label->getSize().y;

		if( !m_options.empty() )
		{
			std::vector<std::string>::const_iterator itor = m_options.begin();
			std::vector<std::string>::const_iterator endt = m_options.end();

			while( itor != endt )
			{
				m_optionLabel->setText( *itor );
				m_optionLabel->sizeToFit();
				maxOptionSize.makeCeil( m_optionLabel->getSize() );
				++itor;
			}
		}
		else
		{
			char tmpBuffer[64];
			Ogre::LwString numberStr(
				Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

			// Min value
			numberStr.clear();
			if( m_denominator == 1 )
				numberStr.a( m_minValue );
			else
				numberStr.a( m_minValue / (float)m_denominator );

			m_optionLabel->setText( numberStr.c_str() );
			m_optionLabel->sizeToFit();
			maxOptionSize.makeCeil( m_optionLabel->getSize() );

			// Max value
			numberStr.clear();
			if( m_denominator == 1 )
				numberStr.a( m_maxValue );
			else
				numberStr.a( m_maxValue / (float)m_denominator );

			m_optionLabel->setText( numberStr.c_str() );
			m_optionLabel->sizeToFit();
			maxOptionSize.makeCeil( m_optionLabel->getSize() );
		}

		outSizeOptionLabel = maxOptionSize.x;
		outHeight = maxOptionSize.y;

		// Restore
		m_optionLabel->setText( oldText );
	}
	//-------------------------------------------------------------------------
	void Spinner::calculateSizes()
	{
		if( !m_autoCalcSizes )
			return;

		float unusedHeight;
		calculateSizes( m_sizeLabel, m_sizeOptionLabel, unusedHeight );
	}
	//-------------------------------------------------------------------------
	void Spinner::updateOptionLabel()
	{
		if( !m_optionLabel )
			return;  //_initialize hasn't been called yet

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

		const Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping();

		const Ogre::Vector2 arrowSize( m_arrowSize );
		m_decrement->setSize( arrowSize );
		m_increment->setSize( arrowSize );

		float columnSizes[SW_NumSubWidgets];
		getSizes( columnSizes );

		if( m_label )
			m_label->setSize( Ogre::Vector2( columnSizes[SW_Label], sizeAfterClipping.y ) );
		m_optionLabel->setSize( Ogre::Vector2( columnSizes[SW_OptionLabel], sizeAfterClipping.y ) );

		const float arrowMargin = m_arrowMargin;
		const bool rightToLeft = m_manager->shouldSwapRTL( m_horizDir );

		float colStart = 0.0f;

		if( !rightToLeft )
		{
			if( m_label )
				m_label->setTopLeft( Ogre::Vector2( colStart, 0.0f ) );
			colStart += columnSizes[SW_Label];
			colStart += columnSizes[SW_Space];
		}

		m_decrement->setTopLeft( Ogre::Vector2( colStart + arrowMargin, 0.0f ) );
		m_decrement->setCenter(
			Ogre::Vector2( m_decrement->getCenter().x, m_optionLabel->getCenter().y ) );
		colStart += columnSizes[SW_Decrement];

		m_optionLabel->setTopLeft( Ogre::Vector2( colStart, 0.0f ) );
		colStart += columnSizes[SW_OptionLabel];

		m_increment->setTopLeft( Ogre::Vector2( colStart + arrowMargin, 0.0f ) );
		m_increment->setCenter(
			Ogre::Vector2( m_increment->getCenter().x, m_optionLabel->getCenter().y ) );
		colStart += columnSizes[SW_Increment];

		if( rightToLeft )
		{
			if( m_label )
				m_label->setTopLeft( Ogre::Vector2( colStart, 0.0f ) );
			colStart += columnSizes[SW_Label];
			colStart += columnSizes[SW_Space];
		}

		if( m_label )
			m_label->updateDerivedTransformFromParent( false );
		m_optionLabel->updateDerivedTransformFromParent( false );
		m_decrement->updateDerivedTransformFromParent( false );
		m_increment->updateDerivedTransformFromParent( false );
	}
	//-------------------------------------------------------------------------
	Label *Spinner::getLabel()
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
			calculateSizes();
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
			return m_options[static_cast<size_t>( m_currentValue )];
		}
		else
		{
			char tmpBuffer[64];
			Ogre::LwString numberStr(
				Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
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
				Ogre::LwString msg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
				msg.a( "Invalid range for Spinner::setRange min = ", minValue, " max = ", maxValue );
				logListener->log( msg.c_str(), LogSeverity::Warning );
				COLIBRI_ASSERT_LOW( minValue <= maxValue );
			}
		}
		else
		{
			logListener->log(
				"Invalid call Spinner::setRange. Cannot call this function while "
				"in list-mode (setOption was not empty)",
				LogSeverity::Warning );
			COLIBRI_ASSERT_LOW( false );
		}

		calculateSizes();
		updateOptionLabel();
	}
	//-------------------------------------------------------------------------
	void Spinner::setOptions( const std::vector<std::string> &options )
	{
		m_options = options;
		calculateSizes();
		updateOptionLabel();
	}
	//-------------------------------------------------------------------------
	const std::vector<std::string> &Spinner::getOptions() const { return m_options; }
	//-------------------------------------------------------------------------
	void Spinner::setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir )
	{
		m_horizDir = horizWidgetDir;
		updateOptionLabel();
	}
	//-------------------------------------------------------------------------
	void Spinner::setFixedWidths( bool bAutoCalculate, float labelWidth, float optionsLabelWidth )
	{
		if( m_autoCalcSizes != bAutoCalculate ||
			( bAutoCalculate == false && m_sizeLabel != labelWidth &&
			  m_sizeOptionLabel != optionsLabelWidth ) )
		{
			m_autoCalcSizes = bAutoCalculate;
			m_sizeLabel = labelWidth;
			m_sizeOptionLabel = optionsLabelWidth;
			calculateSizes();
			updateOptionLabel();
		}
	}
	//-------------------------------------------------------------------------
	void Spinner::sizeToFit()
	{
		m_size = Ogre::Vector2::ZERO;

		float sizeLabel, sizeOptionLabel, height;
		calculateSizes( sizeLabel, sizeOptionLabel, height );

		if( m_label )
			height = std::max( height, m_label->getSize().y );
		height = std::max( height, m_arrowSize.y );

		if( !m_autoCalcSizes )
		{
			m_sizeLabel = sizeLabel;
			m_sizeOptionLabel = sizeOptionLabel;
		}

		float columnSizes[SW_NumSubWidgets];
		getSizes( columnSizes );

		Ogre::Vector2 maxSize( Ogre::Vector2( 0.0f, height ) + getBorderCombined() );
		for( size_t i = 0u; i < SW_NumSubWidgets; ++i )
			maxSize.x += columnSizes[i];
		setSize( maxSize );
		// We must set m_minSize too, because if m_increment (or m_decrement if RTL) is hidden
		// then the layouts will shrink the spinner; and then when the spinner changes,
		// either the arrow is out of bounds or the the user recalculates the layout
		// and the whole window changes
		m_minSize = maxSize;
	}
	//-------------------------------------------------------------------------
	void Spinner::setTransformDirty( uint32_t dirtyReason )
	{
		if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) ==
			TransformDirtyScale )
		{
			calculateSizes();
			updateOptionLabel();
		}

		Renderable::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Spinner::notifyWidgetAction( Widget *widget, Action::Action action )
	{
		if( action == Action::PrimaryActionPerform )
		{
			if( widget == m_decrement )
			{
				if( m_manager->swapRTLControls() )
					++m_currentValue;
				else
					--m_currentValue;
				updateOptionLabel();
				_callActionListeners( Action::ValueChanged );
			}
			else if( widget == m_increment )
			{
				if( m_manager->swapRTLControls() )
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
		if( direction == Borders::Left )
		{
			if( m_manager->swapRTLControls() )
				++m_currentValue;
			else
				--m_currentValue;
			updateOptionLabel();
			_callActionListeners( Action::ValueChanged );
		}
		else if( direction == Borders::Right )
		{
			if( m_manager->swapRTLControls() )
				--m_currentValue;
			else
				++m_currentValue;
			updateOptionLabel();
			_callActionListeners( Action::ValueChanged );
		}
	}
}  // namespace Colibri
