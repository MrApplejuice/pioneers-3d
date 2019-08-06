#include "map_renderer.h"

#include <iostream>

#include "engine_base.h"

namespace pogre {
	MapTile :: MapTile(const Ogre::Vector2& hexPos, Ogre::SceneNode* parent, Hex* hex) : hex(hex) {
		auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();
		auto s = mainEngine->mainScene;
		auto meshman = mainEngine->root->getMeshManager();

		auto m = meshman->getByName("hex", "map");
		if (!m) {
			m = mainEngine->root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), 0.866, 2 * 0.866);
		}

		entity = s->createEntity(m);
		sceneNode = parent->createChildSceneNode();
		sceneNode->setPosition(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hexPos.x, hexPos.y, 0));
		sceneNode->attachObject(entity);

		auto matman = Ogre::MaterialManager::getSingletonPtr();

		std::string matName = "base_hex";
		switch (hex->terrain) {
		case SEA_TERRAIN: matName = "hex_water"; break;
		case FIELD_TERRAIN: matName = "hex_grain"; break;
		case PASTURE_TERRAIN: matName = "hex_sheep"; break;
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
			Ogre::Vector3 minPos, maxPos;
			for (int x = 0; x < MAP_SIZE; x++) {
				for (int y = 0; y < MAP_SIZE; y++) {
					if (auto hex = theMap->grid[x][y]) {
						auto hexPos = Ogre::Vector2(hex->x + (hex->y + 1) / 2, hex->y);
						tiles[x][y] = MapTile::Ptr(new MapTile(
								hexPos, origin, hex));
						minPos.makeFloor(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hexPos.x, hexPos.y, 0));
						maxPos.makeCeil(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hexPos.x, hexPos.y, 0));
					}
				}
			}

			Ogre::Vector3 center = minPos.midPoint(maxPos);
			origin->setPosition(-center * origin->getScale());
		}

		origin->setVisible(true, true);
	}

	MapRenderer :: ~MapRenderer() {
		origin->removeAllChildren();
		mainEngine->mainScene->destroySceneNode(origin);
	}
}
