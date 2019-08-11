#include "player.h"

#include <iostream>
#include <string>

namespace pogre {
	void Village :: loadEntity() {
		auto hmesh = mainEngine->root->getMeshManager()->prepare("village.mesh", "map");
		entity = mainEngine->mainScene->createEntity(hmesh);
		entity->setMaterialName("village_material", "map");

		sceneNode->attachObject(entity);
		sceneNode->setScale(Ogre::Vector3::UNIT_SCALE * 0.01);
	}

	void Village :: setGlobalPosition(Ogre::Vector3 position) {
		auto parent = sceneNode->getParentSceneNode();
		if (parent) parent->removeChild(sceneNode);

		mainEngine->mainScene->getRootSceneNode()->addChild(sceneNode);
		sceneNode->setPosition(position);
	}

	void Village :: setSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position) {
		auto parent = sceneNode->getParentSceneNode();
		if (parent) parent->removeChild(sceneNode);

		if (node) {
			node->addChild(sceneNode);
			sceneNode->setPosition(position);
		}
	}

	void Village :: moveGlobalPosition(Ogre::Vector3 position) {
		setGlobalPosition(position);
	}

	void Village :: moveSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position) {
		setSubPosition(node, position);
	}

	Village& Village :: postInit() {
		loadEntity();
		return *this;
	}

	Village :: Village(const Player* owner, int id) : entity(nullptr), moveAnimation(nullptr), sceneNode(nullptr), id(id), owner(owner) {
		sceneNode = mainEngine->mainScene->createSceneNode();
		moveAnimation = mainEngine->mainScene->createAnimation(
				"animate_village_p" + std::to_string(owner->playerId) + "_" + std::to_string(id), 1.0);
	}

	Village :: ~Village() {
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
	}


	Player :: Player(gint _playerId) : playerId(_playerId), sceneNode(nullptr) {
		::Player* player = player_get(_playerId);

		sceneNode = mainEngine->mainScene->getRootSceneNode()->createChildSceneNode(
				"player_" + std::to_string(playerId));

		std::cout << "Player id = " << playerId << "    desc = " << player->style << std::endl;

		auto gameParams = get_game_params();
		for (int vi = 0; vi < gameParams->num_build_type[BUILD_SETTLEMENT]; vi++) {
			auto newVillage = Village::Ptr(&(new Village(this, vi))->postInit());
			newVillage->setSubPosition(sceneNode, Ogre::Vector3(0.02, 0.01, 0) * vi);
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
