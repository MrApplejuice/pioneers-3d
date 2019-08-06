#pragma once

#ifndef CLIENT_GTK_OGRE_MAP_RENDERER_H_
#define CLIENT_GTK_OGRE_MAP_RENDERER_H_

#include <memory>

#include <Ogre.h>

#include <map.h>

namespace pogre {
	const static Ogre::Matrix3 HEX_PLACEMENT_MATRIX(
			0.866,  -0.433, 0.0,
			0.0,    -0.75,  0.0,
			0.0,     0.0,   1.0);

	class MapTile {
	private:
		Hex* hex;

		Ogre::SceneNode* sceneNode;
		Ogre::Entity* entity;
	public:
		typedef std::shared_ptr<MapTile> Ptr;

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

		MapRenderer(::Map* _map);
		virtual ~MapRenderer();
	};
}

#endif /* CLIENT_GTK_OGRE_MAP_RENDERER_H_ */
