#pragma once

#ifndef CLIENT_GTK_OGRE_MAP_RENDERER_H_
#define CLIENT_GTK_OGRE_MAP_RENDERER_H_

#include <memory>
#include <map>

#include <Ogre.h>

extern "C" {
	#include <client.h>
}

namespace pogre {
	const static float HEX_X_WIDTH = 0.866;
	const static Ogre::Matrix3 HEX_PLACEMENT_MATRIX(
			0.866,  -0.433, 0.0,
			0.0,    -0.75,  0.0,
			0.0,     0.0,   1.0);

	enum class Location {
		City
	};

	class MapTile {
	private:
		Hex* hex;

		Ogre::SceneNode* sceneNode;
		Ogre::Entity* entity;
	public:
		typedef std::shared_ptr<MapTile> Ptr;

		Hex* getHex() const { return hex; }

		virtual Ogre::Vector3 getLocation(Location l, int pos) const;

		MapTile(const Ogre::Vector2& hexPos, Ogre::SceneNode* parent, Hex* hex);
		virtual ~MapTile();
	};

	class MapRenderer {
	private:
		Ogre::SceneNode* origin;

		Ogre::SceneNode* tableNode;
		Ogre::Entity* tableEntity;

		::Map* theMap;
		MapTile::Ptr tiles[MAP_SIZE][MAP_SIZE];
	public:
		typedef std::shared_ptr<MapRenderer> Ptr;

		MapTile* getTile(Hex* hex) const;

		MapRenderer(::Map* _map);
		virtual ~MapRenderer();
	};
}

#endif /* CLIENT_GTK_OGRE_MAP_RENDERER_H_ */
