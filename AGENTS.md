# ColibriGui - AI Assistant Guide

## Project Overview

**ColibriGui** is a cross-platform 2D UI framework built on top of Ogre3D, designed primarily for games but suitable for industrial HMI and embedded systems. It provides a lightweight, flexible UI solution with touch, keyboard, and gamepad support.

### Key Characteristics
- **Primary Use Case**: Game UI development
- **Rendering Backend**: Ogre3D
- **Cross-Platform**: Windows, macOS, Linux, Android, iOS
- **Input Support**: Mouse, touch, keyboard, gamepad
- **Threading**: **NOT thread-safe** - All UI operations must occur on the main thread

---

## Architecture

### Core Classes

| Class | Description |
|-------|-------------|
| `ColibriManager` | Central manager for widget lifecycle, input handling, rendering buffer preparation, focus management |
| `Widget` | Base class for all UI elements with hierarchy, navigation, and transform systems |
| `Renderable` | Base class for drawable widgets |
| `Window` | Container for widgets with scroll support |

### Widget Types

| Widget | Description |
|--------|-------------|
| `Label` / `LabelBmp` | Text rendering with rich text support |
| `Button` | Interactive button |
| `Editbox` | Text input field |
| `Checkbox` | Toggle with three states (checked, unchecked, third) |
| `ToggleButton` | Two-state toggle button |
| `Spinner` | Value selector with increment/decrement buttons |
| `Slider` | Horizontal/vertical value slider |
| `Progressbar` | Progress indicator (dual-layer) |
| `GraphChart` | Line/bar graph visualization |
| `RadarChart` | Spider/radar chart visualization |
| `CustomShape` | Custom vertex-based shapes |

---

## API Reference

### ColibriManager

The central entry point for all UI operations:

```cpp
// Initialization (pass logListener and colibriListener)
// multipass = false, bSecondary = false by default
ColibriManager* manager = new ColibriManager(logListener, colibriListener);

// Set Ogre objects (must be called after construction)
manager->setOgre(root, vaoManager, sceneManager);

// RTL support (swap left/right for Arabic/Hebrew)
manager->setSwapRTLControls(true);

// Main loop integration
manager->update(deltaTime); // Update widgets (call each frame)
manager->prepareRenderCommands();
manager->render();

// Widget creation. Makes the widget child of window
Window* window = manager->createWindow(parentWindow);
Widget* widget = manager->createWidget<Colibri::Button>( window );

// Focus management
widget->setKeyboardFocus();
```

### Widget Base Class

```cpp
class Widget {
public:
    // Hierarchy
    void addChild(Widget* child);
    Widget* getChild(size_t index);
    Widget* getParent() const;
    void removeFromParent();
    
    // Transform (Ogre::Vector2 for positions/sizes)
    void setTransform(const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size);
    void setTopLeft(const Ogre::Vector2 &topLeft);
    void setSize(const Ogre::Vector2 &size);
    void setOrientation(const Ogre::Vector4 &orientation); // 2x2 rotation matrix
    void setOrientation(const Ogre::Radian rotationAngle);
    void setCenter(const Ogre::Vector2 &center);
    
    // State management
    void setState(States::States state, bool smartHighlight=true);
    States::States getCurrentState() const;
    bool isDisabled() const;
    void setClickable(bool clickable);
    bool getClickable() const;
    void setKeyboardNavigable(bool navigable);
    bool getKeyboardNavigable() const;
    
    // Visibility
    void setHidden(bool hidden);
    bool isHidden() const;
    
    // Rendering mode
    bool m_breadthFirst; // Public member: render parents first (faster but may cause overlap issues)
    bool isUltimatelyBreadthFirst() const;
    
    // Z-order
    void setZOrder(uint8_t z);
    uint8_t getZOrder() const;
    
    // Focus & Navigation
    void setKeyboardFocus();
    void setNextWidget(Widget* nextWidget, Borders::Borders direction, bool reciprocate=true);
    Widget* getNextKeyboardNavigableWidget(const Borders::Borders direction);
    
    // Event listener
    void addActionListener(WidgetActionListener* listener, uint32_t actionMask=~0u);
    void removeActionListener(WidgetActionListener* listener, uint32_t actionMask=~0u);
    
    // Other
    void setDebugName(const std::string& debugName);
    const std::string& _getDebugName() const;
};
```

### ActionListener

```cpp
class WidgetActionListener {
public:
    virtual void notifyWidgetAction(Widget* widget, Action::Action action) = 0;
};

// Actions (Action::Action enum):
// - Cancel: Previous action was cancelled
// - Highlighted: Widget is highlighted by user (cursor/button)
// - Hold: User is holding enter/main button
// - PrimaryActionPerform: User released main button, perform action
// - SecondaryActionPerform: User released secondary button
// - ValueChanged: Widget value changed (for Spinner, Slider, etc)

// Action masks (ActionMask::ActionMask enum):
// - Cancel
// - Highlighted
// - Hold
// - PrimaryActionPerform
// - SecondaryActionPerform
// - ValueChanged
```

### Window

```cpp
class Window : public Widget {
public:
    // Scroll visibility
    void setScrollVisible(bool bVisible, Borders::Borders border = Borders::NumBorders);
    bool getScrollVisible(Borders::Borders border) const;
    
    // Scroll control
    void setScrollAnimated(const Ogre::Vector2 &nextScroll, bool animateOutOfRange);
    void setScrollImmediate(const Ogre::Vector2 &scroll);
    bool setKeyScroll(const Ogre::Vector2 &scrollAmount, bool animated=true); // For gamepad
    
    // Scroll state
    const Ogre::Vector2& getCurrentScroll() const;
    const Ogre::Vector2& getNextScroll() const;
    void setMaxScroll(const Ogre::Vector2 &maxScroll);
    const Ogre::Vector2& getMaxScroll() const;
    void setScrollableArea(const Ogre::Vector2 &scrollableArea);
    const Ogre::Vector2& getScrollableArea() const;
    bool hasScroll() const;
    bool hasScrollX() const;
    bool hasScrollY() const;
    
    // Cursor consumption
    void setConsumeCursor(bool bConsumeCursor);
    bool getConsumeCursor() const;
    
};
```

### ColibriManager Destruction Methods

```cpp
class ColibriManager {
public:
    // Destruction
    void destroyWindow(Window* window);
    void destroyWidget(Widget* widget);
};
```

### Label

```cpp
class Label : public Renderable {
public:
    // Text content
    void setText(const std::string& text);
    std::string getText() const;
    
    // Font settings
    void setDefaultFontSize(FontSize defaultFontSize); // Use FontSize helper class for 26.6 fixed point
    void setDefaultFont(uint16_t defaultFont);
    
    // Alignment
    void setTextHorizAlignment(TextHorizAlignment::TextHorizAlignment horiz);
    TextHorizAlignment::TextHorizAlignment getTextHorizAlignment() const;
    void setTextVertAlignment(TextVertAlignment::TextVertAlignment vert);
    TextVertAlignment::TextVertAlignment getTextVertAlignment() const;
    
    // Reading direction
    void setReadingDir(HorizReadingDir::HorizReadingDir horizDir, VertReadingDir::VertReadingDir vertDir);
    
    // Rich text
    void parseRichText(const std::string& richText);
    
    // Line breaking
    void setLinebreakMode(LinebreakMode::LinebreakMode linebreakMode); // WordWrap or Clip
    LinebreakMode::LinebreakMode getLinebreakMode() const;
    
    // Sizing
    void sizeToFit();
    
    // Renderable overrides
    bool isVisualsEnabled() const;
    void setVisualsEnabled(bool enabled);
};
```

### SkinWidgetTypes

```cpp
namespace SkinWidgetTypes {
    enum SkinWidgetTypes {
        Window,
        Button,
        Spinner,
        SpinnerBtnDecrement,
        SpinnerBtnIncrement,
        Checkbox,
        CheckboxTickmarkUnchecked,
        CheckboxTickmarkChecked,
        CheckboxTickmarkThirdState,
        Editbox,
        ProgressbarLayer0,
        ProgressbarLayer1,
        SliderLine,
        SliderHandle,
        ToggleButtonUnchecked,
        ToggleButtonChecked,
        WindowArrowScrollTop,
        WindowArrowScrollLeft,
        WindowArrowScrollRight,
        WindowArrowScrollBottom,
        RadarChart,
        GraphChart,
        NumSkinWidgetTypes // Sentinel value
    };
}
```

---

## Integration with Game Engine

### Main Loop Pattern

```cpp
class Game {
    ColibriManager* colibriManager;
    
public:
    void init() {
        // Create log listener and colibri listener (optional, can be nullptr)
        MyLogListener* logListener = new MyLogListener();
        MyColibriListener* colibriListener = new MyColibriListener();
        
        // Initialize ColibriManager with listeners
        colibriManager = new ColibriManager(logListener, colibriListener);
        
        // Set Ogre objects (must be done after construction)
        colibriManager->setOgre(root, vaoManager, sceneManager);
        
        // Configure canvas size
        colibriManager->setCanvasSize(Ogre::Vector2(1920, 1080), Ogre::Vector2(screenWidth, screenHeight));
    }
    
    void update(float deltaTime) {
        // Update UI widgets
        colibriManager->update(deltaTime);

        // IMPORTANT: This is handled by CompositorPassColibriGui.cpp and should not be
        // called directly unless you're writing the rendering glue.
        colibriManager->prepareRenderCommands();
        colibriManager->render();
    }
    
    // Input callbacks (integrate with your input system)
    void keyPressed(int keyCode, int keyMod) {
        // For text input
        if (keyMod == 0) {
            colibriManager->setTextSpecialKeyPressed(static_cast<uint32_t>(keyCode), keyMod);
        } else {
            colibriManager->setKeyDirectionPressed(static_cast<ColibriManager::Borders>(keyCode));
        }
    }
    
    void keyReleased(int keyCode, int keyMod) {
        if (keyMod == 0) {
            colibriManager->setTextSpecialKeyReleased(static_cast<uint32_t>(keyCode), keyMod);
        } else {
            colibriManager->setKeyDirectionReleased(static_cast<ColibriManager::Borders>(keyCode));
        }
    }
    
    void mouseMoved(float x, float y) {
        // Convert to canvas coordinates
        Ogre::Vector2 canvasPos = /* convert from screen to canvas */;
        colibriManager->setMouseCursorMoved(canvasPos);
    }
    
    void mousePressed(int button) {
        colibriManager->setMouseCursorPressed(allowScrollGesture, alwaysAllowScroll);
        colibriManager->setKeyboardPrimaryPressed();
    }
    
    void mouseReleased(int button) {
        colibriManager->setMouseCursorReleased();
        colibriManager->setKeyboardPrimaryReleased();
    }
    
    void touchStarted(int x, int y) {
        Ogre::Vector2 canvasPos = /* convert from screen to canvas */;
        colibriManager->setMouseCursorMoved(canvasPos);
        colibriManager->setMouseCursorPressed(true, true);
    }
    
    void touchEnded(int x, int y) {
        colibriManager->setMouseCursorReleased();
    }
};
```

### Input Handling Reference

See `Examples/MainDemo/src/ColibriGuiGameState.cpp` for complete input integration.

---

## Skinning System

### Skins.json Format

Skins are defined in JSON format. See `Docs/Skins.json` for the schema.

**Key Structure:**
```json
{
    "skinName": {
        "skinType": {
            "slices": [ ... ],
            "borders": {
                "top": { "slice": X },
                "left": { "slice": Y },
                ...
            }
        }
    }
}
```

### Skins vs SkinPacks

A skin merely dictates how a Renderable should look in a particular state.

A skin pack is a collection of skins for a specific Widget (e.g. Button, Checkbox, etc) where there is one skin for each state (e.g. Disabled, Idle, Highlighted, etc)

Each type of Widget (Button, Checkbox, etc) has a default skin pack that defined in Skins.json that gets applied upon creation.

```cpp
// Load custom skin pack
skinManager->loadSkinPack("MySkin", "path/to/Skins.json");

// Apply the skin pack for the widget
widget->setSkinPack("MySkin", state);

// Apply skin to widget to a specific state, overriding the skin pack because it's called later.
widget->setSkin("MySkin", state);

// Set default skin packs for all widgets
std::string defaultSkins[SkinWidgetTypes::NumSkinWidgetTypes];
defaultSkins[SkinWidgetTypes::Button] = "MyButtonSkin";
manager->setDefaultSkins(defaultSkins);
```

---

## Layout System

There are three layout classes:

1. **LayoutLine**: Extremely similar to CSS flexbox
2. **LayoutMultiline**: Like LayoutLine but makes sure multiple rows (or columns if m_vertical = false) end up all the same size
3. **LayoutTableSameSize**: Extremely similar to a CSS Grid

---

## Text & Internationalization

### Reading Directions

```cpp
// Horizontal text direction
HorizReadingDir::AutoLTR   // Auto-detect from first strong char (LTR default)
HorizReadingDir::AutoRTL   // Auto-detect from first strong char (RTL default)
HorizReadingDir::LTR       // Strong left-to-right
HorizReadingDir::RTL       // Strong right-to-left

// Vertical text direction (for CJK)
VertReadingDir::Disabled        // Obey horizontal settings
VertReadingDir::IfNeededTTB     // If needed, top-to-bottom, newlines right-to-left
VertReadingDir::ForceTTB        // Top-to-bottom, newlines right-to-left
VertReadingDir::ForceTTBLTR     // Top-to-bottom, newlines left-to-right
```

### Line Breaking Modes

```cpp
LinebreakMode::WordWrap  // Words break to next line
LinebreakMode::Clip      // Text outside bounds disappears
```

---

## Coding Standards

### Formatting

- **Indent**: 4 spaces. Use tabs.
- **Column Limit**: 105 characters
- **Braces**: K&R style (after function/class)
- **Pointers**: `type* ptr` (pointer aligns right)
- **Namespace**: Indented

Run `clang-format -i` on modified files.

### Headers

```cpp
#pragma once

#include <dependencies>

namespace Colibri {

// Forward declarations
class Foo;
class Bar;

// Enums
enum class Baz { ... };

// Structs/Classes
class Foo {
    // ...
};

}  // namespace Colibri
```

---

## Build System

### Prerequisites

- CMake 3.10+
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Ogre3D
- HarfBuzz (text shaping)
- FreeType (font rasterization)
- ICU (internationalization)

### Build Commands

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

---

## Important Notes for AI Agents

1. **Thread Safety**: All UI operations must occur on the main thread. No concurrent access to widgets.

2. **Memory Management**: ColibriManager owns all created widgets. Do not manually delete them.

3. **Coordinate System**: All positions and sizes are in virtual canvas. How the virtual canvas translates to display depends on the renderer and `ColibriManager::setCanvasSize`.

4. **Fixed Point**: Font sizes use 26.6 fixed point format internally. Use `FontSize` helper class or pass raw `uint32_t` value.

5. **State Machine**: Widgets have 6 states (not counting NumStates sentinel):
   - `States::Disabled`
   - `States::Idle`
   - `States::HighlightedCursor`
   - `States::HighlightedButton`
   - `States::HighlightedButtonAndCursor`
   - `States::Pressed`

6. **Render Mode**: Choose between `BreadthFirst` (render parents first, faster but can cause issues if Widgets with children overlap) or `DepthFirst` (render children first) for correct Z-ordering. Set via `Widget::m_breadthFirst`.

7. **RTL Support**: Use `ColibriManager::setSwapRTLControls(true)` to globally swap left/right controls for Arabic/Hebrew interfaces.

8. **Focus Navigation**: Keyboard navigation uses tab order. Call `setKeyboardFocus()` on widgets that should receive keyboard focus. Widgets must also be keyboard navigable via `setKeyboardNavigable(true)`.

9. **No Testing Framework**: This project has no unit tests. Verify functionality through examples in `Examples/` directory.

10. **Ogre Integration**: `ColibriManager::setOgre()` must be called after construction to provide Ogre objects for rendering.

---

## Resources

- **Examples**: `Examples/MainDemo/`, `Examples/OffScreenCanvas2D/`, `Examples/OffScreenCanvas3D/`
- **Documentation**: `Docs/AndroidIntegration.md`, `Docs/Skins.json`
- **Header Files**: `include/ColibriGui/`
- **Implementation**: `src/ColibriGui/`