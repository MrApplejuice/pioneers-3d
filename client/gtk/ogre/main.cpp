#include "main.h"

#include <iostream>
#include <memory>
#include <vector>

#include <gtk/gtk.h>

#include <Ogre.h>
#include <Bites/OgreBitesConfigDialog.h>

namespace pogre {
	typedef std::shared_ptr<Ogre::Root> OgreRootPtr;

	OgreRootPtr root;
	Ogre::RenderWindow* window;

	Ogre::SceneManager* mainScene;

	class CameraControls {
	private:
		OgreRootPtr _r;

		Ogre::Camera* camera;
		Ogre::Viewport* viewport;
	public:
		typedef std::shared_ptr<CameraControls> Ptr;

		CameraControls() {
			_r = root;

			camera = mainScene->createCamera("camera");
			viewport = window->addViewport(camera);
			viewport->setDimensions(0, 0, 1, 1);
			viewport->setBackgroundColour(Ogre::ColourValue::Red);

			camera->setAspectRatio((float) window->getWidth() / (float) window->getHeight());
		}

		virtual ~CameraControls() {
			mainScene->destroyCamera(camera);
		}
	};

	class MapRenderer {
	private:
		::Map* theMap;
	public:
		typedef std::shared_ptr<MapRenderer> Ptr;

		MapRenderer(::Map* _map) : theMap(_map) {

		}
	};

	CameraControls::Ptr cameraControls;
}

extern "C" {
	void ogreb_init() {
		pogre::root = pogre::OgreRootPtr(new Ogre::Root());

		if (!pogre::root->restoreConfig()) {
			pogre::root->showConfigDialog(OgreBites::getNativeConfigDialog());
		}
		pogre::window = pogre::root->initialise(true, "Pioneers 3D Game Window");
		pogre::mainScene = pogre::root->createSceneManager(
				Ogre::DefaultSceneManagerFactory::FACTORY_TYPE_NAME,
				"Main Scene");
		//pogre::root-
		//pogre::mainScene->

		pogre::cameraControls = pogre::CameraControls::Ptr(new pogre::CameraControls());
	}

	static gint64 animationTimerValue;
	static gboolean	animate(gpointer user_data) {
		gint64 now = g_get_real_time();
		const Ogre::Real seconds = (now - animationTimerValue) / 1000000.0;
		animationTimerValue = now;
		if (pogre::root) pogre::root->renderOneFrame(seconds);
		return G_SOURCE_CONTINUE;
	}

	VoidFunction pogre_setup_gtk_mainloop(VoidFunction old) {
		animationTimerValue = g_get_real_time();
		g_timeout_add(10, &animate, NULL);
		return old;
	}

	void ogreb_show_map(Map* map) {
	}

	void ogreb_cleanup() {
		pogre::cameraControls.reset();
		pogre::root.reset();
	}
}
