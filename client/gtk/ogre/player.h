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

	class PlayerPiece {
	private:
		PlayerPiece(PlayerPiece const &) = delete;
		void operator=(PlayerPiece const &x) = delete;
	protected:
		Ogre::Entity* entity;
		Ogre::Animation* moveAnimation;

		Ogre::MaterialPtr material;

		virtual void loadEntity() = 0;
	public:
		typedef std::shared_ptr<PlayerPiece> Ptr;

		virtual bool inStock();
		virtual void returnToStock();

		virtual void setRotation(float degrees);

		virtual void setGlobalPosition(Ogre::Vector3 position);
		virtual void setSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position);

		virtual void moveGlobalPosition(Ogre::Vector3 position);
		virtual void moveSubPosition(Ogre::SceneNode* node, Ogre::Vector3 position);

		PlayerPiece& postInit();

		Ogre::SceneNode* sceneNode;

		const int id;
		const int pieceType;
		const Player* owner;

		PlayerPiece(const Player* owner, int pieceType);
		virtual ~PlayerPiece();
	};

	class Village : public PlayerPiece {
	protected:
		virtual void loadEntity() override;
	public:
		typedef std::shared_ptr<Village> Ptr;

		Village(const Player* owner);
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

		int getNextObjectSlotIndex(int type, bool filled) const;
		Ogre::Vector3 getObjectPosition(int type, int no) const;
		float getObjectRotation(int type, int no) const;
		int countObjects(int type) const;

		Village* getStockVillage();

		void applyNewMap(MapRenderer::Ptr mapRenderer);

		Player(gint _playerId, int playerNumber);
		virtual ~Player();
	};
}
