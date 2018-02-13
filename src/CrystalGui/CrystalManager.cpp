
#include "CrystalGui/CrystalManager.h"

#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/Ogre/CrystalOgreRenderable.h"
#include "Vao/OgreVaoManager.h"

namespace Crystal
{
	static LogListener DefaultLogListener;

	CrystalManager::CrystalManager() :
		m_logListener( &DefaultLogListener ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false ),
		m_vaoManager( 0 ),
		m_objectMemoryManager( 0 ),
		m_sceneManager( 0 ),
		m_defaultIndexBuffer( 0 )
	{
	}
	//-------------------------------------------------------------------------
	CrystalManager::~CrystalManager()
	{
		setOgre( 0 );
	}
	//-------------------------------------------------------------------------
	void CrystalManager::setOgre( Ogre::VaoManager * crystalgui_nullable vaoManager )
	{
		if( m_defaultIndexBuffer )
		{
			m_vaoManager->destroyIndexBuffer( m_defaultIndexBuffer );
			m_defaultIndexBuffer = 0;
		}
		m_vaoManager = vaoManager;
		m_defaultIndexBuffer = Ogre::CrystalOgreRenderable::createIndexBuffer( vaoManager );
	}
	//-------------------------------------------------------------------------
	Window* CrystalManager::createWindow( Window * crystalgui_nullable parent )
	{
		Window *retVal = new Window( this );

		if( !parent )
			m_windows.push_back( retVal );
		else
		{
			retVal->m_parent = parent;
			parent->m_childWindows.push_back( retVal );
		}

		retVal->setWindowNavigationDirty();

		return retVal;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::destroyWindow( Window *window )
	{
		if( window->m_parent )
		{
			WindowVec::iterator itor = std::find( m_windows.begin(), m_windows.end(), window );

			if( itor == m_windows.end() )
			{
				m_logListener->log( "Window does not belong to this CrystalManager! "
									"Double free perhaps?", LogSeverity::Fatal );
			}
			else
				m_windows.erase( itor );
		}

		window->setWindowNavigationDirty();

		window->_destroy();
		delete window;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::_setAsParentlessWindow( Window *window )
	{
		m_windows.push_back( window );
	}
	//-------------------------------------------------------------------------
	void CrystalManager::setAsParentlessWindow( Window *window )
	{
		if( window->m_parent )
		{
			window->detachFromParent();
			m_windows.push_back( window );
		}
	}
	//-------------------------------------------------------------------------
	template <typename T>
	void CrystalManager::autosetNavigation( const std::vector<T> &container )
	{
		typename std::vector<T>::const_iterator itor = container.begin();
		typename std::vector<T>::const_iterator end  = container.end();

		while( itor != end )
		{
			Widget *widget = *itor;

			Widget *closestSiblings[Borders::NumBorders] = { 0, 0, 0, 0 };
			float closestSiblingDistances[Borders::NumBorders] =
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			};

			typename std::vector<T>::const_iterator it2 = itor + 1u;
			while( it2 != end )
			{
				Widget *widget2 = *it2;

				const Ogre::Vector2 cornerToCorner[4] =
				{
					widget2->m_position -
					widget->m_position,

					Ogre::Vector2( widget2->getRight(), widget2->m_position.y ) -
					Ogre::Vector2( widget->getRight(), widget->m_position.y ),

					Ogre::Vector2( widget2->m_position.x, widget2->getBottom() ) -
					Ogre::Vector2( widget->m_position.x, widget->getBottom() ),

					Ogre::Vector2( widget2->getRight(), widget2->getBottom() ) -
					Ogre::Vector2( widget->getRight(), widget->getBottom() ),
				};

				for( size_t i=0; i<4u; ++i )
				{
					Ogre::Vector2 dirTo = cornerToCorner[i];

					const float dirLength = dirTo.normalise();

					const float cosAngle( dirTo.dotProduct( Ogre::Vector2::UNIT_X ) );

					if( dirLength < closestSiblingDistances[Borders::Right] &&
						cosAngle <= Ogre::Degree( 45.0f ).valueRadians() )
					{
						closestSiblings[Borders::Right] = widget2;
						closestSiblingDistances[Borders::Right] = dirLength;
					}

					if( dirLength < closestSiblingDistances[Borders::Left] &&
						cosAngle <= Ogre::Degree( 135.0f ).valueRadians() )
					{
						closestSiblings[Borders::Left] = widget2;
						closestSiblingDistances[Borders::Left] = dirLength;
					}

					if( cosAngle >= Ogre::Degree( 45.0f ).valueRadians() &&
						cosAngle <= Ogre::Degree( 135.0f ).valueRadians() )
					{
						float crossProduct = dirTo.crossProduct( Ogre::Vector2::UNIT_X );

						if( crossProduct <= 0.0f )
						{
							if( dirLength < closestSiblingDistances[Borders::Top] )
							{
								closestSiblings[Borders::Top] = widget2;
								closestSiblingDistances[Borders::Top] = dirLength;
							}
						}
						else
						{
							if( dirLength < closestSiblingDistances[Borders::Bottom] )
							{
								closestSiblings[Borders::Bottom] = widget2;
								closestSiblingDistances[Borders::Bottom] = dirLength;
							}
						}
					}
				}

				++it2;
			}

			for( size_t i=0; i<4u; ++i )
			{
				if( widget->m_autoSetNextWidget[i] )
					widget->setNextWidget( closestSiblings[i], static_cast<Borders::Borders>( i ) );
			}

			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::autosetNavigation( Window *window )
	{
		if( window->m_widgetNavigationDirty )
		{
			autosetNavigation( window->m_children );
			window->m_widgetNavigationDirty = false;
		}

		if( window->m_windowNavigationDirty )
		{
			autosetNavigation( window->m_childWindows );
			window->m_windowNavigationDirty = false;
		}

		if( window->m_childrenNavigationDirty )
		{
			WindowVec::const_iterator itor = window->m_childWindows.begin();
			WindowVec::const_iterator end  = window->m_childWindows.end();

			while( itor != end )
				autosetNavigation( *itor++ );

			window->m_childrenNavigationDirty = false;
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::autosetNavigation()
	{
		if( m_windowNavigationDirty )
		{
			autosetNavigation( m_windows );
			m_windowNavigationDirty = false;
		}

		if( m_childrenNavigationDirty )
		{
			WindowVec::const_iterator itor = m_windows.begin();
			WindowVec::const_iterator end  = m_windows.end();

			while( itor != end )
			{
				autosetNavigation( *itor++ );
				m_childrenNavigationDirty = false;
			}
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::_setWindowNavigationDirty()
	{
		m_windowNavigationDirty = true;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::_notifyChildWindowIsDirty()
	{
		m_childrenNavigationDirty = true;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::update()
	{
		autosetNavigation();

		WindowVec::const_iterator itor = m_windows.begin();
		WindowVec::const_iterator end  = m_windows.end();

		while( itor != end )
		{
//			(*itor)->fillBuffersAndCommands( , Ogre::Vector2::ZERO, Ogre::Matrix3::IDENTITY );
			++itor;
		}
	}
}
