#include "map_renderer.h"

#include <iostream>

#include "engine_base.h"

namespace pogre {
	NumberChip :: NumberChip(MapTile* mapTile) : mapTile(mapTile), entity(nullptr), node(nullptr) {
		auto meshman = mainEngine->root->getMeshManager();
		auto mesh = meshman->prepare("numchip.mesh", "map");
		if (!mesh) {
			LOGIC_ERROR("numchip.mesh cannot be loaded");
			return;
		}

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		auto material = matman->getByName(std::string("numchip-") + std::to_string(mapTile->getHex()->roll), "map");

		entity = mainEngine->mainScene->createEntity(mesh);
		if (material) {
			entity->setMaterial(material);
		}

		node = mainEngine->mainScene->createSceneNode();
		node->setScale(Ogre::Vector3::UNIT_SCALE * HEX_DIAMETER * 0.3);
		node->setOrientation(Ogre::Quaternion(Ogre::Degree(rand() % 40 - 20), Ogre::Vector3::UNIT_Z));

		node->attachObject(entity);
	}

	NumberChip :: ~NumberChip() {
		if (node) {
			node->detachAllObjects();
			mainEngine->mainScene->destroySceneNode(node);
		}
		if (entity) {
			mainEngine->mainScene->destroyEntity(entity);
		}
	}


	MapTile :: MapTile(const Ogre::Vector2& hexPos, Ogre::SceneNode* parent, Hex* hex) : hex(hex), numberChip(nullptr), entityNode(nullptr), entity(nullptr) {
		// GRAPHICS
		auto meshman = mainEngine->root->getMeshManager();

		auto scale = Ogre::Vector3::UNIT_SCALE;
		auto m = meshman->getByName("hex", "map");
		if (!m) {
			m = mainEngine->root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), HEX_X_WIDTH, 2 * HEX_X_WIDTH);
		}

		auto matman = Ogre::MaterialManager::getSingletonPtr();

		std::string matName = "base_hex";
		switch (hex->terrain) {
		case SEA_TERRAIN: matName = "hex_water"; break;
		case FIELD_TERRAIN: matName = "hex_grain"; break;
		case PASTURE_TERRAIN: matName = "hex_sheep"; break;
		case MOUNTAIN_TERRAIN: matName = "hex_mountain"; break;
		case FOREST_TERRAIN: matName = "hex_wood"; break;
		case DESERT_TERRAIN:
			matName = "";
			m = meshman->prepare("desert.mesh", "map");
			scale *= HEX_DIAMETER / 2 * 0.975;
			break;
		case HILL_TERRAIN:  matName = "hex_bricks"; break;
		default: ;
		}

		sceneNode = parent->createChildSceneNode();
		sceneNode->setPosition(HEX_PLACEMENT_MATRIX * Ogre::Vector3(hexPos.x, hexPos.y, 0));

		entityNode = sceneNode->createChildSceneNode();
		entityNode->setScale(scale);
		if (m) {
			entity = mainEngine->mainScene->createEntity(m);
			entityNode->attachObject(entity);
		}

		if (entity && !matName.empty()) {
			auto mat = matman->getByName(matName, "map");
			entity->setMaterial(mat);
		}

		if ((hex->roll >= 2) && (hex->roll <= 12)) {
			numberChip = new NumberChip(this);
			sceneNode->addChild(numberChip->node);
		}

		// LOGIC
		for (Node** nodePtr = hex->nodes; nodePtr != hex->nodes + 6; nodePtr++) {
			Node* node = *nodePtr;
			if (!node) continue;
			if (!is_node_on_land(node)) continue;
			if ((node->x != hex->x) || (node->y != hex->y)) continue;

			settlementLocations.push_back(SettlementLocation::Ptr(new SettlementLocation(sceneNode, node)));
		}
		for (Edge** edgePtr = hex->edges; edgePtr != hex->edges + 6; edgePtr++) {
			Edge* edge = *edgePtr;
			if (!edge) continue;
			if ((edge->x != hex->x) || (edge->y != hex->y)) continue;

			roadLocations.push_back(RoadLocation::Ptr(new RoadLocation(sceneNode, edge)));
		}
		location = HexLocation::Ptr(new HexLocation(sceneNode, hex));
	}

	MapTile :: ~MapTile() {
		if (numberChip) {
			delete numberChip;
		}

		auto scene = mainEngine->mainScene;

		sceneNode->detachAllObjects();
		entityNode->detachAllObjects();

		settlementLocations.clear();
		roadLocations.clear();
		location.reset();

		scene->destroySceneNode(sceneNode);
		scene->destroySceneNode(entityNode);
		if (entity) scene->destroyEntity(entity);
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

	SettlementLocation* MapRenderer :: getSettlementLocation(Node* node) {
		Hex* hex = node->map->grid[node->y][node->x];
		MapTile* mtile = getTile(hex);
		if (!mtile) return nullptr;

		for (auto sl : mtile->settlementLocations) {
			if (sl->node == node) {
				return sl.get();
			}
		}
		return nullptr;
	}

	RoadLocation* MapRenderer :: getRoadLocation(Edge* edge) {
		Hex* hex = edge->map->grid[edge->y][edge->x];
		MapTile* mtile = getTile(hex);
		if (!mtile) return nullptr;

		for (auto rl : mtile->roadLocations) {
			if (rl->edge == edge) {
				return rl.get();
			}
		}
		return nullptr;
	}

	MapRenderer :: MapRenderer(::Map* _map) : theMap(_map) {
		tableEntity = nullptr;

		width = 1;
		height = 1;

		tableNode = mainEngine->mainScene->getRootSceneNode()->createChildSceneNode("table_bg");

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
					fabs(center.x) * 2 + .75, fabs(center.y) * 2 + .75,
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
