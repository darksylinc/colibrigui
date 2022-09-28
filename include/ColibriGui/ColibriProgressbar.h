
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgreId.h"
#include "OgreIdString.h"

namespace Ogre
{
	class HlmsUnlitDatablock;
}

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Progressbar
		Displays a non-interactive progreesbar using two Renderables

		Layer 0 is drawn behind layer 1
	@remarks
		Unlike most widgets, Progressbars only support States::Disabled and States::Idle.
		All other states will look like States::Idle regardless of what the skin pack says

		This class has a tighter integration with Ogre3D (i.e. breaks 3D isolation) due
		to having to clone the material and animate it for every instance of the Progressbar.

		Regular Widgets are better isolated due to being able to share materials,
		and referencing them only by name
	*/
	class Progressbar final : public Widget, Ogre::IdObject
	{
	public:
		enum DisplayType
		{
			/// Layer 0 is the frame
			/// Layer 1 is the progress
			Basic,
			/// Layer 0 is the progress
			/// Layer 1 is the frame
			BehindGlass
		};

	protected:
		Renderable *colibri_nullable m_layers[2];

		bool  m_vertical;
		bool  m_animated;
		float m_progress;

	public:
		float m_animSpeed;
		float m_animLength;

	protected:
		float       m_accumTime;
		DisplayType m_displayType;

		Ogre::HlmsUnlitDatablock *colibri_nullable m_progressLayerDatablock[2];

		/// Stores a hard copy for idle & disabled, with a custom material so we can animate it
		SkinInfo *colibri_nullable m_skinCopy;

		/// Assumes m_displayType has already been set
		void cloneSkinAndDatablock( Ogre::IdString skinPackName,
									const SkinInfo *colibri_nonnull *colibri_nonnull
										outSkinInfos );
		/// Assumes m_displayType has already been set
		void cloneSkinAndDatablock( SkinInfo const *colibri_nonnull const *colibri_nonnull
																						   skinInfos,
									const SkinInfo *colibri_nonnull *colibri_nonnull outSkinInfos,
									const bool                                             bIsAnimated );
		void destroyClonedData();
		void updateProgressbar();

	public:
		Progressbar( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		Renderable *getFrameLayer();
		Renderable *getProgressLayer();

		/// @copydoc Renderable::setVisualsEnabled
		void setVisualsEnabled( bool bEnabled );
		bool isVisualsEnabled() const final;

		/** See Renderable::setSkinPack.
			This version sets the skin packs for both layer 0 and layer 1

			Layer 0's pack contain the skin's setting for m_displayType

		@remarks
			Unlike most widgets, Progressbars only support States::Disabled and States::Idle.
			All other states will look like States::Idle regardless of what the skin pack says
		@param skinPackLayer0Name
		@param skinPackLayer1Name
		*/
		void setSkinPack( Ogre::IdString skinPackLayer0Name, Ogre::IdString skinPackLayer1Name );

		void setVertical( bool bVertical );
		bool getVertical() const { return m_vertical; }

		void        setDisplayType( Progressbar::DisplayType displayType );
		DisplayType getDisplayType() const { return m_displayType; }

		/** Sets the progress of the progress bar.
		@param progress
			Value in range [0.0; 1.0].
			0.0 = empty bar
			1.0 = full bar
		*/
		void  setProgress( float progress );
		float getProgress() const { return m_progress; }

		void setTransformDirty( uint32_t dirtyReason ) final;

		void _update( float timeSinceLast ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
