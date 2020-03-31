/*
 * MapEntities.hpp
 *
 *  Created on: 01.04.2020
 *      Author: paul
 */
#pragma once


#include <Ogre.h>
#include <OgreWindowEventUtilities.h>
#include <Bites/OgreBitesConfigDialog.h>
#include <Bites/OgreInput.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>

extern "C" {
	#include <client.h>
}

#include "map_renderer.h"

namespace pogre {
	class Robber {
	private:
		Ogre::Entity* entity;
		Ogre::SceneNode* node;
	public:
		void remove();
		void setTile(MapTile* tile);

		Robber();
		virtual ~Robber();
	};
}
