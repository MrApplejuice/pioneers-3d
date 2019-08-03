#pragma once

#include <iostream>
#include <memory>

#include <gtk/gtk.h>

#include <Ogre.h>
#include <OgreWindowEventUtilities.h>
#include <Bites/OgreBitesConfigDialog.h>
#include <Bites/OgreInput.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>

namespace pogre {
	typedef std::shared_ptr<Ogre::Root> OgreRootPtr;

	OgreRootPtr root;
	Ogre::RenderWindow* window;
	Ogre::SceneManager* mainScene;

	class WindowListener : public OgreBites::WindowEventListener {
	public:
		virtual void windowClosed(Ogre::RenderWindow* rw) override {
			gtk_main_quit();
		}

		virtual void windowMoved(Ogre::RenderWindow* rw) {
			std::cout << "Window moved" << std::endl;
		}
	};

	class CameraControls : public OgreBites::InputListener {
	private:
		OgreRootPtr _r;

		Ogre::Camera* camera;
		Ogre::Viewport* viewport;

		Ogre::SceneNode* location;

		bool rightGrabbed;
	public:
		typedef std::shared_ptr<CameraControls> Ptr;

		virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt) override {
			if (evt.button == OgreBites::BUTTON_RIGHT) {
				rightGrabbed = true;
			}
			return true;
		}

		virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override {
			if (evt.button == OgreBites::BUTTON_RIGHT) {
				rightGrabbed = false;
			}
			return true;
		}

		virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override {
			if (rightGrabbed) {
				location->setPosition(
						Ogre::Vector3(-evt.xrel, evt.yrel, 0) * 0.0005 + location->getPosition());
			}
			return true;
		}

		CameraControls() {
			_r = root;
			rightGrabbed = false;

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
