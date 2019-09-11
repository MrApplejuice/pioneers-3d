/*
 * engine_base.hpp
 *
 *  Created on: Aug 5, 2019
 *      Author: paul
 */
#pragma once

#ifndef CLIENT_GTK_OGRE_ENGINE_BASE_H_
#define CLIENT_GTK_OGRE_ENGINE_BASE_H_

#include <iostream>

#include <Ogre.h>
#include <Bites/OgreInput.h>

extern "C" {
	#include <client.h>
}

namespace pogre {
	static void LOGIC_ERROR(std::string s) {
		std::cerr << "LOGIC ERROR: " << s << std::endl;
	}

	class Modifiers {
	public:
		static const int CTRL = ::OgreBites::KMOD_CTRL;
		static const int SHIFT = 0x01;
		static const int ALT = 0x02;
		static const int SUPER = 0x04;
	};

	typedef std::shared_ptr<Ogre::Root> OgreRootPtr;

	class EngineBase : public OgreBites::InputListener {
	public:
		OgreRootPtr root;
		Ogre::SceneManager* mainScene;
		Ogre::RenderWindow* window;

		virtual void render(float stepSeconds) = 0;

		virtual void startNewGame() = 0;
		virtual void loadNewMap(Map* map) = 0;

		virtual void updateNode(Node* node) = 0;
		virtual void updateEdge(Edge* edge) = 0;

		virtual void updateWindowSize(int width, int height) = 0;
	};

	extern EngineBase* mainEngine;
}

#endif /* CLIENT_GTK_OGRE_ENGINE_BASE_H_ */
