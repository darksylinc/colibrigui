
#include "CrystalGuiGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsTextureManager.h"
#include "OgreHlmsPbs.h"

#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/CrystalWindow.h"

using namespace Demo;

namespace Demo
{
	extern Crystal::CrystalManager *crystalManager;
	Crystal::CrystalManager *crystalManager = 0;
	Crystal::Window *mainWindow = 0;

    CrystalGuiGameState::CrystalGuiGameState( const Ogre::String &helpDescription ) :
		TutorialGameState( helpDescription )
	{
    }
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::createScene01(void)
    {
        mCameraController = new CameraController( mGraphicsSystem, false );

		//crystalManager = new Crystal::CrystalManager();
		crystalManager->setOgre( mGraphicsSystem->getRoot(),
								 mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager(),
								 mGraphicsSystem->getSceneManager() );
		crystalManager->loadSkins( (mGraphicsSystem->getResourcePath() +
								   "Materials/CrystalGui/Skins.crystal.json").c_str() );

		mainWindow = crystalManager->createWindow( 0 );

		Ogre::Hlms *hlms = mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );
		//mainWindow->setDatablock( hlms->getDefaultDatablock() );
		mainWindow->setSkin( "ButtonSkin" );
		//mainWindow->setDatablock( "ButtonSkin" );

		mGraphicsSystem->getSceneManager()->getRootSceneNode()->attachObject( mainWindow );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::update( float timeSinceLast )
	{
		crystalManager->update();
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
	{
		TutorialGameState::generateDebugText( timeSinceLast, outText );
	}
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

		TutorialGameState::keyReleased( arg );
    }
}
