
#pragma once

#if __clang__
	#define crystalgui_nullable _Nullable
	#define crystalgui_nonnull _Nonnull
	#define CRYSTALGUI_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
	#define CRYSTALGUI_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")
#else
	#define crystalgui_nullable
	#define crystalgui_nonnull
	#define CRYSTALGUI_ASSUME_NONNULL_BEGIN
	#define CRYSTALGUI_ASSUME_NONNULL_END
#endif

namespace Crystal
{
	class Button;
	class CrystalManager;
	class Label;
	class LogListener;
	class Renderable;
	class Shaper;
	class ShaperManager;
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
			SecondaryActionPerform
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
			SecondaryActionPerform	= 1u << Action::SecondaryActionPerform
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
		Window * crystalgui_nullable window;
		Widget * crystalgui_nullable widget;

		FocusPair() : window( 0 ), widget( 0 ) {}
	};
}

namespace Ogre
{
	class CrystalOgreRenderable;
}

// No checks done at all
#define CRYSTALGUI_DEBUG_NONE		0
// We perform basic assert checks and other non-performance intensive checks
#define CRYSTALGUI_DEBUG_LOW		1
// We alter classes to add some debug variables for a bit more thorough checking
// and also perform checks that may cause some performance hit
#define CRYSTALGUI_DEBUG_MEDIUM		2
// We perform intensive validation without concerns for performance
#define CRYSTALGUI_DEBUG_HIGH		3

#define CRYSTALGUI_DEBUG CRYSTALGUI_DEBUG_HIGH

#include "CrystalGui/CrystalAssert.h"

#define CRYSTAL_ASSERT_LOW CRYSTAL_ASSERT

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_MEDIUM
	#define CRYSTAL_ASSERT_MEDIUM CRYSTAL_ASSERT
#else
	#define CRYSTAL_ASSERT_MEDIUM do { CRYSTAL_UNUSED(condition); } while(0)
#endif

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_HIGH
	#define CRYSTAL_ASSERT_HIGH CRYSTAL_ASSERT
#else
	#define CRYSTAL_ASSERT_HIGH do { CRYSTAL_UNUSED(condition); } while(0)
#endif
