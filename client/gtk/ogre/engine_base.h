/*
 * engine_base.hpp
 *
 *  Created on: Aug 5, 2019
 *      Author: paul
 */
#pragma once

#ifndef CLIENT_GTK_OGRE_ENGINE_BASE_H_
#define CLIENT_GTK_OGRE_ENGINE_BASE_H_

#include <Ogre.h>
#include <Bites/OgreInput.h>

#include <map.h>

namespace pogre {
	typedef std::shared_ptr<Ogre::Root> OgreRootPtr;

	class EngineBase : public OgreBites::InputListener {
	public:
		OgreRootPtr root;
		Ogre::SceneManager* mainScene;
		Ogre::RenderWindow* window;

		virtual void render(float stepSeconds) = 0;

		virtual void loadNewMap(Map* map) = 0;
	};

	extern EngineBase* mainEngine;
}

#endif /* CLIENT_GTK_OGRE_ENGINE_BASE_H_ */
