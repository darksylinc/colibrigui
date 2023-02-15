
#include "ColibriGui/ColibriSkinManager.h"
#include "ColibriGui/ColibriManager.h"

#include "ColibriGui/ColibriProgressbar.h"

#include "OgreLwString.h"

#if defined( __GNUC__ ) && !defined( __clang__ )
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#if defined( __clang__ )
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#	pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#if defined( __clang__ )
#	pragma clang diagnostic pop
#endif
#if defined( __GNUC__ ) && !defined( __clang__ )
#	pragma GCC diagnostic pop
#endif

#include "sds/sds_fstream.h"
#include "sds/sds_fstreamApk.h"

namespace Colibri
{
	SkinManager::SkinManager( ColibriManager *colibriManager ) :
		m_colibriManager( colibriManager )
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
	void SkinManager::buildGridFromEnclosingUv( Ogre::Vector4 uvTopLeftWidthHeight,
												const Ogre::Vector2 &texResolution,
												Ogre::Vector2 borderSizeTL,
												Ogre::Vector2 borderSizeBR,
												StateInformation &stateInfo,
												const char *skinName, const char *filename )
	{
		if( borderSizeTL.x + borderSizeBR.x > uvTopLeftWidthHeight.z ||
			borderSizeTL.y + borderSizeBR.y > uvTopLeftWidthHeight.w )
		{
			LogListener *log = m_colibriManager->getLogListener();
			char tmpBuffer[512];
			Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

			errorMsg.clear();
			errorMsg.a( "[SkinManager::buildGridFromEnclosingUv] Enclosing's border parameter "
						"must be less than half of the width and less than the height. "
						"Skin: ", skinName, " File: ", filename );
			log->log( errorMsg.c_str(), LogSeverity::Warning );
		}

		stateInfo.centerAspectRatio = (uvTopLeftWidthHeight.z - borderSizeTL.x - borderSizeBR.x) /
									  (uvTopLeftWidthHeight.w - borderSizeTL.y - borderSizeBR.y);

		const Ogre::Vector2 topLeft( uvTopLeftWidthHeight.x / texResolution.x,
		                             uvTopLeftWidthHeight.y / texResolution.y );
		const Ogre::Vector2 widthHeight( uvTopLeftWidthHeight.z / texResolution.x,
		                                 uvTopLeftWidthHeight.w / texResolution.y );
		borderSizeTL /= texResolution;
		borderSizeBR /= texResolution;

		GridLocations::GridLocations idx;

		idx = GridLocations::TopLeft;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + borderSizeTL.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + borderSizeTL.y;

		idx = GridLocations::Top;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x + borderSizeTL.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + widthHeight.x - borderSizeBR.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + borderSizeTL.y;

		idx = GridLocations::TopRight;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x + widthHeight.x - borderSizeBR.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + widthHeight.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + borderSizeTL.y;

		idx = GridLocations::CenterLeft;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y + borderSizeTL.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + borderSizeTL.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + widthHeight.y - borderSizeBR.y;

		idx = GridLocations::Center;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x + borderSizeTL.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y + borderSizeTL.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + widthHeight.x - borderSizeBR.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + widthHeight.y - borderSizeBR.y;

		idx = GridLocations::CenterRight;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x + widthHeight.x - borderSizeBR.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y + borderSizeTL.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + widthHeight.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + widthHeight.y - borderSizeBR.y;

		idx = GridLocations::BottomLeft;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y + widthHeight.y - borderSizeBR.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + borderSizeTL.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + widthHeight.y;

		idx = GridLocations::Bottom;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x + borderSizeTL.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y + widthHeight.y - borderSizeBR.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + widthHeight.x - borderSizeBR.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + widthHeight.y;

		idx = GridLocations::BottomRight;
		stateInfo.uvTopLeftBottomRight[idx].x = topLeft.x + widthHeight.x - borderSizeBR.x;
		stateInfo.uvTopLeftBottomRight[idx].y = topLeft.y + widthHeight.y - borderSizeBR.y;
		stateInfo.uvTopLeftBottomRight[idx].z = topLeft.x + widthHeight.x;
		stateInfo.uvTopLeftBottomRight[idx].w = topLeft.y + widthHeight.y;
	}
	//-------------------------------------------------------------------------
	void SkinManager::loadSkins( const rapidjson::Value &skinsValue, const char *filename )
	{
		LogListener *log = m_colibriManager->getLogListener();
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
				skinInfo.stateInfo.centerAspectRatio = 1.0f;
				skinInfo.stateInfo.defaultColour = Ogre::ColourValue::White;

				const rapidjson::Value &skinValue = itor->value;

				rapidjson::Value::ConstMemberIterator itTmp;

				itTmp = skinValue.FindMember( "copy_from" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsString() )
				{
					const char *baseSkinName = itTmp->value.GetString();
					SkinInfoMap::const_iterator itSkin = m_skins.find( baseSkinName );
					if( itSkin != m_skins.end() )
					{
						skinInfo = itSkin->second;

						//itBaseSkin should be guaranteed to find a match if we're here
						rapidjson::Value::ConstMemberIterator itBaseSkin =
								skinsValue.FindMember( baseSkinName );

						COLIBRI_ASSERT_LOW( itBaseSkin != skinsValue.MemberEnd() );

						itTmp = itBaseSkin->value.FindMember( "tex_resolution" );
						if( itTmp != itBaseSkin->value.MemberEnd() &&
							itTmp->value.IsArray() &&
							itTmp->value.Size() == 2u &&
							itTmp->value[0].IsUint() && itTmp->value[1].IsUint() )
						{
							texResolution.x = Ogre::Real( itTmp->value[0].GetUint() );
							texResolution.y = Ogre::Real( itTmp->value[1].GetUint() );
						}
					}
					else
					{
						errorMsg.clear();
						errorMsg.a( "[SkinManager::loadSkins]: Skin ", baseSkinName,
									" needed by skin ", itor->name.GetString(),
									" via copy_from not found! Note skins to be copied "
									"from must be defined first. in ", filename );
						log->log( errorMsg.c_str(), LogSeverity::Error );
					}
				}

				//Now that we've copied the base parameters, set the actual name
				skinInfo.name = itor->name.GetString();

				itTmp = skinValue.FindMember( "tex_resolution" );
				if( itTmp != skinValue.MemberEnd() &&
					itTmp->value.IsArray() &&
					itTmp->value.Size() == 2u &&
					itTmp->value[0].IsUint() && itTmp->value[1].IsUint() )
				{
					texResolution.x = Ogre::Real( itTmp->value[0].GetUint() );
					texResolution.y = Ogre::Real( itTmp->value[1].GetUint() );
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

						skinInfo.stateInfo.centerAspectRatio = uvTopLeftWidthHeight.z /
															   uvTopLeftWidthHeight.w;

						if( std::isnan( skinInfo.stateInfo.centerAspectRatio ) )
						{
							errorMsg.clear();
							errorMsg.a( "[SkinManager::loadSkins]: Invalid aspect ratio for ",
										itor->name.GetString(), " in ", filename );
							log->log( errorMsg.c_str(), LogSeverity::Warning );
						}

						COLIBRI_ASSERT_LOW( !std::isnan( skinInfo.stateInfo.centerAspectRatio ) );
					}

					itTmp = gridValue.FindMember( "enclosing" );
					if( itTmp != skinValue.MemberEnd() &&
						itTmp->value.IsArray() &&
						((itTmp->value.Size() == 2u &&
						  itTmp->value[0].IsArray() &&
						  itTmp->value[1].IsArray()) ||
						 (itTmp->value.Size() == 3u &&
						  itTmp->value[0].IsArray() &&
						  itTmp->value[1].IsArray() &&
						  itTmp->value[2].IsArray())) )
					{
						const Ogre::Vector4 uvTopLeftWidthHeight = getVector4Array( itTmp->value[0] );
						const Ogre::Vector2 borderUvSizeTL = getVector2Array( itTmp->value[1] );

						Ogre::Vector2 borderUvSizeBR;

						if( itTmp->value.Size() == 2u )
							borderUvSizeBR = borderUvSizeTL;
						else
							borderUvSizeBR = getVector2Array( itTmp->value[2] );

						buildGridFromEnclosingUv( uvTopLeftWidthHeight, texResolution, borderUvSizeTL,
												  borderUvSizeBR, skinInfo.stateInfo,
												  skinInfo.name.c_str(), filename );
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

							if( i == GridLocations::Center )
							{
								skinInfo.stateInfo.centerAspectRatio = uvTopLeftWidthHeight.z /
																	   uvTopLeftWidthHeight.w;

								if( std::isnan( skinInfo.stateInfo.centerAspectRatio ) )
								{
									errorMsg.clear();
									errorMsg.a( "[SkinManager::loadSkins]: Invalid aspect ratio for ",
												itor->name.GetString(), " in ", filename );
									log->log( errorMsg.c_str(), LogSeverity::Warning );
								}

								COLIBRI_ASSERT_LOW(
									!std::isnan( skinInfo.stateInfo.centerAspectRatio ) );
							}
						}
					}

					for( size_t i=0; i<GridLocations::NumGridLocations; ++i )
					{
						skinInfo.stateInfo.uvTopLeftBottomRight[i].x += 0.5f / texResolution.x;
						skinInfo.stateInfo.uvTopLeftBottomRight[i].y += 0.5f / texResolution.y;
						skinInfo.stateInfo.uvTopLeftBottomRight[i].z -= 0.5f / texResolution.x;
						skinInfo.stateInfo.uvTopLeftBottomRight[i].w -= 0.5f / texResolution.y;
					}
				}

				itTmp = skinValue.FindMember( "borders" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsObject() )
				{
					const rapidjson::Value &borderValue = itTmp->value;

					itTmp = borderValue.FindMember( "all" );
					if( itTmp != borderValue.MemberEnd() )
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
						itTmp = borderValue.FindMember( borders[i] );
						if( itTmp != borderValue.MemberEnd() )
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

				itTmp = skinValue.FindMember( "colour" );
				if( itTmp != skinValue.MemberEnd() &&  //
					itTmp->value.IsArray() &&
					( itTmp->value.Size() == 3u || itTmp->value.Size() == 4u ) &&
					itTmp->value[0].IsDouble() &&  //
					itTmp->value[1].IsDouble() &&  //
					itTmp->value[2].IsDouble() &&
					( itTmp->value.Size() == 3u || itTmp->value[3].IsDouble() ) )
				{
					const rapidjson::SizeType numElements = itTmp->value.Size();
					for( rapidjson::SizeType i = 0u; i < numElements; ++i )
					{
						skinInfo.stateInfo.defaultColour[i] =
							static_cast<float>( itTmp->value[i].GetDouble() );
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
	void SkinManager::loadSkinPacks( const rapidjson::Value &packsValue,
									 const rapidjson::Value &skinsValue, const char *filename )
	{
		rapidjson::Value::ConstMemberIterator itor = packsValue.MemberBegin();
		rapidjson::Value::ConstMemberIterator end  = packsValue.MemberEnd();

		while( itor != end )
		{
			if( itor->name.IsString() && itor->value.IsObject() )
			{
				SkinPack skinPack;

				skinPack.progressBarType = 0u;
				skinPack.progressBarIsAnimated = false;
				skinPack.progressBarAnimSpeed = 0.0f;
				skinPack.progressBarAnimLength = 1.0f;

				skinPack.windowScrollArrowSize[0] = 64.0f;
				skinPack.windowScrollArrowSize[1] = 64.0f;
				skinPack.windowScrollArrowProportion = 0.5f;
				skinPack.windowScrollArrowOrientation = 0.0f;

				skinPack.sliderLineSize = 5.0f;
				skinPack.sliderHandleProportion[0] = 0.8f;
				skinPack.sliderHandleProportion[1] = 0.8f;
				skinPack.sliderPositionTopLeftProportion = 0.5f;
				skinPack.sliderAlwaysInside = false;
				skinPack.sliderExcludeBorders = false;
				skinPack.sliderHandleBorderIsHalo = false;

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

				itTmp = skinValue.FindMember( "progress_bar_type" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsString() )
				{
					Ogre::LwConstString progressBarType(
						Ogre::LwString::FromUnsafeCStr( itTmp->value.GetString() ) );

					if( progressBarType == "behind_glass" )
						skinPack.progressBarType = Progressbar::BehindGlass;
				}

				itTmp = skinValue.FindMember( "progress_bar_animated" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsBool() )
					skinPack.progressBarIsAnimated = itTmp->value.GetBool();

				itTmp = skinValue.FindMember( "progress_anim_speed" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsDouble() )
					skinPack.progressBarAnimSpeed = static_cast<float>( itTmp->value.GetDouble() );

				itTmp = skinValue.FindMember( "progress_anim_length" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsDouble() )
					skinPack.progressBarAnimLength = static_cast<float>( itTmp->value.GetDouble() );

				itTmp = skinValue.FindMember( "window_scroll_arrow_size" );
				if( itTmp != skinValue.MemberEnd() )
				{
					if( itTmp->value.IsDouble() )
					{
						skinPack.windowScrollArrowSize[0] =
							static_cast<float>( itTmp->value.GetDouble() );
						skinPack.windowScrollArrowSize[1] = skinPack.windowScrollArrowSize[0];
					}
					else if( itTmp->value.IsArray() && itTmp->value.Size() == 2u &&
							 itTmp->value[0].IsDouble() && itTmp->value[1].IsDouble() )
					{
						skinPack.windowScrollArrowSize[0] =
							static_cast<float>( itTmp->value[0].GetDouble() );
						skinPack.windowScrollArrowSize[1] =
							static_cast<float>( itTmp->value[1].GetDouble() );
					}
				}

				itTmp = skinValue.FindMember( "window_scroll_arrow_orientation" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsDouble() )
				{
					skinPack.windowScrollArrowOrientation =
						Ogre::Degree( static_cast<Ogre::Real>( itTmp->value.GetDouble() ) );
				}

				itTmp = skinValue.FindMember( "window_scroll_arrow_proportion" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsDouble() )
				{
					skinPack.windowScrollArrowProportion =
						static_cast<float>( itTmp->value.GetDouble() );
				}

				itTmp = skinValue.FindMember( "slider_line_size" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsDouble() )
					skinPack.sliderLineSize = static_cast<float>( itTmp->value.GetDouble() );

				itTmp = skinValue.FindMember( "slider_handle_proportion" );
				if( itTmp != skinValue.MemberEnd() )
				{
					if( itTmp->value.IsDouble() )
					{
						// Same value for both XY
						skinPack.sliderHandleProportion[0] =
							static_cast<float>( itTmp->value.GetDouble() );
						skinPack.sliderHandleProportion[1] =
							static_cast<float>( itTmp->value.GetDouble() );
					}
					else if( itTmp->value.IsArray() && itTmp->value.Size() == 2u &&
							 itTmp->value[0].IsDouble() && itTmp->value[1].IsDouble() )
					{
						// Separate values
						skinPack.sliderHandleProportion[0] =
							static_cast<float>( itTmp->value[0].GetDouble() );
						skinPack.sliderHandleProportion[1] =
							static_cast<float>( itTmp->value[1].GetDouble() );
					}
				}

				itTmp = skinValue.FindMember( "slider_handle_position_top_left_proportion" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsDouble() )
				{
					skinPack.sliderPositionTopLeftProportion =
						static_cast<float>( itTmp->value.GetDouble() );
				}

				itTmp = skinValue.FindMember( "slider_always_inside" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsBool() )
					skinPack.sliderAlwaysInside = itTmp->value.GetBool();

				itTmp = skinValue.FindMember( "slider_exclude_borders" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsBool() )
					skinPack.sliderExcludeBorders = itTmp->value.GetBool();

				itTmp = skinValue.FindMember( "slider_handle_border_is_halo" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsBool() )
					skinPack.sliderHandleBorderIsHalo = itTmp->value.GetBool();

				const char *states[States::NumStates] =
				{
					"disabled",
					"idle",
					"highlighted_cursor",
					"highlighted_button",
					"highlighted_button_and_cursor",
					"pressed"
				};

				itTmp = skinValue.FindMember( "auto" );
				if( itTmp != skinValue.MemberEnd() && itTmp->value.IsString() )
				{
					char tmpBuffer[256];
					Ogre::LwString skinName( Ogre::LwString::FromEmptyPointer( tmpBuffer,
																			   sizeof( tmpBuffer ) ) );

					skinName = itTmp->value.GetString();
					const size_t prefixSize = skinName.size();

					for( size_t i=0u; i<States::NumStates; ++i )
					{
						skinName.resize( prefixSize );
						skinName.a( "_", states[i] );
						if( skinsValue.FindMember( skinName.c_str() ) != skinsValue.MemberEnd() )
							skinPack.skinInfo[i] = skinName.c_str();
						else
						{
							//Assume _idle exists
							skinName.resize( prefixSize );
							skinName.a( "_", states[States::Idle] );
							skinPack.skinInfo[i] = skinName.c_str();
						}
					}
				}

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
	void SkinManager::loadDefaultSkinPacks( const rapidjson::Value &packsValue, const char *filename )
	{
		const char *c_strSkinWidgetTypes[] = {
			"Window",
			"Button",
			"Spinner",
			"SpinnerBtnDecrement",
			"SpinnerBtnIncrement",
			"Checkbox",
			"CheckboxTickmarkUnchecked",
			"CheckboxTickmarkChecked",
			"CheckboxTickmarkThirdState",
			"Editbox",
			"ProgressbarLayer0",
			"ProgressbarLayer1",
			"SliderLine",
			"SliderHandle",
			"ToggleButtonUnchecked",
			"ToggleButtonChecked",
			"WindowArrowScrollTop",
			"WindowArrowScrollLeft",
			"WindowArrowScrollRight",
			"WindowArrowScrollBottom",
		};

		std::string defaultSkins[SkinWidgetTypes::NumSkinWidgetTypes];

		COLIBRI_STATIC_ASSERT( (sizeof( c_strSkinWidgetTypes ) / sizeof( c_strSkinWidgetTypes[0] )) ==
							   SkinWidgetTypes::NumSkinWidgetTypes );

		for( size_t i=0; i<SkinWidgetTypes::NumSkinWidgetTypes; ++i )
		{
			rapidjson::Value::ConstMemberIterator itTmp;

			itTmp = packsValue.FindMember( c_strSkinWidgetTypes[i] );
			if( itTmp != packsValue.MemberEnd() && itTmp->value.IsString() )
				defaultSkins[i] = itTmp->value.GetString();
		}

		m_colibriManager->setDefaultSkins( defaultSkins );
	}
	//-------------------------------------------------------------------------
	SkinPack const *colibri_nullable
	SkinManager::findSkinPack( Ogre::IdString name, LogSeverity::LogSeverity logSeverity ) const
	{
		SkinPack const *retVal = 0;

		SkinPackMap::const_iterator itor = m_skinPacks.find( name );
		if( itor == m_skinPacks.end() )
		{
			char tmpBuffer[512];
			Ogre::LwString errorMsg(
				Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
			errorMsg.a( "[SkinManager::findSkinPack] Skin pack '", name.getFriendlyText().c_str(),
						"' not found!" );
			m_colibriManager->getLogListener()->log( errorMsg.c_str(), logSeverity );
		}
		else
		{
			retVal = &itor->second;
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	SkinInfo const *colibri_nullable SkinManager::findSkin(
		const SkinPack &pack, States::States state, LogSeverity::LogSeverity logSeverity ) const
	{
		SkinInfo const *retVal = 0;

		SkinInfoMap::const_iterator itor;
		itor = m_skins.find( pack.skinInfo[state] );
		if( itor == m_skins.end() )
		{
			char tmpBuffer[512];
			Ogre::LwString errorMsg(
				Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
			errorMsg.a( "[SkinManager::findSkin] Skin '", pack.skinInfo[state].getFriendlyText().c_str(),
						"' not found from pack '", pack.name.c_str(), "'" );
			m_colibriManager->getLogListener()->log( errorMsg.c_str(), logSeverity );
		}
		else
		{
			retVal = &itor->second;
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	void SkinManager::loadSkins( const char *fullPath )
	{
		LogListener *log = m_colibriManager->getLogListener();
		char tmpBuffer[512];
		Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

		sds::PackageFstream inFile( fullPath, sds::fstream::InputEnd );

		if( !inFile.is_open() )
		{
			errorMsg.clear();
			errorMsg.a( "[SkinManager::loadSkins]: Could not open JSON file ", fullPath );
			log->log( errorMsg.c_str(), LogSeverity::Error );
			return;
		}

		const size_t fileSize = inFile.getFileSize( false );
		inFile.seek( 0, sds::fstream::beg );

		if( fileSize > 0 )
		{
			std::vector<char> fileData;
			fileData.resize( fileSize + 1u );
			inFile.read( &fileData[0], fileSize );
			fileData[fileSize] = '\0'; // Add null terminator

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

		LogListener *log = m_colibriManager->getLogListener();
		char tmpBuffer[512];
		Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

		if( d.HasParseError() )
		{
			errorMsg.clear();
			errorMsg.a( "[SkinManager::loadSkins]: Invalid JSON string in file ", filename );
			log->log( errorMsg.c_str(), LogSeverity::Error );
			log->log( rapidjson::GetParseError_En( d.GetParseError() ), LogSeverity::Error );
			return;
		}

		rapidjson::Value::ConstMemberIterator itTmp;

		itTmp = d.FindMember( "skins" );
		{
			if( itTmp != d.MemberEnd() && itTmp->value.IsObject() )
				loadSkins( itTmp->value, filename );

			rapidjson::Value::ConstMemberIterator itTmp2 = d.FindMember( "skin_packs" );
			if( itTmp2 != d.MemberEnd() && itTmp2->value.IsObject() )
				loadSkinPacks( itTmp2->value, itTmp->value, filename );
		}

		itTmp = d.FindMember( "default_skin_packs" );
		if( itTmp != d.MemberEnd() && itTmp->value.IsObject() )
			loadDefaultSkinPacks( itTmp->value, filename );
	}
}
