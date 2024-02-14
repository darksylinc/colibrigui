
#include "GraphicsSystem.h"
#include "ColibriGuiGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/Text/ColibriShaperManager.h"
#include "ColibriGui/Text/ColibriShaper.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiProvider.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "ColibriGui/Ogre/OgreHlmsColibri.h"
#include "OgreArchiveManager.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#include "OgreLogManager.h"
#include "hb.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <pwd.h>
    #include <errno.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    #include "shlobj.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    #include "OSX/macUtils.h"
        #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    #include "System/iOS/iOSUtils.h"
    #else
        #include "System/OSX/OSXUtils.h"
    #endif
#endif


#define TODO_fix_leak

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
	extern Colibri::ColibriManager *colibriManager;
	class ColibriLogListener final : public Colibri::LogListener
	{
		void log( const char *text, Colibri::LogSeverity::LogSeverity severity ) override
		{
			Ogre::LogManager::getSingleton().logMessage( text );
		}
	};
	class ColibriListener final : public Colibri::ColibriListener
	{
		void setClipboardText( const char *text ) override { SDL_SetClipboardText( text ); }

		bool getClipboardText( char *colibri_nonnull *const colibri_nullable outText )
			override
		{
			*outText = SDL_GetClipboardText();
			return *outText != 0;
		}

		void freeClipboardText( char *colibri_nullable text ) override { free( text ); }

		/** Overload this function to trigger sound effects (or whatever else you wish to do as
			feedback).

			@see Colibri::ColibriListener::flushEffectReaction
		@param effectReaction
			See Colibri::ColibriListener::flushEffectReaction documentation.
		@param repeatCount
			See Colibri::ColibriListener::flushEffectReaction documentation.
		*/
		void flushEffectReaction( uint16_t effectReaction, uint16_t repeatCount ) override
		{
			using namespace Colibri;
			switch( effectReaction )
			{
			case EffectReaction::NoReaction:
				break;
			case EffectReaction::Moved:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Moved\n" );
				break;
			case EffectReaction::PrimaryAction:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Click!\n" );
				break;
			case EffectReaction::NotPressable:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Stop clicking me!\n" );
				break;
			case EffectReaction::SliderMoved:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Sliding\n" );
				break;
			case EffectReaction::SpinnerChanged:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Spinning\n" );
				break;
			case EffectReaction::Toggled:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Toggled\n" );
				break;
			case EffectReaction::Checkboxed:
				for( size_t i = 0u; i < repeatCount; ++i )
					printf( "Checkboxed\n" );
				break;
			default:
				for( size_t i = 0u; i < repeatCount; ++i )
				{
					// Notice that effectReaction can be an arbitrary out-of-range value!
					//
					// You are in control of values that are not in EffectReaction.
					// This happens whenever you call setEffectReaction() (see
					// DemoWidgetListener::notifyWidgetAction).
					//
					// Colibri will not set an out-of-bounds value on its own.
					// We are here because our DemoWidgetListener specifically set the value to 1000u.
					if( effectReaction == 1000u )
						printf( "Special SFX for innerToggleOrder button!\n" );
				}
				break;
			}
		}
	};
	static ColibriLogListener g_colibriLogListener;
	static ColibriListener g_colibriListener;

    class ColibriGuiGraphicsSystem : public GraphicsSystem
	{
		virtual Ogre::CompositorWorkspace* setupCompositor()
		{
			Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
			return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
													"ColibriGuiWorkspace", true );
		}

		void registerHlms(void)
		{
			Ogre::ConfigFile cf;
			cf.load( mResourcePath + "resources2.cfg" );

	#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
			Ogre::String rootHlmsFolder = Ogre::macBundlePath() + '/' +
									  cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
	#else
			Ogre::String rootHlmsFolder = mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
	#endif

			if( rootHlmsFolder.empty() )
				rootHlmsFolder = "./";
			else if( *(rootHlmsFolder.end() - 1) != '/' )
				rootHlmsFolder += "/";

			//At this point rootHlmsFolder should be a valid path to the Hlms data folder

			Ogre::HlmsColibri *hlmsColibri = 0;
			Ogre::HlmsPbs *hlmsPbs = 0;

			//For retrieval of the paths to the different folders needed
			Ogre::String mainFolderPath;
			Ogre::StringVector libraryFoldersPaths;
			Ogre::StringVector::const_iterator libraryFolderPathIt;
			Ogre::StringVector::const_iterator libraryFolderPathEn;

			Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

			{
				//Create & Register HlmsColibri
				//Get the path to all the subdirectories used by HlmsColibri
				Ogre::HlmsColibri::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
				Ogre::Archive *archiveUnlit = archiveManager.load( rootHlmsFolder + mainFolderPath,
																   "FileSystem", true );
				Ogre::ArchiveVec archiveUnlitLibraryFolders;
				libraryFolderPathIt = libraryFoldersPaths.begin();
				libraryFolderPathEn = libraryFoldersPaths.end();
				while( libraryFolderPathIt != libraryFolderPathEn )
				{
					Ogre::Archive *archiveLibrary =
							archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
					archiveUnlitLibraryFolders.push_back( archiveLibrary );
					++libraryFolderPathIt;
				}

				//Create and register the unlit Hlms
				hlmsColibri = OGRE_NEW Ogre::HlmsColibri( archiveUnlit, &archiveUnlitLibraryFolders );
				Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsColibri );
			}

			{
				//Create & Register HlmsPbs
				//Do the same for HlmsPbs:
				Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
				Ogre::Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath,
																 "FileSystem", true );

				//Get the library archive(s)
				Ogre::ArchiveVec archivePbsLibraryFolders;
				libraryFolderPathIt = libraryFoldersPaths.begin();
				libraryFolderPathEn = libraryFoldersPaths.end();
				while( libraryFolderPathIt != libraryFolderPathEn )
				{
					Ogre::Archive *archiveLibrary =
							archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
					archivePbsLibraryFolders.push_back( archiveLibrary );
					++libraryFolderPathIt;
				}

				//Create and register
				hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
				Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
			}


			Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
			if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
			{
				//Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
				//and below to avoid saturating AMD's discard limit (8MB) or
				//saturate the PCIE bus in some low end machines.
				bool supportsNoOverwriteOnTextureBuffers;
				renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
												  &supportsNoOverwriteOnTextureBuffers );

				if( !supportsNoOverwriteOnTextureBuffers )
				{
					hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
					hlmsColibri->setTextureBufferDefaultSize( 512 * 1024 );
				}
			}

            #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                //Now setup is complete enough to allow permanent addition of the /Data/ path.
                //Ideally this would have been there from the beginning,
                //but this is difficult with reading in specific resources2.cfg files.
                mResourcePath += "/Data/";
            #endif
		}

        virtual void setupResources(void)
        {
			TODO_fix_leak;

			struct ShaperSettings
			{
				const char *locale;
				const char *fullpath;
				hb_script_t script;
				Colibri::HorizReadingDir::HorizReadingDir horizReadingDir;
				bool useKerning;
				bool allowsVerticalLayout;
				ShaperSettings( const char *_locale, const char *_fullpath, hb_script_t _script,
								bool _useKerning=false,
								Colibri::HorizReadingDir::HorizReadingDir _horizReadingDir=
						Colibri::HorizReadingDir::LTR,
								bool _allowsVerticalLayout=false ) :
					locale( _locale ),
					fullpath( _fullpath ),
					script( _script ),
					horizReadingDir( _horizReadingDir ),
					useKerning( _useKerning ),
					allowsVerticalLayout( _allowsVerticalLayout )
				{

				}
			};

            Ogre::String dataPath;
            #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                dataPath = mResourcePath + "/Data/";
            #else
                dataPath = "../Data/";
            #endif
			ShaperSettings shaperSettings[3] =
			{
				ShaperSettings( "en", "Fonts/DejaVuSerif.ttf", HB_SCRIPT_LATIN, true ),
				ShaperSettings( "ar", "Fonts/amiri-0.104/amiri-regular.ttf", HB_SCRIPT_ARABIC, false,
				Colibri::HorizReadingDir::RTL ),
				ShaperSettings( "ch", "Fonts/fireflysung-1.3.0/fireflysung.ttf", HB_SCRIPT_HAN, false,
				Colibri::HorizReadingDir::LTR, true )
			};

			colibriManager = new Colibri::ColibriManager( &g_colibriLogListener,
														  &g_colibriListener );
			Colibri::ShaperManager *shaperManager = colibriManager->getShaperManager();

			for( size_t i=0; i<sizeof( shaperSettings ) / sizeof( shaperSettings[0] ); ++i )
			{
				Colibri::Shaper *shaper;
				const Ogre::String fullPath = dataPath + shaperSettings[i].fullpath;
				shaper = shaperManager->addShaper( shaperSettings[i].script, fullPath.c_str(),
												   shaperSettings[i].locale );
				if( shaperSettings[i].useKerning )
					shaper->addFeatures( Colibri::Shaper::KerningOn );
			}

			size_t defaultFont = 0; //"en"
			shaperManager->setDefaultShaper( defaultFont + 1u,
											 shaperSettings[defaultFont].horizReadingDir,
											 shaperSettings[defaultFont].allowsVerticalLayout );

			shaperManager->addBmpFont( ( dataPath + "Fonts/ExampleBmpFont.fnt" ).c_str() );
			shaperManager->setDefaultBmpFontForRaster( 0u );

			if( defaultFont == 1 )
				colibriManager->setSwapRTLControls( true );

			Ogre::CompositorPassColibriGuiProvider *compoProvider =
					OGRE_NEW Ogre::CompositorPassColibriGuiProvider( colibriManager );
			Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
			compositorManager->setCompositorPassProvider( compoProvider );

            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load(mResourcePath + "resources2.cfg");

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = "./";
            else if( *(dataFolder.end() - 1) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, "FileSystem", "General" );
        }

    public:
        ColibriGuiGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
            #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                //mResourcePath = Ogre::macBundlePath() + "/Contents/Resources/Data/";
                //This can't be /Data/ for some of the early setup.
                //This is changed after that setup.
                mResourcePath = Ogre::macBundlePath() + "/Contents/Resources/";
            #else
                mResourcePath = "../Data/";
            #endif
			mAlwaysAskForConfig = false;

            //It's recommended that you set this path to:
            //	%APPDATA%/ColibriGui/ on Windows
            //	~/.config/ColibriGui/ on Linux
            //	macCachePath() + "/ColibriGui/" (NSCachesDirectory) on Apple -> Important because
            //	on iOS your app could be rejected from App Store when they see iCloud
            //	trying to backup your Ogre.log & ogre.cfg auto-generated without user
            //	intervention. Also convenient because these settings will be deleted
            //	if the user removes cached data from the app, so the settings will be
            //	reset.
            //  Obviously you can replace "ColibriGui" by your app's name.
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            mWriteAccessFolder =  + "/";
            TCHAR path[MAX_PATH];
            if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL,
                                            SHGFP_TYPE_CURRENT, path ) != S_OK ) )
            {
                //Need to convert to OEM codepage so that fstream can
                //use it properly on international systems.
        #if defined(_UNICODE) || defined(UNICODE)
                int size_needed = WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen(path),
                                                       NULL, 0, NULL, NULL );
                mWriteAccessFolder = std::string( size_needed, 0 );
                WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen(path),
                                     &mWriteAccessFolder[0], size_needed, NULL, NULL );
        #else
                TCHAR oemPath[MAX_PATH];
                CharToOem( path, oemPath );
                mWriteAccessFolder = std::string( oemPath );
        #endif
                mWriteAccessFolder += "/ColibriGui/";

                //Attempt to create directory where config files go
                if( !CreateDirectoryA( mWriteAccessFolder.c_str(), NULL ) &&
                    GetLastError() != ERROR_ALREADY_EXISTS )
                {
                    //Couldn't create directory (no write access?),
                    //fall back to current working dir
                    mWriteAccessFolder = "";
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            const char *homeDir = getenv("HOME");
            if( homeDir == 0 )
                homeDir = getpwuid( getuid() )->pw_dir;
            mWriteAccessFolder = homeDir;
            mWriteAccessFolder += "/.config";
            int result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
            int errorReason = errno;

            //Create "~/.config"
            if( result && errorReason != EEXIST )
            {
                printf( "Error. Failing to create path '%s'. Do you have access rights?",
                        mWriteAccessFolder.c_str() );
                mWriteAccessFolder = "";
            }
            else
            {
                //Create "~/.config/ColibriGui"
                mWriteAccessFolder += "/ColibriGui/";
                result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
                errorReason = errno;

                if( result && errorReason != EEXIST )
                {
                    printf( "Error. Failing to create path '%s'. Do you have access rights?",
                            mWriteAccessFolder.c_str() );
                    mWriteAccessFolder = "";
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            mWriteAccessFolder = Ogre::macCachePath() + "/ColibriGui/";
            //Create "pathToCache/ColibriGui"
            //mWriteAccessFolder += "/ColibriGui/";
            int result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
            int errorReason = errno;

            if( result && errorReason != EEXIST )
            {
                printf( "Error. Failing to create path '%s'. Do you have access rights?",
                        mWriteAccessFolder.c_str() );
                mWriteAccessFolder = "";
            }
#endif
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        ColibriGuiGameState *gfxGameState = new ColibriGuiGameState(
        "Empty Project Example" );

        GraphicsSystem *graphicsSystem = new ColibriGuiGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                          GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState,
                                          LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "Empty Project Sample";
    }
}
