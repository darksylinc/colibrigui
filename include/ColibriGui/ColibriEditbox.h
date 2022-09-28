
#pragma once

#include "ColibriGui/ColibriRenderable.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	namespace InputType
	{
		enum InputType
		{
			Text,
			Multiline,
			Password,
			Email
		};
	}

	/** @ingroup Controls
	@class Editbox
	*/
	class Editbox : public Renderable
	{
		Label                     *m_label;
		Label                     *m_caret;
		Label *colibri_nullable m_secureLabel;
		Label *colibri_nullable m_placeholder;
		/// It's in glyphs
		uint32_t m_cursorPos;

#if defined( __ANDROID__ ) || ( defined( __APPLE__ ) && defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE )
		InputType::InputType m_inputType;
		std::string          m_textHint;
#endif

	public:
		/// When true, multiline prevents Enter key from being
		/// used for activating the Widget's PrimaryAction
		///
		/// @remark	PUBLIC MEMEBER: CAN BE EDITED DIRECTLY
		///
		/// @see	ColibriManager::isTextMultiline
		bool m_multiline;

		float m_blinkTimer;

	protected:
		void showCaret();
		bool requiresActiveUpdate() const;

		void syncSecureLabel();

	public:
		Editbox( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		Label *getLabel();

		Label *colibri_nullable getPlaceholderLabel();

		/// When changing the text programatically, prefer using this function over directly
		/// modifying the Label (via getLabel) because this function will update the caret
		/// cursor position the way the user would expect.
		void setText( const char *text );

		/// Same as doing this->getLabel()->getText()
		const std::string &getText() const;

		/** Sets placeholder text that is only shown while the main text is empty.
		@param text
			Text to use. Can be empty or nullptr to disable the placeholder.
		*/
		void setPlaceholder( const char *colibri_nullable text );

		/// Returns the text from the placeholder. Can be nullptr or empty if there is none
		const std::string *colibri_nullable getPlaceholder() const;

		void setState( States::States state, bool smartHighlight = true ) override;

		/// When true, text is rendered as '*' and Clipboard copy
		/// is forbidden (Clipboard paste is still allowed).
		void setSecureEntry( const bool bSecureEntry );
		bool isSecureEntry() const;

		/// Sets the input type for Android and iOS platforms. This allows backends
		/// to pass on hints for the soft keyboards.
		void setInputType( InputType::InputType inputType, const std::string &textHint );

		/// Returns what was set on setInputType. Note: On some platforms what was set
		/// in setInputType is ignored
		///
		/// If setSecureEntry( true ) was called, it has higher priority and getInputType
		/// will return InputType::Password
		InputType::InputType getInputType() const;

		/// Gets the text hint in setInputType.
		/// On most platforms the hint is discarded and always returns ""
		std::string getTextHint() const;

		void _update( float timeSinceLast ) override;

		void _setTextEdit( const char *text, int32_t selectStart,
						   int32_t selectLength ) override;
		void _setTextSpecialKey( uint32_t keyCode, uint16_t keyMod, size_t repetition ) override;
		void _setTextInput( const char *text, const bool bReplaceContents,
							const bool bCallActionListener = true ) override;

		Ogre::Vector2 _getImeLocation() override;

		bool isTextMultiline() const override;
		bool wantsTextInput() const override;

		void setTransformDirty( uint32_t dirtyReason ) final;

		void _notifyActionKeyMovement( Borders::Borders direction ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
