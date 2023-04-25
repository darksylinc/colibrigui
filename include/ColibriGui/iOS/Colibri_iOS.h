#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

namespace Colibri
{
	COLIBRI_ASSUME_NONNULL_BEGIN

	/** Shows a native UITextField with a keyboard, then sends everything to Colibri
		and deletes the UITextField.

		Example:

		@code
			class MyColibriListener final : public Colibri::ColibriListener
			{
				void showTextInput( Colibri::Editbox *editbox ) override
				{
					iOS_showTextInput( CFBridgingRetain( uiView ), editbox );
				}
			};
		@endcode
	@param _uiView
		Parent UIView
	@param editbox
		Editbox. Can safely become dangling pointer after we return.
	*/
	void iOS_showTextInput( const void *_uiView, Editbox *editbox );

	COLIBRI_ASSUME_NONNULL_END
}  // namespace Colibri
