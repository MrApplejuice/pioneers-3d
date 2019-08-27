#pragma once

#include <memory>
#include <vector>

#include "engine_base.h"
#include "map_renderer.h"

extern "C" {
	#include <client.h>
}

namespace pogre {
	class Player;

	class Village {
	private:
		Village(Village const &) = delete;
		void operator=(Village const &x) = delete;
	protected:
		Ogre::Entity* entity;
		Ogre::Animation* moveAnimation;

		Ogre::MaterialPtr material;

		void loadEntity();
	public:
		typedef std::shared_ptr<Village> Ptr;

		Ogre::SceneNode* sceneNode;

		const int id;
		const Player* owner;

		virtual bool inStock();

		virtual void setRotation(float degrees);

		virtual void setGlobalPosition(Ogre::Vector3 position);
		virtual void setSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position);

		virtual void moveGlobalPosition(Ogre::Vector3 position);
		virtual void moveSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position);

		Village& postInit();

		Village(const Player* owner, int id);
		virtual ~Village();
	};

	class Player {
	private:
	public:
		typedef std::shared_ptr<Player> Ptr;

		const gint playerId;
		Ogre::SceneNode* sceneNode;

		std::vector<Village::Ptr> villages;

		Ogre::ColourValue colour;

		Ogre::Vector3 getObjectPosition(int type, int no) const;
		float getObjectRotation(int type, int no) const;

		Village* getStockVillage();

		void applyNewMap(MapRenderer::Ptr mapRenderer);

		Player(gint _playerId, int playerNumber);
		virtual ~Player();
	};
}
