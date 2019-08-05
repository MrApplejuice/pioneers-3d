#pragma once

#include <iostream>
#include <memory>
#include <string>

#include <gtk/gtk.h>

#include <Ogre.h>
#include <OgreWindowEventUtilities.h>
#include <Bites/OgreBitesConfigDialog.h>
#include <Bites/OgreInput.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>

extern "C" {
#include <map.h>
}


namespace pogre {
	class Engine;

	typedef std::shared_ptr<Ogre::Root> OgreRootPtr;

	extern OgreRootPtr root;
	extern Engine* mainEngine;

	class CameraControls : public OgreBites::InputListener {
	private:
		Ogre::Camera* camera;
		Ogre::Viewport* viewport;

		Ogre::SceneNode* location;

		bool rightGrabbed;
	public:
		typedef std::shared_ptr<CameraControls> Ptr;

		virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt) override;
		virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override;
		virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override;

		CameraControls();
		virtual ~CameraControls();
	};

	class MapRenderer {
	private:
		::Map* theMap;
	public:
		typedef std::shared_ptr<MapRenderer> Ptr;

		MapRenderer(::Map* _map);
		virtual ~MapRenderer();
	};

	class Engine : public OgreBites::InputListener {
	private:
		Ogre::RenderWindow* window;
	public:
		Ogre::SceneManager* mainScene;
		CameraControls::Ptr cameraControls;
		MapRenderer::Ptr mapRenderer;

	    virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt);
	    virtual bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt);
	    virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt);
	    virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt);

		virtual Ogre::RenderWindow* getWindow() const { return window; }

		Engine(std::string windowName);
		virtual ~Engine();
	};
}
