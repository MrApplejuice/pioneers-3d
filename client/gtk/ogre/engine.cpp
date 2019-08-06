#include "engine.h"

namespace pogre {
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

    bool CameraControls :: mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) {
    	const Ogre::Matrix3 viewMat = location->getLocalAxes();
    	location->setPosition(location->getPosition() + viewMat.GetColumn(2) * evt.y * 0.01);
    	return true;
    }

	CameraControls :: CameraControls() {
		rightGrabbed = false;

		auto s = mainEngine->mainScene;

		camera = s->createCamera("camera");
		viewport = mainEngine->window->addViewport(camera);
		viewport->setDimensions(0, 0, 1, 1);
		viewport->setBackgroundColour(Ogre::ColourValue::Black);

		camera->setAutoAspectRatio(true);
		camera->setNearClipDistance(.01);
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


	void Engine :: render(float stepSeconds) {
		root->renderOneFrame(stepSeconds);
	}

	void Engine :: loadNewMap(Map* map) {
		std::cout << "Ogre backend - created map" << std::endl;
		mapRenderer.reset();
		mapRenderer = MapRenderer::Ptr(new pogre::MapRenderer(map));
	}

    bool Engine :: mouseMoved(const OgreBites::MouseMotionEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mouseMoved(evt);
    	}
    	return true;
    }

    bool Engine :: mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mouseWheelRolled(evt);
    	}
    	return true;
    }

    bool Engine :: mousePressed(const OgreBites::MouseButtonEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mousePressed(evt);
    	}
    	return true;
    }

    bool Engine :: mouseReleased(const OgreBites::MouseButtonEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mouseReleased(evt);
    	}
    	return true;
    }

	Engine :: Engine(std::string windowName) {
		mainEngine = this;
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


		// Resources
		auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();

		resgrpman->createResourceGroup("map", false);
		resgrpman->addResourceLocation("ogre_resources/", "FileSystem", "map");
		resgrpman->addResourceLocation("ogre_resources/textures/", "FileSystem", "map");
		resgrpman->initialiseAllResourceGroups();


		// Create base scene objects
		mainScene = root->createSceneManager(
				Ogre::DefaultSceneManagerFactory::FACTORY_TYPE_NAME,
				"Main Scene");
		mainScene->setAmbientLight(Ogre::ColourValue(.1, .1, .1));
		cameraControls = CameraControls::Ptr(new pogre::CameraControls());


		auto light = mainScene->createLight("test-light");
		light->setDiffuseColour(1, 0.9, 0.9);
		light->setAttenuation(1, 1, 0, 1);
		auto lightSceneNode = mainScene->getRootSceneNode()->createChildSceneNode(
				"test-light-location", Ogre::Vector3(0, 0, 0.4));
		lightSceneNode->attachObject(light);

		// Extra features
		std::cout << "init shader man " << Ogre::RTShader::ShaderGenerator::initialize() << std::endl;
		auto rtshare = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
		rtshare->addSceneManager(mainScene);
	}

	Engine :: ~Engine() {
		mapRenderer.reset();
		cameraControls.reset();
		root.reset();
		if (mainEngine == this) mainEngine = nullptr;
	}
}
