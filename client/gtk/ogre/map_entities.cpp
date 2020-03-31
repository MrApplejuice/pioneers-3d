#include "map_entities.h"

#include "engine_base.h"

namespace pogre {
	void Robber :: remove() {
		if (node->getParentSceneNode()) {
			node->getParentSceneNode()->removeChild(node);
		}
	}

	void Robber :: setTile(MapTile* tile) {
		auto sceneNode = tile->location->getSpecialSceneNode(HexSceneNodeType::ROBBER);
		sceneNode->addChild(node);
	}

	Robber :: Robber() {
		node = mainEngine->mainScene->createSceneNode("robber");

		auto meshman = mainEngine->root->getMeshManager();
		auto mesh = meshman->prepare("robber.mesh", "map");

		entity = mainEngine->mainScene->createEntity(mesh);

		auto matman = Ogre::MaterialManager::getSingletonPtr();
		entity->setMaterialName("RobberMaterial", "map");

		node->attachObject(entity);
		node->setVisible(true, true);
		node->setScale(Ogre::Vector3::UNIT_SCALE * 0.05);
	}

	Robber :: ~Robber() {
		node->detachAllObjects();
		mainEngine->mainScene->destroySceneNode(node);
		mainEngine->mainScene->destroyEntity(entity);
	}
}
