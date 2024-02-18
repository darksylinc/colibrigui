
#include "GraphicsSystem.h"

namespace Colibri
{
	class ColibriManager;
}
namespace Ogre
{
	class CompositorPassColibriGuiProvider;
}

namespace Demo
{
	/** We override the Demo classes from OgreNext in order to:
			1. Create our own HlmsColibri and register it.
			2. Create & setup ColibriManager.
			3. Register Colibri's compositor provider
			4. Use a compositor script that has the colibri pass.
	*/
	class ColibriGuiGraphicsSystem : public GraphicsSystem
	{
		Colibri::ColibriManager *m_colibriManager;

		Ogre::CompositorPassColibriGuiProvider *m_colibriCompoProvider;

		Ogre::CompositorWorkspace *setupCompositor() override;

		void registerHlms() override;

		void setupResources() override;

	public:
		ColibriGuiGraphicsSystem( GameState *gameState );

		Colibri::ColibriManager *getColibriManager() { return m_colibriManager; }

		Ogre::CompositorPassColibriGuiProvider *getColibriCompoProvider()
		{
			return m_colibriCompoProvider;
		}

		void deinitialize() override;
	};
}  // namespace Demo
