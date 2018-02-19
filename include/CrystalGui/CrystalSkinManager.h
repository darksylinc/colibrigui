
#pragma once

#include "CrystalGui/CrystalRenderable.h"

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

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
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
	};

	typedef std::map<Ogre::IdString, SkinInfo> SkinInfoMap;
	typedef std::map<Ogre::IdString, SkinPack> SkinPackMap;

	class SkinManager
	{
		CrystalManager	*m_crystalManager;
		SkinInfoMap		m_skins;
		SkinPackMap		m_skinPacks;

		inline Ogre::Vector2 getVector2Array( const rapidjson::Value &jsonArray );
		inline Ogre::Vector4 getVector4Array( const rapidjson::Value &jsonArray );

		inline Ogre::Vector4 widthHeightToBottomLeft( Ogre::Vector4 uvTopLeftWidthHeight,
													  Ogre::Vector2 texResolution );

		void buildGridFromEnclosingUv( Ogre::Vector4 uvTopLeftWidthHeight,
									   const Ogre::Vector2 &texResolution,
									   Ogre::Vector2 borderSize,
									   StateInformation &stateInfo,
									   const char *skinName, const char *filename );

		void loadSkins( const rapidjson::Value &skinsValue, const char *filename );
		void loadSkinPacks( const rapidjson::Value &packsValue, const char *filename );

	public:
		SkinManager( CrystalManager *crystalManager );

		const SkinInfoMap& getSkins() const			{ return m_skins; }
		const SkinPackMap& getSkinPacks() const			{ return m_skinPacks; }

		void loadSkins( const char *fullPath );
		void loadSkins( const char *jsonString, const char *filename );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
