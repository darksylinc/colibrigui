
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
	class CrystalManager;
	class Label;
	class Renderable;
	class SkinManager;
	class Widget;
	class Window;

	namespace States
	{
		enum States
		{
			Disabled,
			Idle,
			Highlighted,
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
}

namespace Ogre
{
	class CrystalOgreRenderable;
}
