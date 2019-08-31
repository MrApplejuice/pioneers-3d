#include "player.h"

#include <iostream>
#include <string>

namespace pogre {
	bool PlayerPiece :: inStock() {
		Ogre::Node* node = sceneNode;
		while (node) {
			if (node == owner->sceneNode) {
				return true;
			}
			node = node->getParent();
		}
		return false;
	}

	void PlayerPiece :: setRotation(float degrees) {
		Ogre::Quaternion q;
		q.FromAngleAxis(Ogre::Degree(degrees), Ogre::Vector3::UNIT_Z);
		sceneNode->setOrientation(q);
	}

	void PlayerPiece :: setGlobalPosition(Ogre::Vector3 position) {
		auto parent = sceneNode->getParentSceneNode();
		if (parent) parent->removeChild(sceneNode);

		mainEngine->mainScene->getRootSceneNode()->addChild(sceneNode);
		sceneNode->setPosition(position);
	}

	void PlayerPiece :: setSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position) {
		auto parent = sceneNode->getParentSceneNode();
		if (parent) parent->removeChild(sceneNode);

		if (node) {
			node->addChild(sceneNode);
			sceneNode->setPosition(position);
		}
	}

	void PlayerPiece :: moveGlobalPosition(Ogre::Vector3 position) {
		setGlobalPosition(position);
	}

	void PlayerPiece :: moveSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position) {
		setSubPosition(node, position);
	}

	PlayerPiece& PlayerPiece :: postInit() {
		loadEntity();
		return *this;
	}

	PlayerPiece :: PlayerPiece(const Player* owner, const int pieceType) :
			entity(nullptr), moveAnimation(nullptr), material(nullptr), sceneNode(nullptr),
			owner(owner), id(owner->countObjects(pieceType)), pieceType(pieceType) {
		sceneNode = mainEngine->mainScene->createSceneNode();
		moveAnimation = mainEngine->mainScene->createAnimation(
				"animate_piece_" + std::to_string(pieceType) + "_p"
				+ std::to_string(owner->playerId) + "_" + std::to_string(id), 1.0);
	}

	PlayerPiece :: ~PlayerPiece() {
		if (moveAnimation) {
			moveAnimation->destroyAllTracks();
			mainEngine->mainScene->destroyAnimation(moveAnimation->getName());
		}
		if (sceneNode) {
			sceneNode->detachAllObjects();
			mainEngine->mainScene->destroySceneNode(sceneNode);
		}
		if (entity) {
			mainEngine->mainScene->destroyEntity(entity);
		}
		if (material) {
			auto matman = Ogre::MaterialManager::getSingletonPtr();
			matman->remove(material);
		}
	}


	void Village :: loadEntity() {
		auto hmesh = mainEngine->root->getMeshManager()->prepare("village.mesh", "map");
		entity = mainEngine->mainScene->createEntity(hmesh);

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		auto baseMaterial = matman->getByName("village_material", "map");

		material = baseMaterial->clone("player_village_material_" + std::to_string((long long) this));
		material->setDiffuse(owner->colour);
		material->setAmbient(owner->colour * 0.25);
		entity->setMaterial(material);

		sceneNode->attachObject(entity);
		sceneNode->setScale(Ogre::Vector3::UNIT_SCALE * 0.01);
	}

	Village :: Village(const Player* owner) : PlayerPiece(owner, BUILD_SETTLEMENT) {
	}

	Village :: ~Village() {
	}


	Village* Player :: getStockVillage() {
		for (auto village : villages) {
			if (village->inStock()) {
				return village.get();
			}
		}
		return nullptr;
	}

	void Player :: applyNewMap(MapRenderer::Ptr mapRenderer) {
		// RESET OBJECTS
		int i = 0;
		for (auto village : villages) {
			village->setSubPosition(sceneNode, getObjectPosition(BUILD_SETTLEMENT, i));
			village->setRotation(getObjectRotation(BUILD_SETTLEMENT, i));
			i++;
		}

		auto map = mapRenderer->getMap();
		for (int x = 0; x < MAP_SIZE; x++) {
			for (int y = 0; y < MAP_SIZE; y++) {
				Hex* hex = map->grid[y][x];
				if (!hex) continue;
				for (int ni = 0; ni < 6; ni++) {
					Node* node = hex->nodes[ni];
					if (!node) continue;
					if (node->x != x) continue;
					if (node->y != y) continue;

					if (node->owner == playerId) {
						if (node->type == BUILD_SETTLEMENT) {
							// HACK
							auto setLoc = mapRenderer->getSettlementLocation(node);
							auto village = getStockVillage();
							if (village) {
								village->setSubPosition(setLoc->location, Ogre::Vector3::ZERO);
							}
						}
					}
				}
			}
		}
	}

	Ogre::Vector3 Player :: getObjectPosition(int type, int no) const {
		switch (type) {
		case BUILD_SETTLEMENT:
			return Ogre::Vector3(0.02, 0.01, 0) * no;
		default:
			return Ogre::Vector3::ZERO;
		}
	}

	float Player :: getObjectRotation(int type, int no) const {
		switch (type) {
		case BUILD_SETTLEMENT:
			return -45 + ((no % 2) - 1) * 5 + ((no % 4) - 1) * 2;
		default:
			return 0;
		}
	}

	int Player :: countObjects(int type) const {
		switch (type) {
		case BUILD_SETTLEMENT:
			return villages.size();
		default:
			return 0;
		};
	}

	Player :: Player(gint _playerId, int playerNumber) : playerId(_playerId), sceneNode(nullptr) {
		::Player* player = player_get(_playerId);

		const static Ogre::ColourValue COLOR_ROTATION[8] = {
			Ogre::ColourValue(0.5, 0.5, 0.5, 1),
			Ogre::ColourValue(1.0, 0.0, 0.0, 1),
			Ogre::ColourValue(0.0, 0.0, 1.0, 1),
			Ogre::ColourValue(0.0, 1.0, 0.0, 1),
			Ogre::ColourValue(1.0, 1.0, 0.0, 1),
			Ogre::ColourValue(1.0, 0.0, 1.0, 1),
			Ogre::ColourValue(0.0, 1.0, 1.0, 1),
			Ogre::ColourValue(1.0, 1.0, 1.0, 1),
		};
		colour = COLOR_ROTATION[playerNumber % 8];

		sceneNode = mainEngine->mainScene->getRootSceneNode()->createChildSceneNode(
				"player_" + std::to_string(playerId));

		std::cout << "Player id = " << playerId << std::endl;

		auto gameParams = get_game_params();
		for (int vi = 0; vi < gameParams->num_build_type[BUILD_SETTLEMENT]; vi++) {
			auto newVillage = Village::Ptr(new Village(this));
			newVillage->postInit();
			newVillage->setSubPosition(sceneNode, getObjectPosition(BUILD_SETTLEMENT, vi));
			newVillage->setRotation(getObjectRotation(BUILD_SETTLEMENT, vi));
			villages.push_back(newVillage);
		}

		sceneNode->setVisible(true, true);
	}

	Player :: ~Player() {
		villages.clear();

		if (sceneNode) {
			mainEngine->mainScene->destroySceneNode(sceneNode);
		}
	}
}
