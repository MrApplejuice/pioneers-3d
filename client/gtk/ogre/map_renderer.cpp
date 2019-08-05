#include "map_renderer.h"

#include <iostream>

#include "engine_base.h"

namespace pogre {
	MapRenderer :: MapRenderer(::Map* _map) : theMap(_map) {
		auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();
		auto s = mainEngine->mainScene;

		auto m = mainEngine->root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), .1, .1);
		auto e = s->createEntity(m);
		auto l = s->getRootSceneNode()->createChildSceneNode();
		l->attachObject(e);

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		auto mat = matman->getByName("base_hex", "map");

		e->setMaterial(mat);

	//			auto mat = Ogre::MaterialManager::getSingleton().create("hex", "map");
	//			mat->setCullingMode(Ogre::CullingMode::CULL_NONE);
	}

	MapRenderer :: ~MapRenderer() {
	}
}
