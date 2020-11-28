
#pragma once

#include "ColibriGui/ColibriRenderable.h"

// Forward declaration for |Document|.
namespace rapidjson
{
	class CrtAllocator;
//	template <typename> class MemoryPoolAllocator;
	template <typename> struct UTF8;
//	template <typename, typename, typename> class GenericDocument;
//	typedef GenericDocument< UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator > Document;

	template <typename BaseAllocator> class MemoryPoolAllocator;
	template <typename Encoding, typename>  class GenericValue;
	typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator> > Value;
}

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	struct SkinInfo
	{
		std::string			name;
		std::string			materialName;
		StateInformation	stateInfo;
	};
	struct SkinPack
	{
		std::string			name;
		Ogre::IdString		skinInfo[States::NumStates];
		/// See Progressbar::DisplayType
		uint8_t progressBarType;
		bool	progressBarIsAnimated;
		float   progressBarAnimSpeed;
		float   progressBarAnimLength;

		float sliderLineSize;
		float sliderHandleProportion[2];
		bool  sliderAlwaysInside;
	};

	typedef std::map<Ogre::IdString, SkinInfo> SkinInfoMap;
	typedef std::map<Ogre::IdString, SkinPack> SkinPackMap;

	class SkinManager
	{
		ColibriManager	*m_colibriManager;
		SkinInfoMap		m_skins;
		SkinPackMap		m_skinPacks;

		inline Ogre::Vector2 getVector2Array( const rapidjson::Value &jsonArray );
		inline Ogre::Vector4 getVector4Array( const rapidjson::Value &jsonArray );

		inline Ogre::Vector4 widthHeightToBottomLeft( Ogre::Vector4 uvTopLeftWidthHeight,
													  Ogre::Vector2 texResolution );

		void buildGridFromEnclosingUv( Ogre::Vector4 uvTopLeftWidthHeight,
									   const Ogre::Vector2 &texResolution,
									   Ogre::Vector2 borderSizeTL, Ogre::Vector2 borderSizeBR,
									   StateInformation &stateInfo,
									   const char *skinName, const char *filename );

		void loadSkins( const rapidjson::Value &skinsValue, const char *filename );
		void loadSkinPacks( const rapidjson::Value &packsValue, const rapidjson::Value &skinsValue,
							const char *filename );
		void loadDefaultSkinPacks( const rapidjson::Value &packsValue, const char *filename );

	public:
		SkinManager( ColibriManager *colibriManager );

		const SkinInfoMap &getSkins() const { return m_skins; }
		const SkinPackMap &getSkinPacks() const { return m_skinPacks; }

		/** Finds the given skinpack by its name. Nullptr if not found
		@param name
			Name of skin pack to search
		@param logSeverity
			The severity to log in case we don't find the skin pack.
		@return
			SkinPack ptr. Nullptr if not found.
		*/
		SkinPack const *colibrigui_nullable findSkinPack(
			Ogre::IdString name, LogSeverity::LogSeverity logSeverity = LogSeverity::Error ) const;

		/** Finds the associated skin for the given State in the input skin pack
			Returns null if not found (i.e. malformed skin pack)
		@param pack
		@param state
		@param logSeverity
			The severity to log in case we don't find the skin.
		@return
			Associated skin for the given state in the given pack.
			Nullptr if not found.
		*/
		SkinInfo const *colibrigui_nullable
						findSkin( const SkinPack &pack, States::States state,
								  LogSeverity::LogSeverity logSeverity = LogSeverity::Warning ) const;

		void loadSkins( const char *fullPath );
		void loadSkins( const char *jsonString, const char *filename );
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
