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

	void PlayerPiece :: returnToStock() {
		if (!inStock()) {
			this->moveSubPosition(
					owner->sceneNode,
					owner->getObjectPosition(
							pieceType,
							owner->getNextObjectSlotIndex(pieceType, false)));
		}
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
		sceneNode->getUserObjectBindings().setUserAny("playerpiece", Ogre::Any((PlayerPiece*) this));
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


	const int Village :: STATIC_TYPE = BUILD_SETTLEMENT;

	void Village :: loadEntity() {
		auto hmesh = mainEngine->root->getMeshManager()->prepare("village.mesh", "map");
		entity = mainEngine->mainScene->createEntity(hmesh);

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		auto baseMaterial = matman->getByName("village_material", "map");

		material = baseMaterial->clone("player_village_material_" + std::to_string((long long) this));
		for (auto techique : material->getTechniques()) {
			for (auto pass : techique->getPasses()) {
				pass->setDiffuse(owner->colour);
				pass->setAmbient(owner->colour);
				pass->setEmissive(owner->colour * .2);
			}
		}

		entity->setMaterial(material);

		sceneNode->attachObject(entity);
		sceneNode->setScale(Ogre::Vector3::UNIT_SCALE * 0.01);
	}

	Village :: Village(const Player* owner) : PlayerPiece(owner, Village::STATIC_TYPE) {
	}

	Village :: ~Village() {
	}


	const int Road :: STATIC_TYPE = BUILD_ROAD;
	const float Road :: ROAD_HEIGHT = HEX_DIAMETER * 0.06;

	void Road :: loadEntity() {
		auto meshMan = mainEngine->root->getMeshManager();
		auto mesh = meshMan->prepare("simpleroad.mesh", "map");

		auto matman = Ogre::MaterialManager::getSingletonPtr();

		std::string materialName = "player_road_material_" + std::to_string(owner->playerId);
		Ogre::MaterialPtr material = matman->getByName(materialName, "map");
		if (!material) {
			auto baseMaterial = matman->getByName("road", "map");
			material = baseMaterial->clone(materialName);

			for (auto techique : material->getTechniques()) {
				for (auto pass : techique->getPasses()) {
					pass->setDiffuse(owner->colour);
					pass->setAmbient(owner->colour);
					pass->setEmissive(owner->colour * .2);
				}
			}
		}

		entity = mainEngine->mainScene->createEntity(mesh);
		entity->setMaterial(material);

		sceneNode->attachObject(entity);
		sceneNode->setScale(Ogre::Vector3::UNIT_SCALE * HEX_DIAMETER / 2);
	}

	Road :: Road(const Player* owner) : PlayerPiece(owner, Road::STATIC_TYPE) {
	}

	Road :: ~Road() {
	}



	template <typename T>
	static int searchSlot(T& v, bool filled) {
		int start = filled ? 0 : v.size() - 1;
		int end = filled ? v.size() : -1;
		for (int i = start; i != end; i += (end >= start ? 1 : -1)) {
			if (filled == (bool) v[i]->inStock()) {
				return i;
			}
		}
		return -1;
	}

	int Player :: getNextObjectSlotIndex(int type, bool filled) const {
		switch (type) {
		case BUILD_SETTLEMENT: return searchSlot(villages, filled);
		default: break;
		}

		LOGIC_ERROR("getNextObjectSlotIndex could not find a valid location");
		return -1;
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
		case -1:
			return Ogre::Vector3(0.2, 0.1, 0) * no;
		case BUILD_SETTLEMENT:
			return getObjectPosition(-1, no) * HEX_DIAMETER;
		case BUILD_ROAD:
			return Ogre::Vector3(0.0, -0.05, 0) + getObjectPosition(-1, no) * 0.4 * HEX_DIAMETER;
		default:
			return Ogre::Vector3::ZERO;
		}
	}

	float Player :: getObjectRotation(int type, int no) const {
		switch (type) {
		case BUILD_SETTLEMENT:
			return -45 + ((no % 2) - 1) * 5 + ((no % 4) - 1) * 2;
		case BUILD_ROAD:
			return -45 + ((no % 4) - 1) * 2;
		default:
			return 0;
		}
	}

	int Player :: countObjects(int type) const {
		switch (type) {
		case BUILD_SETTLEMENT:
			return villages.size();
		case BUILD_ROAD:
			return roads.size();
		default:
			return 0;
		};
	}

	template <typename T>
	static void initPlayerObjects(Player& player, int count, std::vector<typename T::Ptr>& v) {
		for (int i = 0; i < count; i++) {
			typename T::Ptr newItem = typename T::Ptr(new T(&player));
			newItem->postInit();
			newItem->setSubPosition(player.sceneNode, player.getObjectPosition(T::STATIC_TYPE, i));
			newItem->setRotation(player.getObjectRotation(T::STATIC_TYPE, i));
			v.push_back(newItem);
		}
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

		initPlayerObjects<Village>(*this, gameParams->num_build_type[BUILD_SETTLEMENT], villages);
		initPlayerObjects<Road>(*this, gameParams->num_build_type[BUILD_ROAD], roads);

		sceneNode->setVisible(true, true);
	}

	Player :: ~Player() {
		villages.clear();
		roads.clear();

		if (sceneNode) {
			mainEngine->mainScene->destroySceneNode(sceneNode);
		}
	}
}
