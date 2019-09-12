#pragma once

#ifndef CLIENT_GTK_OGRE_MAP_RENDERER_H_
#define CLIENT_GTK_OGRE_MAP_RENDERER_H_

#include <memory>
#include <map>

#include <Ogre.h>

extern "C" {
	#include <client.h>
}

#include "engine_base.h"

namespace pogre {
	const static float HEX_DIAMETER = 0.1;
	const static float HEX_X_WIDTH = 0.0866;
	const static Ogre::Matrix3 HEX_PLACEMENT_MATRIX(
			0.0866,  -0.0433, 0.0,
			0.0,     -0.075,  0.0,
			0.0,      0.0,    0.1);

	class MapTile;

	class SettlementLocation {
	private:
	public:
		typedef std::shared_ptr<SettlementLocation> Ptr;

		Node* node;
		Ogre::SceneNode* location;

		SettlementLocation(SettlementLocation& other) = delete;
		SettlementLocation& operator=(SettlementLocation& other) = delete;

		SettlementLocation(Ogre::SceneNode* parent, Node* node) : node(node) {
			Ogre::Quaternion rot(Ogre::Degree(360.f / 6.f * (0.5 + node->pos)), Ogre::Vector3::UNIT_Z);

			location = parent->createChildSceneNode("settlement_pos_" + std::to_string(node->y) + "_" + std::to_string(node->x) + "_" + std::to_string(node->pos));
			location->setPosition(rot * Ogre::Vector3::UNIT_X * HEX_DIAMETER / 2);
			location->setInitialState();
			location->setVisible(true);
		}

		~SettlementLocation() {
			location->detachAllObjects();
			mainEngine->mainScene->destroySceneNode(location);
		}
	};

	class RoadLocation {
	private:
	public:
		typedef std::shared_ptr<RoadLocation> Ptr;

		Edge* edge;
		Ogre::SceneNode* location;

		RoadLocation(RoadLocation& other) = delete;
		RoadLocation& operator=(RoadLocation& other) = delete;

		RoadLocation(Ogre::SceneNode* parent, Edge* edge) : edge(edge) {
			Ogre::Quaternion rot(Ogre::Degree(360.f / 6.f * (edge->pos)), Ogre::Vector3::UNIT_Z);

			location = parent->createChildSceneNode("road_pos_" + std::to_string(edge->y) + "_" + std::to_string(edge->x) + "_" + std::to_string(edge->pos));
			location->setPosition(rot * Ogre::Vector3::UNIT_X * HEX_X_WIDTH / 2);
			location->setOrientation(rot * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z));
			location->setInitialState();
			location->setVisible(true);
		}

		~RoadLocation() {
			location->detachAllObjects();
			mainEngine->mainScene->destroySceneNode(location);
		}
	};

	class NumberChip {
	private:
		const MapTile* mapTile;
		Ogre::Entity* entity;
	public:
		Ogre::SceneNode* node;

		NumberChip(MapTile* mapTile);
		virtual ~NumberChip();
	};

	class MapTile {
	private:
		Hex* hex;

		NumberChip* numberChip;

		Ogre::SceneNode* sceneNode;
		Ogre::SceneNode* entityNode;
		Ogre::Entity* entity;
	public:
		typedef std::shared_ptr<MapTile> Ptr;

		std::vector<SettlementLocation::Ptr> settlementLocations;
		std::vector<RoadLocation::Ptr> roadLocations;

		Hex* getHex() const { return hex; }

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

		::Map* getMap() const {
			return theMap;
		}

		MapTile* getTile(Hex* hex) const;
		SettlementLocation* getSettlementLocation(Node* node);
		RoadLocation* getRoadLocation(Edge* edge);

		MapRenderer(::Map* _map);
		virtual ~MapRenderer();
	};
}

#endif /* CLIENT_GTK_OGRE_MAP_RENDERER_H_ */
