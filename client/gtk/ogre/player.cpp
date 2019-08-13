#include "player.h"

#include <iostream>
#include <string>

namespace pogre {
	void Village :: loadEntity() {
		auto hmesh = mainEngine->root->getMeshManager()->prepare("village.mesh", "map");
		entity = mainEngine->mainScene->createEntity(hmesh);

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		auto baseMaterial = matman->getByName("village_material", "map");

		material = baseMaterial->clone("player_village_material_" + std::to_string((long long) this));
		material->setDiffuse(owner->colour);
		entity->setMaterial(material);

		sceneNode->attachObject(entity);
		sceneNode->setScale(Ogre::Vector3::UNIT_SCALE * 0.01);
	}

	void Village :: setRotation(float degrees) {
		Ogre::Quaternion q;
		q.FromAngleAxis(Ogre::Degree(degrees), Ogre::Vector3::UNIT_Z);
		sceneNode->setOrientation(q);
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
		if (material) {
			auto matman = Ogre::MaterialManager::getSingletonPtr();
			matman->remove(material);
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
		for (int vi = 0; vi < gameParams->num_build_type[BUILD_SETTLEMENT]; vi++) {
			auto newVillage = Village::Ptr(&(new Village(this, vi))->postInit());
			newVillage->setSubPosition(sceneNode, Ogre::Vector3(0.02, 0.01, 0) * vi);
			newVillage->setRotation(-45 + ((vi % 2) - 1) * 5 + ((vi % 4) - 1) * 2);
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
