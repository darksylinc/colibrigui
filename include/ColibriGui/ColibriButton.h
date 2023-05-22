
#pragma once

#include "ColibriGui/ColibriRenderable.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Button
	*/
	class Button : public Renderable
	{
		Label	*m_label;

		Ogre::Vector2 m_labelTopLeftMargin;
		Ogre::Vector2 m_labelBottomRightMargin;

	public:
		Button( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		/** By default getLabel()->m_margin can be used to set both top left & bottom right
			margins. This setting allows to control them separately, which are ADDED on top of
			getLabel()->m_margin.

			This setting is useful when left- or right-aligning text and having an icon
			(not just text). Checkboxes in BigButton mode are such an example.
		@param labelTopLeftMargin
			Text margin on the top and left side
		@param labelBottomRightMargin
			Text margin on the bottom and right side
		*/
		void setLabelMargins( const Ogre::Vector2 &labelTopLeftMargin,
							  const Ogre::Vector2 &labelBottomRightMargin );

		/// Returns true if getLabel() has been already called at least once.
		bool hasLabel() const;

		/// Returns the Label. Creates one if none exists. Never returns a nullptr.
		Label* getLabel();

		/// @copydoc Label::sizeToFit
		void sizeToFit( float maxAllowedWidth=std::numeric_limits<float>::max(),
						TextHorizAlignment::TextHorizAlignment newHorizPos=TextHorizAlignment::Left,
						TextVertAlignment::TextVertAlignment newVertPos=TextVertAlignment::Top,
						States::States baseState=States::NumStates );

		void setTransformDirty( uint32_t dirtyReason ) final;
	};
}

COLIBRI_ASSUME_NONNULL_END
