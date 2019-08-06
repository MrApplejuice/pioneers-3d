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

		entity = s->createEntity(m);
		sceneNode = parent->createChildSceneNode();
		sceneNode->setPosition(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hex->x - hex->y / 2, hex->y, 0));
		sceneNode->attachObject(entity);

		auto matman = Ogre::MaterialManager::getSingletonPtr();

		std::string matName = "base_hex";
		switch (hex->terrain) {
		case SEA_TERRAIN: matName = "hex_water"; break;
		case FIELD_TERRAIN: matName = "hex_grain"; break;
		default: ;
		}
		auto mat = matman->getByName(matName, "map");

		entity->setMaterial(mat);
	}

	MapTile :: ~MapTile() {
		auto scene = mainEngine->mainScene;
		sceneNode->detachAllObjects();
		scene->destroySceneNode(sceneNode);
		scene->destroyEntity(entity);
	}


	MapRenderer :: MapRenderer(::Map* _map) : theMap(_map) {
		origin = mainEngine->mainScene->getRootSceneNode()->createChildSceneNode("map_root");
		origin->setScale(.1, .1, .1);

		if (theMap) {
			for (int x = 0; x < MAP_SIZE; x++) {
				for (int y = 0; y < MAP_SIZE; y++) {
					if (theMap->grid[x][y]) {
						tiles[x][y] = MapTile::Ptr(new MapTile(
								origin, theMap->grid[x][y]));
					}
				}
			}

			origin->setPosition(HEX_PLACEMENT_MATRIX * -Ogre::Vector3(theMap->x_size - theMap->y_size / 2 - 1, theMap->y_size - 1, 0) / 2 * origin->getScale());
		}

		origin->setVisible(true, true);
	}

	MapRenderer :: ~MapRenderer() {
		origin->removeAllChildren();
		mainEngine->mainScene->destroySceneNode(origin);
	}
}
