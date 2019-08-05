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

#include "engine_base.h"

#include "map_renderer.h"

namespace pogre {
	class Engine;

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

	class Engine : public EngineBase {
	private:
	public:
		CameraControls::Ptr cameraControls;
		MapRenderer::Ptr mapRenderer;

		virtual void render(float stepSeconds);

	    virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt);
	    virtual bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt);
	    virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt);
	    virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt);

		Engine(std::string windowName);
		virtual ~Engine();
	};
}
