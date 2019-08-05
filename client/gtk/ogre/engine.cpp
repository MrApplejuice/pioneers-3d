#include "engine.h"

namespace pogre {
	OgreRootPtr root;
	std::shared_ptr<Engine> mainEngine;

	bool CameraControls :: mousePressed(const OgreBites::MouseButtonEvent& evt) {
		if (evt.button == OgreBites::BUTTON_RIGHT) {
			rightGrabbed = true;
		}
		return true;
	}

	bool CameraControls :: mouseReleased(const OgreBites::MouseButtonEvent& evt) {
		if (evt.button == OgreBites::BUTTON_RIGHT) {
			rightGrabbed = false;
		}
		return true;
	}

	bool CameraControls :: mouseMoved(const OgreBites::MouseMotionEvent& evt) {
		if (rightGrabbed) {
			location->setPosition(
					Ogre::Vector3(-evt.xrel, evt.yrel, 0) * 0.0005 + location->getPosition());
		}
		return true;
	}

	CameraControls :: CameraControls() {
		rightGrabbed = false;

		auto s = mainEngine->mainScene;

		camera = s->createCamera("camera");
		viewport = mainEngine->getWindow()->addViewport(camera);
		viewport->setDimensions(0, 0, 1, 1);
		viewport->setBackgroundColour(Ogre::ColourValue::Red);

		camera->setAutoAspectRatio(true);
		camera->setNearClipDistance(.5);
		camera->setFarClipDistance(10);

		location = s->getRootSceneNode()->createChildSceneNode("camera location");
		location->attachObject(camera);
		location->setPosition(0, -.5, .5);

		location->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TransformSpace::TS_WORLD);
	}

	CameraControls :: ~CameraControls() {
		mainEngine->mainScene->getRootSceneNode()->removeAndDestroyChild(location);
		mainEngine->mainScene->destroyCamera(camera);
	}

	MapRenderer :: MapRenderer(::Map* _map) : theMap(_map) {
		std::cout << "init shader man " << Ogre::RTShader::ShaderGenerator::initialize() << std::endl;

		auto s = mainEngine->mainScene;
		auto rtshare = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
		rtshare->addSceneManager(s);

		auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();

		resgrpman->createResourceGroup("map", false);
		resgrpman->addResourceLocation("ogre_resources/", "FileSystem", "map");
		resgrpman->addResourceLocation("ogre_resources/textures/", "FileSystem", "map");
		resgrpman->initialiseAllResourceGroups();

		auto m = root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), .1, .1);
		auto e = s->createEntity(m);
		auto l = s->getRootSceneNode()->createChildSceneNode();
		l->attachObject(e);

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		auto mat = matman->getByName("base_hex", "map");

		resgrpman->loadResourceGroup("map");

		e->setMaterial(mat);

//			auto mat = Ogre::MaterialManager::getSingleton().create("hex", "map");
//			mat->setCullingMode(Ogre::CullingMode::CULL_NONE);
	}

	MapRenderer :: ~MapRenderer() {
	}

	Engine :: Engine(std::string windowName) {
		root = OgreRootPtr(new Ogre::Root());

		if (!root->restoreConfig()) {
			root->showConfigDialog(OgreBites::getNativeConfigDialog());
		}
		root->initialise(false);

		Ogre::RenderWindowDescription rwDesc;
		rwDesc.width = 600;
		rwDesc.height = 400;
		rwDesc.useFullScreen = false;
		rwDesc.name = "default";
		rwDesc.miscParams["externalWindowHandle"] = windowName;

		window = root->createRenderWindow(rwDesc);

		mainScene = root->createSceneManager(
				Ogre::DefaultSceneManagerFactory::FACTORY_TYPE_NAME,
				"Main Scene");

		mainScene->setAmbientLight(Ogre::ColourValue(.5, .5, .5));

		cameraControls = CameraControls::Ptr(new pogre::CameraControls());

		mapRenderer = MapRenderer::Ptr(new pogre::MapRenderer(nullptr));
	}

	Engine :: ~Engine() {
		cameraControls.reset();
		root.reset();
	}
}
