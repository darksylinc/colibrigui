
#include "CrystalGui/CrystalSkinManager.h"
#include "CrystalGui/CrystalManager.h"

#include "OgreLwString.h"
#include "rapidjson/document.h"

#include <fstream>

namespace Crystal
{
	SkinManager::SkinManager( CrystalManager *crystalManager ) :
		m_crystalManager( crystalManager )
	{
	}
	//-------------------------------------------------------------------------
	inline Ogre::Vector2 SkinManager::getVector2Array( const rapidjson::Value &jsonArray )
	{
		Ogre::Vector2 retVal( Ogre::Vector2::ZERO );

		const rapidjson::SizeType arraySize = std::min( 2u, jsonArray.Size() );
		for( rapidjson::SizeType i=0; i<arraySize; ++i )
		{
			if( jsonArray[i].IsNumber() )
				retVal[i] = static_cast<float>( jsonArray[i].GetDouble() );
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	inline Ogre::Vector4 SkinManager::getVector4Array( const rapidjson::Value &jsonArray )
	{
		Ogre::Vector4 retVal( Ogre::Vector4::ZERO );

		const rapidjson::SizeType arraySize = std::min( 4u, jsonArray.Size() );
		for( rapidjson::SizeType i=0; i<arraySize; ++i )
		{
			if( jsonArray[i].IsNumber() )
				retVal[i] = static_cast<float>( jsonArray[i].GetDouble() );
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	inline Ogre::Vector4 SkinManager::widthHeightToBottomLeft( Ogre::Vector4 uvTopLeftWidthHeight,
														Ogre::Vector2 texResolution )
	{
		uvTopLeftWidthHeight.x /= texResolution.x;
		uvTopLeftWidthHeight.y /= texResolution.y;
		uvTopLeftWidthHeight.z /= texResolution.x;
		uvTopLeftWidthHeight.w /= texResolution.y;

		return Ogre::Vector4( uvTopLeftWidthHeight.x, uvTopLeftWidthHeight.y,
							  uvTopLeftWidthHeight.x + uvTopLeftWidthHeight.z,
							  uvTopLeftWidthHeight.y + uvTopLeftWidthHeight.w );
	}
	//-------------------------------------------------------------------------
	void SkinManager::loadSkins( const rapidjson::Value &skinsValue, const char *filename )
	{
		LogListener *log = m_crystalManager->getLogListener();
		char tmpBuffer[512];
		Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

		rapidjson::Value::ConstMemberIterator itor = skinsValue.MemberBegin();
		rapidjson::Value::ConstMemberIterator end  = skinsValue.MemberEnd();

		while( itor != end )
		{
			if( itor->name.IsString() && itor->value.IsObject() )
			{
				Ogre::Vector2 texResolution( 1.0f, 1.0f );
				SkinInfo skinInfo;
				memset( &skinInfo.stateInfo, 0, sizeof(skinInfo.stateInfo) );
				skinInfo.stateInfo.uvTopLeftBottomRight[GridLocations::Center] =
						Ogre::Vector4( 0, 0, 1, 1 );

				skinInfo.name = itor->name.GetString();
				const rapidjson::Value &skinValue = itor->value;

				rapidjson::Value::ConstMemberIterator itTmp;

				itTmp = skinValue.FindMember( "tex_resolution" );
				if( itTmp != skinValue.MemberEnd() &&
					itTmp->value.IsArray() &&
					itTmp->value.Size() == 2u &&
					itTmp->value[0].IsUint() && itTmp->value[1].IsUint() )
				{
					texResolution.x = itTmp->value[0].GetUint();
					texResolution.y = itTmp->value[1].GetUint();
				}

				itTmp = skinValue.FindMember( "material" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsString() )
				{
					skinInfo.materialName = itTmp->value.GetString();
					skinInfo.stateInfo.materialName = skinInfo.materialName;
				}

				itTmp = skinValue.FindMember( "grid_uv" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsObject() )
				{
					const rapidjson::Value &gridValue = itTmp->value;

					itTmp = gridValue.FindMember( "all" );
					if( itTmp != skinValue.MemberEnd() && itTmp->value.IsArray() )
					{
						const Ogre::Vector4 uvTopLeftWidthHeight = getVector4Array( itTmp->value );
						const Ogre::Vector4 uvTopLeftBottomRight =
								widthHeightToBottomLeft( uvTopLeftWidthHeight, texResolution );
						for( size_t i=0; i<GridLocations::NumGridLocations; ++i )
							skinInfo.stateInfo.uvTopLeftBottomRight[i] = uvTopLeftBottomRight;
					}
					const char *gridLocations[GridLocations::NumGridLocations] =
					{
						"top_left",
						"top",
						"top_right",
						"center_left",
						"center",
						"center_right",
						"bottom_left",
						"bottom",
						"bottom_right"
					};

					for( size_t i=0; i<GridLocations::NumGridLocations; ++i )
					{
						itTmp = gridValue.FindMember( gridLocations[i] );
						if( itTmp != skinValue.MemberEnd() && itTmp->value.IsArray() )
						{
							const Ogre::Vector4 uvTopLeftWidthHeight = getVector4Array( itTmp->value );
							const Ogre::Vector4 uvTopLeftBottomRight =
									widthHeightToBottomLeft( uvTopLeftWidthHeight, texResolution );
							skinInfo.stateInfo.uvTopLeftBottomRight[i] = uvTopLeftBottomRight;
						}
					}
				}

				itTmp = skinValue.FindMember( "borders" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsObject() )
				{
					const rapidjson::Value &gridValue = itTmp->value;

					itTmp = gridValue.FindMember( "all" );
					if( itTmp != skinValue.MemberEnd() )
					{
						Ogre::Vector2 borderSizeRepeat( Ogre::Vector2::ZERO );
						if( itTmp->value.IsArray() )
							borderSizeRepeat = getVector2Array( itTmp->value );
						else if( itTmp->value.IsNumber() )
							borderSizeRepeat = static_cast<float>( itTmp->value.GetDouble() );

						for( size_t i=0; i<Borders::NumBorders; ++i )
						{
							skinInfo.stateInfo.borderSize[i] = borderSizeRepeat.x;
							skinInfo.stateInfo.borderRepeatSize[i] = borderSizeRepeat.y;
						}
					}

					const char *borders[Borders::NumBorders] =
					{
						"top",
						"left",
						"right",
						"bottom",
					};

					for( size_t i=0; i<Borders::NumBorders; ++i )
					{
						itTmp = gridValue.FindMember( borders[i] );
						if( itTmp != skinValue.MemberEnd() )
						{
							Ogre::Vector2 borderSizeRepeat( Ogre::Vector2::ZERO );
							if( itTmp->value.IsArray() )
								borderSizeRepeat = getVector2Array( itTmp->value );
							else if( itTmp->value.IsNumber() )
								borderSizeRepeat = static_cast<float>( itTmp->value.GetDouble() );

							skinInfo.stateInfo.borderSize[i] = borderSizeRepeat.x;
							skinInfo.stateInfo.borderRepeatSize[i] = borderSizeRepeat.y;
						}
					}
				}

				if( skinInfo.materialName.empty() )
				{
					errorMsg.clear();
					errorMsg.a( "[SkinManager::loadSkins]: No material defined for skin ",
								itor->name.GetString(), " in ", filename );
					log->log( errorMsg.c_str(), LogSeverity::Warning );
				}
				else
				{
					m_skins[skinInfo.name] = skinInfo;
				}
			}

			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void SkinManager::loadSkinPacks( const rapidjson::Value &packsValue, const char *filename )
	{
		rapidjson::Value::ConstMemberIterator itor = packsValue.MemberBegin();
		rapidjson::Value::ConstMemberIterator end  = packsValue.MemberEnd();

		while( itor != end )
		{
			if( itor->name.IsString() && itor->value.IsObject() )
			{
				SkinPack skinPack;

				skinPack.name = itor->name.GetString();
				const rapidjson::Value &skinValue = itor->value;

				rapidjson::Value::ConstMemberIterator itTmp;

				itTmp = skinValue.FindMember( "all" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsString() )
				{
					skinPack.skinInfo[0] = itTmp->value.GetString();
					for( size_t i=1u; i<States::NumStates; ++i )
						skinPack.skinInfo[i] = skinPack.skinInfo[0];
				}

				const char *states[States::NumStates] =
				{
					"disabled",
					"idle",
					"highlighted",
					"pressed"
				};
				for( size_t i=0u; i<States::NumStates; ++i )
				{
					itTmp = skinValue.FindMember( states[i] );
					if( itTmp != skinValue.MemberEnd() && itTmp->value.IsString() )
						skinPack.skinInfo[i] = itTmp->value.GetString();
				}

				m_skinPacks[skinPack.name] = skinPack;
			}

			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void SkinManager::loadSkins( const char *fullPath )
	{
		LogListener *log = m_crystalManager->getLogListener();
		char tmpBuffer[512];
		Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

		std::ifstream inFile( fullPath, std::ios::in | std::ios::binary );

		if( !inFile.is_open() )
		{
			errorMsg.clear();
			errorMsg.a( "[SkinManager::loadSkins]: Could not open JSON file ", fullPath );
			log->log( errorMsg.c_str(), LogSeverity::Error );
			return;
		}

		inFile.seekg( 0, std::ios_base::end );
		const size_t fileSize = inFile.tellg();
		inFile.seekg( 0, std::ios_base::beg );

		if( fileSize > 0 )
		{
			std::vector<char> fileData;
			fileData.resize( fileSize );
			inFile.read( &fileData[0], fileSize );

			std::string filename = fullPath;
			std::string::size_type pos = filename.find_last_of( "/\\" );
			if( pos != std::string::npos )
				filename.erase( 0, pos + 1u );

			loadSkins( &fileData[0], filename.c_str() );
		}
		else
		{
			errorMsg.clear();
			errorMsg.a( "[SkinManager::loadSkins]: JSON file ", fullPath, " is empty!" );
			log->log( errorMsg.c_str(), LogSeverity::Error );
		}
	}
	//-------------------------------------------------------------------------
	void SkinManager::loadSkins( const char *jsonString, const char *filename )
	{
		rapidjson::Document d;
		d.Parse( jsonString );

		LogListener *log = m_crystalManager->getLogListener();
		char tmpBuffer[512];
		Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

		if( d.HasParseError() )
		{
			errorMsg.clear();
			errorMsg.a( "[SkinManager::loadSkins]: Invalid JSON string in file ", filename );
			log->log( errorMsg.c_str(), LogSeverity::Error );
			return;
		}

		rapidjson::Value::ConstMemberIterator itTmp;

		itTmp = d.FindMember( "skins" );
		if( itTmp != d.MemberEnd() && itTmp->value.IsObject() )
			loadSkins( itTmp->value, filename );

		itTmp = d.FindMember( "skin_packs" );
		if( itTmp != d.MemberEnd() && itTmp->value.IsObject() )
			loadSkinPacks( itTmp->value, filename );
	}
}

