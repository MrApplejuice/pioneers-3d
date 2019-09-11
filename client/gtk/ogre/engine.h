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
	#include <client.h>
}

#include "engine_base.h"

#include "player.h"
#include "map_renderer.h"

namespace pogre {
	class Engine;

	class CameraControls : public OgreBites::InputListener, public Ogre::FrameListener {
	private:
		Ogre::Camera* camera;
		Ogre::Viewport* viewport;

		Ogre::SceneNode* location;
		Ogre::SceneNode* tiltNode;

		Ogre::Vector3 tiltPYR, targetTiltPYR;

		bool rightGrabbed;

		bool isValidCoordinate(const Ogre::Vector3& v) const;
	public:
		typedef std::shared_ptr<CameraControls> Ptr;

		virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt) override;
		virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override;
		virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override;
	    virtual bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) override;

	    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt) override;

		CameraControls();
		virtual ~CameraControls();
	};

	class Engine : public EngineBase {
	private:
		std::vector<Player::Ptr> players;

		bool enableLightMovement;

		Ogre::SceneNode* spotLightNode;
		Ogre::Light* spotLight;

		float getBoardWidth();
		float getBoardHeight();

		void placePlayers();
	public:
		CameraControls::Ptr cameraControls;
		MapRenderer::Ptr mapRenderer;

		virtual void render(float stepSeconds) override;

		virtual void startNewGame() override;
		virtual void loadNewMap(Map* map) override;
		virtual void updateNode(Node* node) override;
		virtual void updateEdge(Edge* edge) override;

	    virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override;
	    virtual bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) override;
	    virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt) override;
	    virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override;

	    virtual bool keyPressed(const OgreBites::KeyboardEvent& evt) override;

		virtual void updateWindowSize(int width, int height);

		Engine(std::string windowName);
		virtual ~Engine();
	};
}
