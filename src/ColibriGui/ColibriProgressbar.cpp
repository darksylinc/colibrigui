
#include "ColibriGui/ColibriProgressbar.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreLwString.h"

namespace Colibri
{
	Progressbar::Progressbar( ColibriManager *manager ) :
		Widget( manager ),
		IdObject( Ogre::Id::generateNewId<Progressbar>() ),
		m_vertical( false ),
		m_animated( false ),
		m_progress( 0.5f ),
		m_animSpeed( 0.0f ),
		m_animLength( 1.0f ),
		m_accumTime( 0.0f ),
		m_displayType( BehindGlass ),
		m_skinCopy( 0 )
	{
		memset( m_layers, 0, sizeof( m_layers ) );
		memset( m_progressLayerDatablock, 0, sizeof( m_progressLayerDatablock ) );
	}
	//-------------------------------------------------------------------------
	void Progressbar::_initialize()
	{
		const Ogre::IdString skinPackName =
			m_manager->getDefaultSkinPackName( SkinWidgetTypes::ProgressbarLayer0 );
		const SkinManager *skinManager = m_manager->getSkinManager();

		const SkinPack *defaultSkinPack = skinManager->findSkinPack( skinPackName, LogSeverity::Fatal );

		if( !defaultSkinPack )
			return;

		m_displayType = static_cast<DisplayType>( defaultSkinPack->progressBarType );
		m_animSpeed = defaultSkinPack->progressBarAnimSpeed;
		m_animLength = defaultSkinPack->progressBarAnimLength;
		m_animated = defaultSkinPack->progressBarIsAnimated;

		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i] = m_manager->createWidget<Renderable>( this );

		{
			// Assign the frame layer's skin
			const SkinWidgetTypes::SkinWidgetTypes skinWidgetTypeFrame =
				m_displayType == Basic ? SkinWidgetTypes::ProgressbarLayer0
									   : SkinWidgetTypes::ProgressbarLayer1;
			Renderable *frameLayer = getFrameLayer();
			frameLayer->_setSkinPack( m_manager->getDefaultSkin( skinWidgetTypeFrame ) );
		}

		{
			// Clone the progress layer's skin and assign that clone
			const SkinWidgetTypes::SkinWidgetTypes skinWidgetTypeProgress =
				m_displayType == Basic ? SkinWidgetTypes::ProgressbarLayer1
									   : SkinWidgetTypes::ProgressbarLayer0;

			SkinInfo const *const *skinInfo = m_manager->getDefaultSkin(
				static_cast<SkinWidgetTypes::SkinWidgetTypes>( skinWidgetTypeProgress ) );

			COLIBRI_ASSERT_LOW( !m_progressLayerDatablock[0] && "_initialize already called!" );

			SkinInfo const *colibrigui_nonnull newSkinInfos[States::NumStates];
			cloneSkinAndDatablock( skinInfo, newSkinInfos, defaultSkinPack->progressBarIsAnimated );

			Renderable *progressLayer = getProgressLayer();
			progressLayer->_setSkinPack( newSkinInfos );
		}

		updateProgressbar();

		Widget::_initialize();

		if( m_animated )
			m_manager->_addUpdateWidget( this );
	}
	//-------------------------------------------------------------------------
	void Progressbar::_destroy()
	{
		if( m_animated )
			m_manager->_removeUpdateWidget( this );

		Widget::_destroy();

		// m_layers[i] are children of us, so they will be destroyed by our super class
		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i] = 0;

		destroyClonedData();
	}
	//-------------------------------------------------------------------------
	Renderable *colibrigui_nullable Progressbar::getFrameLayer()
	{
		const size_t frameLayer = m_displayType == Basic ? 0u : 1u;
		return m_layers[frameLayer];
	}
	//-------------------------------------------------------------------------
	Renderable *colibrigui_nullable Progressbar::getProgressLayer()
	{
		const size_t progressLayer = m_displayType == Basic ? 1u : 0u;
		return m_layers[progressLayer];
	}
	//-------------------------------------------------------------------------
	void Progressbar::setVisualsEnabled( bool bEnabled )
	{
		if( bEnabled != m_layers[0]->isVisualsEnabled() && m_animated )
		{
			if( bEnabled )
				m_manager->_addUpdateWidget( this );
			else
				m_manager->_removeUpdateWidget( this );
		}

		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i]->setVisualsEnabled( bEnabled );
	}
	//-------------------------------------------------------------------------
	bool Progressbar::isVisualsEnabled() const { return m_layers[0]->isVisualsEnabled(); }
	//-------------------------------------------------------------------------
	void Progressbar::setState( States::States state, bool smartHighlight, bool broadcastEnable )
	{
		Widget::setState( state, smartHighlight, broadcastEnable );

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
	void Progressbar::setSkinPack( Ogre::IdString skinPackLayer0Name, Ogre::IdString skinPackLayer1Name )
	{
		SkinManager *skinManager = m_manager->getSkinManager();

		const SkinPack *skinPackL0 = skinManager->findSkinPack( skinPackLayer0Name, LogSeverity::Fatal );
		if( !skinPackL0 )
			return;

		if( m_animated != skinPackL0->progressBarIsAnimated )
		{
			if( m_animated )
				m_manager->_removeUpdateWidget( this );
			if( skinPackL0->progressBarIsAnimated )
				m_manager->_addUpdateWidget( this );
		}

		m_displayType = static_cast<DisplayType>( skinPackL0->progressBarType );
		m_animSpeed = skinPackL0->progressBarAnimSpeed;
		m_animLength = skinPackL0->progressBarAnimLength;
		m_animated = skinPackL0->progressBarIsAnimated;

		{
			// Assign the frame layer's skin
			Ogre::IdString skinPackFrameName =
				m_displayType == Basic ? skinPackLayer0Name : skinPackLayer1Name;
			Renderable *frameLayer = getFrameLayer();
			frameLayer->setSkinPack( skinPackFrameName );
		}

		{
			// Clone the progress layer's skin and assign that clone
			Ogre::IdString skinPackProgressName =
				m_displayType == Basic ? skinPackLayer1Name : skinPackLayer0Name;

			SkinInfo const *colibrigui_nonnull newSkinInfos[States::NumStates];
			cloneSkinAndDatablock( skinPackProgressName, newSkinInfos );

			Renderable *progressLayer = getProgressLayer();
			progressLayer->_setSkinPack( newSkinInfos );
		}
	}
	//-------------------------------------------------------------------------
	void Progressbar::setVertical( bool bVertical )
	{
		m_vertical = bVertical;
		updateProgressbar();
	}
	//-------------------------------------------------------------------------
	void Progressbar::setDisplayType( DisplayType displayType )
	{
		m_displayType = displayType;
		updateProgressbar();
	}
	//-------------------------------------------------------------------------
	void Progressbar::cloneSkinAndDatablock( Ogre::IdString skinPackName,
											 const SkinInfo *colibrigui_nonnull *colibrigui_nonnull
												 outSkinInfos )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinPack *skinPack = skinManager->findSkinPack( skinPackName, LogSeverity::Fatal );
		if( !skinPack )
			return;

		SkinInfo const *colibrigui_nullable skinInfos[States::NumStates];

		for( size_t i = 0u; i < States::NumStates; ++i )
			skinInfos[i] = skinManager->findSkin( *skinPack, static_cast<States::States>( i ) );

		if( !skinInfos[States::Disabled] || !skinInfos[States::Disabled] )
		{
			m_manager->getLogListener()->log(
				"Progressbar::cloneSkinAndDatablock called but skin pack did not specify a skin for "
				"Idle and/or Disabled states for the progress layer. "
				"Progressbar will not look correctly",
				LogSeverity::Warning );
		}
		else
		{
			cloneSkinAndDatablock( skinInfos, outSkinInfos, skinPack->progressBarIsAnimated );
		}
	}
	//-------------------------------------------------------------------------
	void Progressbar::cloneSkinAndDatablock( const SkinInfo *const *skinInfos,
											 SkinInfo const **outSkinInfos, const bool bIsAnimated )
	{
		Ogre::HlmsManager *hlmsManager = m_manager->getOgreHlmsManager();

		// Temporarily assign a different skin since we need to destroy
		// this one (and switching would cause reading dangling pointers)
		getProgressLayer()->_setSkinPack( skinInfos );
		destroyClonedData();

		if( !bIsAnimated )
		{
			memcpy( outSkinInfos, skinInfos, sizeof( SkinInfo * ) * States::NumStates );
			return;
		}

		Ogre::HlmsDatablock *datablocks[2] = {
			hlmsManager->getDatablock( skinInfos[States::Disabled]->stateInfo.materialName ),
			hlmsManager->getDatablock( skinInfos[States::Idle]->stateInfo.materialName ),
		};

		for( size_t i = 0u; i < 2u; ++i )
		{
			char tmpBuffer[128];
			Ogre::LwString matName( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
			matName.a( "Colibri/Progressbar/Clone/", i == 0u ? "Disabled" : "Idle", getId() );

			Ogre::HlmsDatablock *newClone = datablocks[i]->clone( matName.c_str() );

			COLIBRI_ASSERT_HIGH( dynamic_cast<Ogre::HlmsUnlitDatablock *>( newClone ) );

			m_progressLayerDatablock[i] = static_cast<Ogre::HlmsUnlitDatablock *>( newClone );

			Ogre::HlmsSamplerblock samplerblock;
			samplerblock.setAddressingMode( Ogre::TAM_WRAP );
			m_progressLayerDatablock[i]->setSamplerblock( 0u, samplerblock );
			m_progressLayerDatablock[i]->setEnableAnimationMatrix( 0u, true );

			Ogre::Matrix4 animMat( Ogre::Matrix4::IDENTITY );
			animMat.setTrans( Ogre::Vector3( 0, 0, 0 ) );
			animMat.setScale( Ogre::Vector3( m_animLength, 1.0f, 1.0f ) );
			m_progressLayerDatablock[i]->setAnimationMatrix( 0u, animMat );
		}

		memcpy( outSkinInfos, skinInfos, sizeof( SkinInfo * ) * States::NumStates );

		m_skinCopy = new SkinInfo[2];
		memset( m_skinCopy, 0, sizeof( SkinInfo ) * 2u );

		m_skinCopy[0] = *skinInfos[States::Disabled];
		m_skinCopy[1] = *skinInfos[States::Idle];

		for( size_t i = 0u; i < 2u; ++i )
		{
			m_skinCopy[i].materialName = *m_progressLayerDatablock[i]->getNameStr();
			m_skinCopy[i].stateInfo.materialName = m_progressLayerDatablock[i]->getName();
		}

		// Set all states to match States::Idle, except Disabled (see our class' remarks)
		for( size_t i = 0u; i < States::NumStates; ++i )
			outSkinInfos[i] = &m_skinCopy[1];
		outSkinInfos[States::Disabled] = &m_skinCopy[0];
	}
	//-------------------------------------------------------------------------
	void Progressbar::destroyClonedData()
	{
		for( size_t i = 0u; i < 2u; ++i )
		{
			if( m_progressLayerDatablock[i] )
			{
				Ogre::Hlms *creator = m_progressLayerDatablock[i]->getCreator();
				creator->destroyDatablock( m_progressLayerDatablock[i]->getName() );
				m_progressLayerDatablock[i] = 0;
			}
		}

		delete[] m_skinCopy;
		m_skinCopy = 0;
	}
	//-------------------------------------------------------------------------
	void Progressbar::updateProgressbar()
	{
		if( !m_layers[0] )
			return;  //_initialize hasn't been called yet

		const size_t frameLayer = m_displayType == Basic ? 0u : 1u;
		const size_t progressLayer = m_displayType == Basic ? 1u : 0u;

		const Ogre::Vector2 frameSize = getSize();

		m_layers[frameLayer]->setSize( frameSize );

		Ogre::Vector2 progressFraction( 1.0f );
		progressFraction[m_vertical] = m_progress;

		Ogre::Vector2 progressSize = frameSize * progressFraction;
		if( m_displayType == Basic )
			progressSize -= m_layers[frameLayer]->getBorderCombined();

		m_layers[progressLayer]->setSize( progressSize );

		const bool rightToLeft = m_manager->shouldSwapRTL( HorizWidgetDir::AutoLTR );

		if( rightToLeft || m_vertical )
		{
			if( m_displayType == Basic )
			{
				m_layers[progressLayer]->setTopLeft(
					frameSize - ( progressSize + m_layers[frameLayer]->getBorderTopLeft() ) );
			}
			else
				m_layers[progressLayer]->setTopLeft( frameSize - progressSize );
		}
		else
		{
			if( m_displayType == Basic )
				m_layers[progressLayer]->setTopLeft( m_layers[frameLayer]->getBorderTopLeft() );
			else
				m_layers[progressLayer]->setTopLeft( Ogre::Vector2::ZERO );
		}

		for( size_t i = 0u; i < 2u; ++i )
			m_layers[i]->updateDerivedTransformFromParent( false );
	}
	//-------------------------------------------------------------------------
	void Progressbar::setProgress( float progress )
	{
		progress = Ogre::Math::Clamp( progress, 0.0f, 1.0f );

		if( m_progress != progress )
		{
			m_progress = progress;
			updateProgressbar();
		}
	}
	//-------------------------------------------------------------------------
	void Progressbar::setTransformDirty( uint32_t dirtyReason )
	{
		// Only update if our size is directly being changed, not our parent's
		if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) ==
			TransformDirtyScale )
		{
			updateProgressbar();
		}

		Widget::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Progressbar::_update( float timeSinceLast )
	{
		if( m_currentState == States::Disabled )
			return;

		Ogre::Matrix4 animMat;
		animMat.makeTransform( Ogre::Vector3( -m_accumTime, 0, 0 ),
							   Ogre::Vector3( m_animLength * m_progress, 1.0f, 1.0f ),
							   Ogre::Quaternion::IDENTITY );

		if( m_vertical )
		{
			Ogre::Matrix4 animMat2;
			animMat2.makeTransform(
				Ogre::Vector3::ZERO, Ogre::Vector3::UNIT_SCALE,
				Ogre::Quaternion( Ogre::Radian( (float)M_PI / 2.0f ), Ogre::Vector3::UNIT_Z ) );
			animMat = animMat * animMat2;
		}

		for( size_t i = 0u; i < 2u; ++i )
			m_progressLayerDatablock[i]->setAnimationMatrix( 0u, animMat );

		m_accumTime += timeSinceLast * m_animSpeed * m_animLength;
		float intPart;
		m_accumTime = modff( m_accumTime, &intPart );
	}
}  // namespace Colibri
