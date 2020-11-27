
#pragma once

#include "ColibriGui/ColibriWidget.h"
#include "ColibriGui/Ogre/ColibriOgreRenderable.h"

#include "OgreColourValue.h"

namespace Ogre
{
	struct CbDrawCallStrip;
	struct CbDrawStrip;
	class HlmsColibri;
}

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	struct StateInformation
	{
		//One set of UVs for each cell in the 3x3 grid.
		Ogre::Vector4	uvTopLeftBottomRight[GridLocations::NumGridLocations];
		//Values are in screen pixels, thus they're independent from canvas resolution
		float			borderSize[Borders::NumBorders];
		float			borderRepeatSize[Borders::NumBorders]; /// 0 or negative means disable repeat
		float			centerAspectRatio;
		Ogre::ColourValue defaultColour;
		Ogre::IdString	materialName;
	};

	struct UiVertex
	{
		float x;
		float y;
		uint16_t u;
		uint16_t v;
		uint8_t rgbaColour[4];
		float clipDistance[Borders::NumBorders];
	};

	struct GlyphVertex
	{
		float x;
		float y;
		uint16_t width;
		uint16_t height;
		uint32_t offset;
		uint32_t rgbaColour;
		float clipDistance[Borders::NumBorders];
	};

	/** @ingroup Api_Backend
	@class ApiEncapsulatedObjects
		This structure encapsulates API-specific pointers required for rendering.
		Most or all of the classes inside this structure are forward declared to
		isolate ColibriGui's code from knowing about the 3D API backend as much as
		possible

		It's an argument to Renderable::_addCommands. Whatever happens inside
		Renderable::_addCommands will obviously be very API-specific.
	*/
	struct ApiEncapsulatedObjects
	{
		//Ogre::HlmsColibriGui		*hlms;
		Ogre::HlmsCache const		*lastHlmsCache;
		Ogre::HlmsCache const 		*passCache;
		Ogre::HlmsColibri			*hlms;
		Ogre::uint32				lastVaoName;
		Ogre::CommandBuffer			*commandBuffer;
		Ogre::IndirectBufferPacked	*indirectBuffer;
		uint8_t						*indirectDraw;
		uint8_t						*startIndirectDraw;
		//Used to see if we need to switch to a new draw when rendering text (since text rendering
		//has arbitrary number of of vertices, thus we can't properly calculate the drawId and
		//therefore the material ID)
		Ogre::HlmsDatablock			*lastDatablock;
		int							baseInstanceAndIndirectBuffers;
		Ogre::CbDrawCallStrip		* colibrigui_nullable drawCmd;
		Ogre::CbDrawStrip			* colibrigui_nullable drawCountPtr;
		uint16_t primCount;
		uint32_t basePrimCount[2]; //[0] = regular widgets, [1] = text
		uint32_t nextFirstVertex;
	};

	/**
	@class Renderable
		Renderables are visible. They're actually useful since they can be used to render 2D images
		instead of common widgets such as buttons or edit boxes.

	@remarks
		Renderable must derive from Ogre::ColibriOgreRenderable (or encapsulated class)
		We can only share Vaos & Vertex Buffers, which will be owned by Window.
	*/
	class Renderable : public Widget, public Ogre::ColibriOgreRenderable
	{
	protected:
		StateInformation m_stateInformation[States::NumStates];

		bool              m_overrideSkinColour;
		Ogre::ColourValue m_colour;

	protected:
		/// WARNING: Most of the code assumes m_numVertices is hardcoded to 6*9;
		/// this value is dynamic because certain widgets (such as Labels) can
		/// have arbitrary number of vertices and the rest of the code
		/// also acknowledges that!
		uint32_t			m_numVertices;
		uint32_t			m_currVertexBufferOffset;

		bool				m_visualsEnabled;

	public:
		/// @copydoc Widget::addChildrenCommands
		void _addCommands( ApiEncapsulatedObjects &apiObject, bool collectingBreadthFirst );

	protected:
		inline void addQuad( UiVertex * RESTRICT_ALIAS vertexBuffer,
							 Ogre::Vector2 topLeft,
							 Ogre::Vector2 bottomRight,
							 Ogre::Vector4 uvTopLeftBottomRight,
							 uint8_t *rgbaColour,
							 Ogre::Vector2 parentDerivedTL,
							 Ogre::Vector2 parentDerivedBR,
							 Ogre::Vector2 invSize,
							 float canvasAspectRatio,
							 float invCanvasAspectRatio,
							 Matrix2x3 parentRot );

		virtual void _notifyCanvasChanged() colibri_override;

		virtual void stateChanged( States::States newState ) colibri_override;

	public:
		Renderable( ColibriManager *manager );

		/** Disables drawing this widget, but it is still active. That means you can click on it,
			highlight it, navigate to it via the keyboard, etc; as if everything were normal.

			However the widget won't be drawn on screen. This is mostly useful for Windows,
			as widgets must require at least a root window, and you may not this window to
			have any visual representation (i.e. you just want to render a few widgets
			on screen)
		@param bEnabled
			True to enable rendering this widget (Default).
			False to disable rendering.
		*/
		void setVisualsEnabled( bool bEnabled );
		virtual bool isVisualsEnabled() const colibri_final;

		/** Sets a custom colour
		@param overrideSkinColour
			When false, we ignore 'colour' argument and reset back to using the skin's default

			When true, we always use the set 'colour', regardless of current state.
			Each state may have its own skin colour, hence this overrides all of them
		@param colour
			The colour to use.
			Ignored if overrideSkinColour = false
		*/
		void setColour( bool overrideSkinColour, const Ogre::ColourValue &colour );

		const Ogre::ColourValue &getColour() const;

		bool getOverrideSkinColour() const;

		/** Sets an individual skin (not a pack!) to be used during a specifc state

			A skin pack is instead a collection of different skins, one for each state.
			To set a pack use setSkinPack.

			The main reason to use this function is if you intend to draw UI images, rather
			than buttons or other widgets (e.g. you need to set a particular image, which
			doesn't have disabled/highlighted states), since packs are more suitable for widgets
		@param skinName
			Name of the skin to use, i.e. m_manager->getSkinManager()->getSkins().find( skinName )
		@param forState
			The state to use, use special value States::NumStates to set this skin to all states
		*/
		void setSkin( Ogre::IdString skinName, States::States forState = States::NumStates );

		void setSkinPack( Ogre::IdString skinPackName );

		/** Override's the skin's border size and then calls setClipBordersMatchSkin
		@param borderSize
			The size of all 4 borders
		@param forState
			The state to use, use special value States::NumStates to apply for all states
		@param bClipBordersMatchSkin
			When false, we don't call setClipBordersMatchSkin
		*/
		void setBorderSize( const float borderSize[colibrigui_nonnull Borders::NumBorders],
							States::States forState = States::NumStates,
							bool bClipBordersMatchSkin = true );

		/** Directly set the skins via pointers instead of requiring map lookups.
		@param skinInfo
			Must not be null.
			skinInfo[i] can be null
			skinInfo must be able to hold States::NumStates elements
		*/
		void _setSkinPack( SkinInfo const * colibrigui_nonnull const * colibrigui_nullable skinInfos );

		virtual void setState( States::States state, bool smartHighlight=true,
							   bool broadcastEnable=false ) colibri_override;

		/** Calls setClipBorders and makes the clipping region to match that of the current skin

			IMPORTANT: Skins' border size is dependent on real resolution (not canvas), thus
			this function should be called again and widgets be repositioned (and maybe even resized)
			if you change the real resolution.
		*/
		void setClipBordersMatchSkin();
		void setClipBordersMatchSkin( States::States state );

		virtual void broadcastNewVao( Ogre::VertexArrayObject *vao,
									  Ogre::VertexArrayObject *textVao ) colibri_final;

		virtual bool isRenderable() const colibri_final	{ return true; }

		const StateInformation& getStateInformation( States::States state = States::NumStates ) const;

		inline void _fillBuffersAndCommands( UiVertex * colibrigui_nonnull * colibrigui_nonnull
											 RESTRICT_ALIAS vertexBuffer,
											 GlyphVertex * colibrigui_nonnull * colibrigui_nonnull
											 RESTRICT_ALIAS textVertBuffer,
											 const Ogre::Vector2 &parentPos,
											 const Ogre::Vector2 &parentCurrentScrollPos,
											 const Matrix2x3 &parentRot,
											 const Ogre::Vector2 &currentScrollPos,
											 bool forWindows );
		virtual void _fillBuffersAndCommands( UiVertex * colibrigui_nonnull * colibrigui_nonnull
											  RESTRICT_ALIAS vertexBuffer,
											  GlyphVertex * colibrigui_nonnull * colibrigui_nonnull
											  RESTRICT_ALIAS textVertBuffer,
											  const Ogre::Vector2 &parentPos,
											  const Ogre::Vector2 &parentCurrentScrollPos,
											  const Matrix2x3 &parentRot ) colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
