#include "map_renderer.h"

#include <iostream>

#include "engine_base.h"

namespace pogre {
	Ogre::Vector3 MapTile :: getLocation(Location l, int pos) const {
		if (l == Location::City) {
			Ogre::Matrix3 rot;
			rot.FromAngleAxis(Ogre::Vector3::UNIT_Z, Ogre::Radian(Ogre::Degree(60 * pos)));
			Ogre::Vector3 basePos(HEX_X_WIDTH, 0.5, 0);
			return sceneNode->convertLocalToWorldPosition(rot * basePos);
		}

		return sceneNode->convertLocalToWorldPosition(Ogre::Vector3::ZERO);
	}

	MapTile :: MapTile(const Ogre::Vector2& hexPos, Ogre::SceneNode* parent, Hex* hex) : hex(hex) {
		auto meshman = mainEngine->root->getMeshManager();

		auto m = meshman->getByName("hex", "map");
		if (!m) {
			m = mainEngine->root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), 0.866, 2 * 0.866);
		}

		entity = mainEngine->mainScene->createEntity(m);
		sceneNode = parent->createChildSceneNode();
		sceneNode->setPosition(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hexPos.x, hexPos.y, 0));
		sceneNode->attachObject(entity);

		auto matman = Ogre::MaterialManager::getSingletonPtr();

		std::string matName = "base_hex";
		switch (hex->terrain) {
		case SEA_TERRAIN: matName = "hex_water"; break;
		case FIELD_TERRAIN: matName = "hex_grain"; break;
		case PASTURE_TERRAIN: matName = "hex_sheep"; break;
		case MOUNTAIN_TERRAIN: matName = "hex_mountain"; break;
		case FOREST_TERRAIN: matName = "hex_wood"; break;
		case DESERT_TERRAIN: matName = "hex_desert"; break;
		case HILL_TERRAIN:  matName = "hex_bricks"; break;
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


	MapTile* MapRenderer :: getTile(Hex* hex) const {
		for (int i = 0; i < MAP_SIZE; i++) {
			for (int j = 0; j < MAP_SIZE; j++) {
				MapTile* p = tiles[i][j].get();
				if (p) {
					if (p->getHex() == hex) {
						return p;
					}
				}
			}
		}
		return nullptr;
	}

	MapRenderer :: MapRenderer(::Map* _map) : theMap(_map) {
		tableEntity = nullptr;

		width = 1;
		height = 1;

		tableNode = mainEngine->mainScene->getRootSceneNode()->createChildSceneNode("table_bg");
		tableNode->setScale(.1, .1, .1);

		origin = tableNode->createChildSceneNode("map_root");
		origin->setPosition(0, 0, .01);

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
			origin->setPosition(origin->getPosition() - center);

			width = (maxPos.x - minPos.x) * tableNode->getScale().x;
			height = (maxPos.y - minPos.y) * tableNode->getScale().y;

			Ogre::MeshManager* meshman = mainEngine->root->getMeshManager();
			auto tableMesh = meshman->createPlane(
					"table-background", "map", Ogre::Plane(0, 0, 1, 0),
					fabs(center.x) * 2 + 5, fabs(center.y) * 2 + 5,
					40, 40, true, 1, 6, 4);
			tableEntity = mainEngine->mainScene->createEntity(tableMesh);
			tableEntity->setMaterialName("wooden_base", "map");
			tableNode->attachObject(tableEntity);
		}

		tableNode->setVisible(true, true);
	}

	MapRenderer :: ~MapRenderer() {
		origin->removeAllChildren();
		if (tableEntity) {
			tableNode->detachObject(tableEntity);
			mainEngine->mainScene->destroyEntity(tableEntity);
			mainEngine->root->getMeshManager()->remove("table-background", "map");
		}
		mainEngine->mainScene->destroySceneNode(origin);
		mainEngine->mainScene->destroySceneNode(tableNode);
	}
}
