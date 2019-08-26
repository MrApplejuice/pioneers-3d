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

	class SettlementLocation {
	private:
	public:
		Node* node;
		Ogre::Vector3 location;

		SettlementLocation(const Ogre::Vector2& hexPos, Node* node) : node(node) {
			Ogre::Quaternion rot(Ogre::Degree(360 / 6 * (node->pos + 0.5)), Ogre::Vector3::UNIT_Z);

			location = HEX_PLACEMENT_MATRIX * Ogre::Vector3(hexPos.x, hexPos.y, 0) + rot * Ogre::Vector3::UNIT_X;
		}
	};

	class MapTile {
	private:
		Hex* hex;

		Ogre::SceneNode* sceneNode;
		Ogre::Entity* entity;
	public:
		typedef std::shared_ptr<MapTile> Ptr;

		std::vector<SettlementLocation> settlementLocations;

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

		float width, height;

		MapTile* getTile(Hex* hex) const;
		SettlementLocation* getSettlementLocation(Node* node);

		MapRenderer(::Map* _map);
		virtual ~MapRenderer();
	};
}

#endif /* CLIENT_GTK_OGRE_MAP_RENDERER_H_ */
