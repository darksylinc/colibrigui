
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

	public:
		Button( ColibriManager *manager );

		virtual void _initialize();
		virtual void _destroy();

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
