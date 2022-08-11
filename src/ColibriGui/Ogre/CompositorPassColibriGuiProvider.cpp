
#include "ColibriGui/Ogre/CompositorPassColibriGuiProvider.h"

#include "ColibriGui/Ogre/CompositorPassColibriGui.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiDef.h"

#include "OgreLogManager.h"
#include "OgreScriptTranslator.h"
#include "OgreStringConverter.h"

namespace Ogre
{
	CompositorPassColibriGuiProvider::CompositorPassColibriGuiProvider(
		Colibri::ColibriManager *colibriManager ) :
		m_colibriManager( colibriManager )
	{
	}
	//-------------------------------------------------------------------------
	CompositorPassDef *CompositorPassColibriGuiProvider::addPassDef(
		CompositorPassType passType, IdString customId, CompositorTargetDef *parentTargetDef,
		CompositorNodeDef *parentNodeDef )
	{
		if( customId == "colibri_gui" )
			return OGRE_NEW CompositorPassColibriGuiDef( parentTargetDef );

		return 0;
	}
	//-------------------------------------------------------------------------
	CompositorPass *CompositorPassColibriGuiProvider::addPass( const CompositorPassDef *definition,
															   Camera *defaultCamera,
															   CompositorNode *parentNode,
															   const RenderTargetViewDef *rtvDef,
															   SceneManager *sceneManager )
	{
		COLIBRI_ASSERT( dynamic_cast<const CompositorPassColibriGuiDef *>( definition ) );
		const CompositorPassColibriGuiDef *colibriGuiDef =
			static_cast<const CompositorPassColibriGuiDef *>( definition );
		return OGRE_NEW CompositorPassColibriGui( colibriGuiDef, defaultCamera, sceneManager, rtvDef,
												  parentNode, m_colibriManager );
	}
	//-------------------------------------------------------------------------
	static bool ScriptTranslatorGetBoolean( const AbstractNodePtr &node, bool *result )
	{
		if( node->type != ANT_ATOM )
			return false;
		const AtomAbstractNode *atom = (const AtomAbstractNode *)node.get();
		if( atom->id == 1 || atom->id == 2 )
		{
			*result = atom->id == 1 ? true : false;
			return true;
		}
		return false;
	}
	//-------------------------------------------------------------------------
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 3, 0, 0 )
	void CompositorPassColibriGuiProvider::translateCustomPass( ScriptCompiler *compiler,
																const AbstractNodePtr &node,
																IdString customId,
																CompositorPassDef *customPassDef )
	{
		if( customId != "colibri_gui" )
			return;  // Custom pass not created by us

		CompositorPassColibriGuiDef *colibriGuiDef =
			static_cast<CompositorPassColibriGuiDef *>( customPassDef );

		ObjectAbstractNode *obj = reinterpret_cast<ObjectAbstractNode *>( node.get() );

		obj->context = Any( static_cast<CompositorPassDef *>( colibriGuiDef ) );

		AbstractNodeList::const_iterator itor = obj->children.begin();
		AbstractNodeList::const_iterator endt = obj->children.end();

		while( itor != endt )
		{
			if( ( *itor )->type == ANT_OBJECT )
			{
				ObjectAbstractNode *childObj = reinterpret_cast<ObjectAbstractNode *>( itor->get() );

				if( childObj->id == ID_LOAD )
				{
					CompositorLoadActionTranslator compositorLoadActionTranslator;
					compositorLoadActionTranslator.translate( compiler, *itor );
				}
				else if( childObj->id == ID_STORE )
				{
					CompositorStoreActionTranslator compositorStoreActionTranslator;
					compositorStoreActionTranslator.translate( compiler, *itor );
				}
			}
			else if( ( *itor )->type == ANT_PROPERTY )
			{
				const PropertyAbstractNode *prop =
					reinterpret_cast<const PropertyAbstractNode *>( itor->get() );
				if( prop->id == ID_SKIP_LOAD_STORE_SEMANTICS )
				{
					if( prop->values.size() != 1u ||
						!ScriptTranslatorGetBoolean( prop->values.front(),
													 &colibriGuiDef->mSkipLoadStoreSemantics ) )
					{
						Ogre::LogManager::getSingleton().logMessage(
							"Error in colibri_gui skip_load_store_semantics at " + prop->file +
							" line " + StringConverter::toString( prop->line ) );
					}
				}
				else if( prop->name == "aspect_ratio_mode" )
				{
					bool bValid = false;
					if( prop->values.size() == 1u )
					{
						const AbstractNodePtr &valueNode = prop->values.back();
						const Ogre::String value = valueNode->getValue();
						if( value == "none" )
						{
							colibriGuiDef->mAspectRatioMode = CompositorPassColibriGuiDef::ArNone;
							bValid = true;
						}
						else if( value == "keep_width" )
						{
							colibriGuiDef->mAspectRatioMode = CompositorPassColibriGuiDef::ArKeepWidth;
							bValid = true;
						}
						else if( value == "keep_height" )
						{
							colibriGuiDef->mAspectRatioMode = CompositorPassColibriGuiDef::ArKeepHeight;
							bValid = true;
						}
					}

					if( !bValid )
					{
						compiler->addError( ScriptCompiler::CE_STRINGEXPECTED, obj->file, obj->line,
											"aspect_ratio_mode accepts <none|keep_width|keep_height>" );
					}
				}
			}
			++itor;
		}
#endif
	}
}  // namespace Ogre
