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

	class MapLocation {
	private:
	public:
		Ogre::SceneNode* location;

		virtual ~MapLocation() {}
	};

	class SettlementLocation : public MapLocation {
	private:
	public:
		typedef std::shared_ptr<SettlementLocation> Ptr;

		Node* node;

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

	class RoadLocation : public MapLocation {
	private:
	public:
		typedef std::shared_ptr<RoadLocation> Ptr;

		Edge* edge;

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

	enum class HexSceneNodeType {
		ROBBER = 0,
		Count
	};

	class HexLocation : public MapLocation {
	private:
		Ogre::SceneNode* specialNodes[2];

		void newSpecialNode(HexSceneNodeType type, const Ogre::Vector3& relLocation) {
			auto name = location->getName() + "_type:" + std::to_string((int) type);
			specialNodes[(int) type] = location->createChildSceneNode(name, relLocation);
		}
	public:
		typedef std::shared_ptr<HexLocation> Ptr;

		Hex* hex;

		virtual Ogre::SceneNode* getSpecialSceneNode(HexSceneNodeType type) {
			if (type < HexSceneNodeType::Count) {
				if (specialNodes[(int) type]) {
					return specialNodes[(int) type];
				}
			}
			return location;
		}

		HexLocation(HexLocation& other) = delete;
		HexLocation& operator=(HexLocation& other) = delete;

		HexLocation(Ogre::SceneNode* parent, Hex* hex) : hex(hex) {
			for (int i = 0; i < (int) HexSceneNodeType::Count; i++) {
				specialNodes[i] = nullptr;
			}

			location = parent->createChildSceneNode("hex_pos_" + std::to_string(hex->y) + "_" + std::to_string(hex->x));
			location->setPosition(Ogre::Vector3(0, 0, 0));
			location->setInitialState();
			location->setVisible(true);

			newSpecialNode(HexSceneNodeType::ROBBER, Ogre::Vector3(-0.4, 0.3, 0) * HEX_DIAMETER / 2);
		}

		~HexLocation() {
			location->detachAllObjects();
			mainEngine->mainScene->destroySceneNode(location);

			for (int i = 0; i < (int) HexSceneNodeType::Count; i++) {
				if (specialNodes[i]) {
					specialNodes[i]->detachAllObjects();
					mainEngine->mainScene->destroySceneNode(specialNodes[i]);
				}
			}
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
		HexLocation::Ptr location;

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
