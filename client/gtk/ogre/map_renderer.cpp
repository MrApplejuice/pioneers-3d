#include "map_renderer.h"

#include <iostream>

#include "engine_base.h"

namespace pogre {
	MapTile :: MapTile(Ogre::SceneNode* parent, Hex* hex) : hex(hex) {
		auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();
		auto s = mainEngine->mainScene;
		auto meshman = mainEngine->root->getMeshManager();

		auto m = meshman->getByName("hex", "map");
		if (!m) {
			m = mainEngine->root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), 0.866, 2 * 0.866);
		}

		auto e = s->createEntity(m);
		auto l = parent->createChildSceneNode();
		l->setPosition(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hex->x - hex->y / 2, hex->y, 0));
		l->attachObject(e);

		auto matman = Ogre::MaterialManager::getSingletonPtr();

		std::string matName = "base_hex";
		switch (hex->terrain) {
		case SEA_TERRAIN: matName = "hex_water"; break;
		default: ;
		}
		auto mat = matman->getByName(matName, "map");

		e->setMaterial(mat);

	//			auto mat = Ogre::MaterialManager::getSingleton().create("hex", "map");
	//			mat->setCullingMode(Ogre::CullingMode::CULL_NONE);
	}

	MapRenderer :: MapRenderer(::Map* _map) : theMap(_map) {
		origin = mainEngine->mainScene->getRootSceneNode()->createChildSceneNode("map_root");
		origin->setScale(.1, .1, .1);

		for (int x = 0; x < MAP_SIZE; x++) {
			for (int y = 0; y < MAP_SIZE; y++) {
				if (theMap->grid[x][y]) {
					tiles[x][y] = MapTile::Ptr(new MapTile(
							origin, theMap->grid[x][y]));
				}
			}
		}

		origin->setVisible(true, true);
		mainEngine->mainScene->setAmbientLight(Ogre::ColourValue::Blue);
	}

	MapRenderer :: ~MapRenderer() {
		origin->detachAllObjects();
		mainEngine->mainScene->destroySceneNode(origin);
	}
}
