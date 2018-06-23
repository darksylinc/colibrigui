/// @file ColibriGuiPrerequisites.h

#pragma once

#if __clang__
	#define colibrigui_nullable _Nullable
	#define colibrigui_nonnull _Nonnull
	#define COLIBRIGUI_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
	#define COLIBRIGUI_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")
	#define colibrigui_likely(x)	__builtin_expect((x),1)
	#define colibrigui_unlikely(x)	__builtin_expect((x),0)
#else
	#define colibrigui_nullable
	#define colibrigui_nonnull
	#define COLIBRIGUI_ASSUME_NONNULL_BEGIN
	#define COLIBRIGUI_ASSUME_NONNULL_END
	#if __GNUC__
		#define colibrigui_likely(x)	__builtin_expect((x),1)
		#define colibrigui_unlikely(x)	__builtin_expect((x),0)
	#else
		#define colibrigui_likely(x)	(x)
		#define colibrigui_unlikely(x)	(x)
	#endif
#endif

#include <stdint.h>
#include <math.h>
#include "math_round.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

/// @defgroup Controls
/// @defgroup Api_Backend
namespace Colibri
{
	class Button;
	struct CachedGlyph;
	class Checkbox;
	class ColibriManager;
	class Label;
	class LogListener;
	class Renderable;
	struct ShapedGlyph;
	class Shaper;
	class ShaperManager;
	struct SkinInfo;
	class SkinManager;
	class Widget;
	class Window;

	namespace Action
	{
		enum Action
		{
			/// Any previous action on this widget was cancelled
			Cancel,
			/// Widget is highlighted by user (e.g. cursor is on top of a button)
			Highlighted,
			/// User is holding enter/main button, but hasn't released it yet
			Hold,
			/// User released the main button and the main action should be performed
			PrimaryActionPerform,
			/// User released the secondary button and that action should be performed
			SecondaryActionPerform,
			/// The value the Widget was holding has changed. For example, Spinners
			ValueChanged
		};
	}
	namespace ActionMask
	{
		enum ActionMask
		{
			Cancel					= 1u << Action::Cancel,
			Highlighted				= 1u << Action::Highlighted,
			Hold					= 1u << Action::Hold,
			PrimaryActionPerform	= 1u << Action::PrimaryActionPerform,
			SecondaryActionPerform	= 1u << Action::SecondaryActionPerform,
			ValueChanged			= 1u << Action::ValueChanged
		};
	}

	namespace States
	{
		enum States
		{
			Disabled,
			Idle,
			HighlightedCursor,
			HighlightedButton,
			HighlightedButtonAndCursor,
			Pressed,
			NumStates
		};
	}
	namespace Borders
	{
		enum Borders
		{
			Top,
			Left,
			Right,
			Bottom,
			NumBorders
		};
	}
	namespace GridLocations
	{
		enum GridLocations
		{
			TopLeft,
			Top,
			TopRight,
			CenterLeft,
			Center,
			CenterRight,
			BottomLeft,
			Bottom,
			BottomRight,
			NumGridLocations
		};
	}
	/*namespace Corners
	{
		enum NumCorners
		{
			TopLeft,
			BottomLeft,
			BottomRight,
			TopRight,
			NumCorners
		};
	}*/

	struct FocusPair
	{
		Window * colibrigui_nullable window;
		Widget * colibrigui_nullable widget;

		FocusPair() : window( 0 ), widget( 0 ) {}
	};

	namespace HorizReadingDir
	{
		enum HorizReadingDir
		{
			/// Use same setting as ShaperManager
			Default,
			/// The base direction depends on the first strong directional character in the
			/// text according to the Unicode Bidirectional Algorithm. If no strong directional
			/// character is present, then set the paragraph level to 1 (LTR or RTL respectively)
			///
			/// For example the text (which 1st strong directional char. is LTR from the h in hello):
			///		hello ARABIC person FARSI
			/// Will be displayed as:
			/// 	hello CIBARA person ISRAF
			///
			/// While the text (which 1st strong directional char. is RTL from A in ARABIC):
			///		ARABIC hello FARSI person
			/// Will be displayed as:
			/// 	person ISRAF hello CIBARA
			AutoLTR,
			AutoRTL,
			/// Text is strongly LTR
			/// The text:
			///		hello ARABIC person FARSI
			/// Will be displayed as:
			/// 	hello CIBARA person ISRAF
			///
			/// While the text
			///		ARABIC hello FARSI person
			/// Will be displayed as:
			/// 	CIBARA hello ISRAF person
			LTR,
			/// Text is strongly RTL
			/// The text:
			///		hello ARABIC person FARSI
			/// Will be displayed as:
			/// 	ISRAF person CIBARA hello
			///
			/// While the text
			///		ARABIC hello FARSI person
			/// Will be displayed as:
			/// 	person ISRAF hello CIBARA
			RTL
		};
	}
	namespace TextHorizAlignment
	{
		enum TextHorizAlignment
		{
			/// When default langauge is LTR, it will display text aligned Left unless the string
			/// is fully RTL.
			/// If the string is mixed, text is aligned to the Left but the reading directions
			/// will be respected for RTL words according to HorizReadingDir.
			/// CJK languages are Left unless overriden by VertReadingDir
			///
			/// When default langauge is RTL, it will display text aligned Right unless the string
			/// is fully LTR.
			/// If the string is mixed, text is aligned to the Right but the reading directions
			/// will be respected for LTR words according to HorizReadingDir.
			Natural,
			Mixed = Natural,
			/// Text always starts from the left. Can be overriden by VertReadingDir
			Left,
			/// Text is always centered
			Center,
			/// Text always starts from the right. Can be overriden by VertReadingDir
			Right
		};
	}
	namespace VertReadingDir
	{
		enum VertReadingDir
		{
			/// Always obey HorizReadingDir & TextHorizAlignment (Default)
			Disabled,
			/// When default language is not CJK, same as Disabled
			/// When default language is CJK, same aligns top to bottom, newlines right to left
			IfNeededTTB,
			/// Aligns top to bottom, newlines right to left, regardless of language
			ForceTTB,
			/// Aligns top to bottom, newlines left to right, regardless of language
			ForceTTBLTR,
			/// Not used
			//IfNeededBTT,
			/// Not used
			//ForceBTT,
		};
	}
	namespace TextVertAlignment
	{
		enum TextVertAlignment
		{
			/// Always equal to Top. It's there just in case there's ever a bottom to top language
			/// Also useful for "use the default" in Label::sizeToFit
			Natural,
			/// Note: Anything other than 'Top' is SLIGHTLY slower for text rendering, as we need
			/// to first calculate the height of the entire text before iterating through
			/// each character to upload position data to GPU.
			Top,
			Center,
			Bottom
		};
	}

	namespace HorizWidgetDir
	{
		enum HorizWidgetDir
		{
			/// Use LTR unless ColibriManager::swapRTLControls returns true
			AutoLTR,
			/// Use RTL unless ColibriManager::swapRTLControls returns true
			AutoRTL,
			/// Always use LTR
			LTR,
			/// Always use RTL
			RTL
		};
	}

	/**
	@class FontSize
		Font sizes are in points, represented by 26.6 fixed point.
		This class allows converting to/from floats for easier handling
	*/
	struct FontSize
	{
		/// Fixed point 26.6
		uint32_t value26d6;

		FontSize() : value26d6( 0u ) {}
		FontSize( uint32_t val ) : value26d6( val ) {}
		FontSize( float val ) : value26d6( 0u )
		{
			fromFloat( val );
		}

		bool operator == ( const FontSize &other ) const
		{ return this->value26d6 == other.value26d6; }
		bool operator != ( const FontSize &other ) const
		{ return this->value26d6 != other.value26d6; }

		void fromFloat( float ptSize )
		{
			value26d6 = static_cast<uint32_t>( round( ptSize * 64.0f ) );
		}
		float asFloat() const
		{
			return value26d6 / 64.0f;
		}
	};

	struct RichText
	{
		FontSize ptSize;
		uint32_t offset;
		uint32_t length;
		HorizReadingDir::HorizReadingDir readingDir;
		uint32_t rgba32;			//Not part of operator ==
		uint32_t backgroundRgba32;	//Not part of operator ==
		uint16_t font;
		bool noBackground;		//Not part of operator ==

		uint32_t glyphStart;	//Not part of operator ==
		uint32_t glyphEnd;		//Not part of operator ==

		bool operator == ( const RichText &other ) const;
	};

	namespace LinebreakMode
	{
		enum LinebreakMode
		{
			/// Words will be broken into the next newline.
			/// Our word-wrap algorithm is quite simple: spaces, tabs, commas and
			/// similar characters are used to identify separation of words. This works
			/// well for popular languages such as western ones (English, Spanish, German, etc)
			/// and some eastern scripts.
			/// However it does not properly perform word breaking for complex languages
			/// such as Thai, Chinese or Japanese; as these would require us to include very
			/// big unicode dictionaries, which goes against ColibriGui's philosophy of
			/// being small.
			WordWrap,
			/// Text outside bounds will disapear
			Clip
		};
	}

	namespace SkinWidgetTypes
	{
		/// KEEP THIS IN SYNC WITH SkinManager::loadDefaultSkinPacks !!!
		enum SkinWidgetTypes
		{
			Window,
			Button,
			Spinner,
			SpinnerBtnDecrement,
			SpinnerBtnIncrement,
			Checkbox,
			CheckboxTickmarkUnchecked,
			CheckboxTickmarkChecked,
			CheckboxTickmarkThirdState,
			NumSkinWidgetTypes
		};
	}
}

namespace Ogre
{
	class ColibriOgreRenderable;
}

COLIBRIGUI_ASSUME_NONNULL_END

// No checks done at all
#define COLIBRIGUI_DEBUG_NONE		0
// We perform basic assert checks and other non-performance intensive checks
#define COLIBRIGUI_DEBUG_LOW		1
// We alter classes to add some debug variables for a bit more thorough checking
// and also perform checks that may cause some performance hit
#define COLIBRIGUI_DEBUG_MEDIUM		2
// We perform intensive validation without concerns for performance
#define COLIBRIGUI_DEBUG_HIGH		3

#define COLIBRIGUI_DEBUG COLIBRIGUI_DEBUG_HIGH

#include "ColibriGui/ColibriAssert.h"

#define COLIBRI_ASSERT_LOW COLIBRI_ASSERT

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
	#define COLIBRI_ASSERT_MEDIUM COLIBRI_ASSERT
#else
	#define COLIBRI_ASSERT_MEDIUM do { COLIBRI_UNUSED(condition); } while(0)
#endif

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_HIGH
	#define COLIBRI_ASSERT_HIGH COLIBRI_ASSERT
#else
	#define COLIBRI_ASSERT_HIGH do { COLIBRI_UNUSED(condition); } while(0)
#endif
