#include "main.h"

#include <iostream>
#include <memory>
#include <vector>

#include <gtk/gtk.h>

#include <Ogre.h>
#include <Bites/OgreBitesConfigDialog.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>

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

		Ogre::SceneNode* location;
	public:
		typedef std::shared_ptr<CameraControls> Ptr;

		CameraControls() {
			_r = root;

			camera = mainScene->createCamera("camera");
			viewport = window->addViewport(camera);
			viewport->setDimensions(0, 0, 1, 1);
			viewport->setBackgroundColour(Ogre::ColourValue::Red);

			camera->setAutoAspectRatio(true);
			camera->setNearClipDistance(.5);
			camera->setFarClipDistance(10);

			location = mainScene->getRootSceneNode()->createChildSceneNode("camera location");
			location->attachObject(camera);
			location->setPosition(0, -.5, .5);

			location->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TransformSpace::TS_WORLD);
		}

		virtual ~CameraControls() {
			mainScene->getRootSceneNode()->removeAndDestroyChild(location);
			mainScene->destroyCamera(camera);
		}
	};

	class MapRenderer {
	private:
		::Map* theMap;
	public:
		typedef std::shared_ptr<MapRenderer> Ptr;

		MapRenderer(::Map* _map) : theMap(_map) {
			std::cout << "init shader man " << Ogre::RTShader::ShaderGenerator::initialize() << std::endl;
			auto rtshare = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
			rtshare->addSceneManager(mainScene);

			auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();

			resgrpman->createResourceGroup("map", false);
			resgrpman->addResourceLocation("ogre_resources/", "FileSystem", "map");
			resgrpman->addResourceLocation("ogre_resources/textures/", "FileSystem", "map");
			resgrpman->initialiseAllResourceGroups();

			auto m = root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), .1, .1);
			auto e = mainScene->createEntity(m);
			auto l = mainScene->getRootSceneNode()->createChildSceneNode();
			l->attachObject(e);

			auto matman = Ogre::MaterialManager::getSingletonPtr();
			auto mat = matman->getByName("base_hex", "map");

			resgrpman->loadResourceGroup("map");

			e->setMaterial(mat);

//			auto mat = Ogre::MaterialManager::getSingleton().create("hex", "map");
//			mat->setCullingMode(Ogre::CullingMode::CULL_NONE);
		}

		virtual ~MapRenderer() {

		}
	};

	CameraControls::Ptr cameraControls;
	MapRenderer::Ptr mapRenderer;
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

		pogre::mainScene->setAmbientLight(Ogre::ColourValue(.5, .5, .5));

		pogre::cameraControls = pogre::CameraControls::Ptr(new pogre::CameraControls());


		pogre::mapRenderer = pogre::MapRenderer::Ptr(new pogre::MapRenderer(nullptr));
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
		using namespace pogre;
		mapRenderer = MapRenderer::Ptr(new MapRenderer(map));
	}

	void ogreb_cleanup() {
		pogre::cameraControls.reset();
		pogre::root.reset();
	}
}
