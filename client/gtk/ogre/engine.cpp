#include "engine.h"

#include <iostream>

namespace pogre {
	bool CameraControls :: isValidCoordinate(const Ogre::Vector3& v) const {
		return (v.z > 0.01) && (Ogre::Vector3::ZERO.distance(v) < MAP_SIZE * 0.1);
	}

	bool CameraControls :: mousePressed(const OgreBites::MouseButtonEvent& evt) {
		if (evt.button == OgreBites::BUTTON_RIGHT) {
			rightGrabbed = true;
		}
		return true;
	}

	bool CameraControls :: mouseReleased(const OgreBites::MouseButtonEvent& evt) {
		if (evt.button == OgreBites::BUTTON_RIGHT) {
			rightGrabbed = false;
		}
		return true;
	}

	bool CameraControls :: mouseMoved(const OgreBites::MouseMotionEvent& evt) {
		float ratioX = 1.0 * evt.x / mainEngine->window->getWidth();
		float ratioY = 1.0 * evt.y / mainEngine->window->getHeight();
		ratioX = 2 * ratioX - 1;
		ratioY = 1 - 2 * ratioY;

		//tiltMatrix
		targetTiltPYR.x = ratioY * 1;
		targetTiltPYR.y = -ratioX * 1;

		if (rightGrabbed) {
	    	auto newLocation = Ogre::Vector3(-evt.xrel, evt.yrel, 0) * 0.0005 + location->getPosition();
	    	if (isValidCoordinate(newLocation)) {
	    		location->setPosition(newLocation);
	    	}
		}
		return true;
	}

    bool CameraControls :: mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) {
    	const Ogre::Matrix3 viewMat = location->getLocalAxes();
    	auto newLocation = location->getPosition() + viewMat.GetColumn(2) * evt.y * -0.05;

    	if (isValidCoordinate(newLocation)) {
    		location->setPosition(newLocation);
    	}
    	return true;
    }

    bool CameraControls :: frameRenderingQueued(const Ogre::FrameEvent& evt)  {
    	auto mixFactor = pow(0.3, evt.timeSinceLastFrame);
    	tiltPYR = mixFactor * tiltPYR + (1 - mixFactor) * (targetTiltPYR - tiltPYR);

    	Ogre::Quaternion pitch, yaw;
    	pitch.FromAngleAxis(Ogre::Degree(tiltPYR.x), Ogre::Vector3::UNIT_X);
    	yaw.FromAngleAxis(Ogre::Degree(tiltPYR.y), Ogre::Vector3::UNIT_Y);
    	tiltNode->setOrientation(pitch * yaw);

    	return true;
    }

	CameraControls :: CameraControls() {
		rightGrabbed = false;

		tiltPYR = Ogre::Vector3::ZERO;
		targetTiltPYR = Ogre::Vector3::ZERO;

		auto s = mainEngine->mainScene;
		mainEngine->root->addFrameListener(this);

		camera = s->createCamera("camera");
		viewport = mainEngine->window->addViewport(camera);
		viewport->setDimensions(0, 0, 1, 1);
		viewport->setBackgroundColour(Ogre::ColourValue::Black);

		camera->setAutoAspectRatio(true);
		camera->setNearClipDistance(.01);
		camera->setFarClipDistance(10);

		location = s->getRootSceneNode()->createChildSceneNode("camera location");
		location->setPosition(0, -.5, .5);
		location->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TransformSpace::TS_WORLD);

		tiltNode = location->createChildSceneNode("camera tilt");
		tiltNode->attachObject(camera);
	}

	CameraControls :: ~CameraControls() {
		mainEngine->root->removeFrameListener(this);
		mainEngine->mainScene->getRootSceneNode()->removeAndDestroyChild(location);
		mainEngine->mainScene->destroyCamera(camera);
	}


	float Engine :: getBoardWidth() {
		if (mapRenderer) return mapRenderer->width;
		return 1.0f;
	}

	float Engine :: getBoardHeight() {
		if (mapRenderer) return mapRenderer->height;
		return 1.0f;
	}

	void Engine :: placePlayers() {
		int oponents = players.size() - (my_player_spectator() ? 0 : 1);
		float leftRightInc = oponents > 1 ? (getBoardWidth() - 0.01 * 2) / (oponents - 1) : 0;
		float leftRight = -(getBoardWidth() / 2 - 0.01);

		for (auto player : players) {
			Ogre::Quaternion rot;
			Ogre::Vector3 pos;

			std::cout << "Placing player " << player->playerId << std::endl;

			if (!my_player_spectator() && (player->playerId == my_player_num())) {
				pos = Ogre::Vector3(0, -getBoardHeight() / 2 - 0.1, 0);
			} else {
				pos = Ogre::Vector3(leftRight, getBoardHeight() / 2 + 0.1, 0);
				rot.FromAngleAxis(Ogre::Degree(180), Ogre::Vector3::UNIT_Z);

				leftRight += leftRightInc;
			}

			player->sceneNode->setPosition(pos);
			player->sceneNode->setOrientation(rot);

			player->applyNewMap(mapRenderer);
		}
	}

	void Engine :: render(float stepSeconds) {
		root->renderOneFrame(stepSeconds);

		if (enableLightMovement) {
			Ogre::Quaternion lightRotation(Ogre::Degree(30 * stepSeconds), Ogre::Vector3::UNIT_Z);
			spotLightNode->setPosition(lightRotation * spotLightNode->getPosition());
		}
	}

	void Engine :: startNewGame() {
		std::cout << "Ogre backend - starting new game" << std::endl;

		int playerNumber = 0;
		players.clear();
		for (gint playerId = 0; playerId < num_players(); playerId++) {
			if (!player_is_spectator(playerId)) {
				auto player = Player::Ptr(new Player(playerId, playerNumber++));
				players.push_back(player);

				Ogre::Vector3 a = player->villages[2]->sceneNode->convertLocalToWorldPosition(Ogre::Vector3::ZERO);
			}
		}

		placePlayers();
	}

	void Engine :: loadNewMap(Map* map) {
		mapRenderer.reset();
		if (map) mapRenderer = MapRenderer::Ptr(new pogre::MapRenderer(map));
		placePlayers();
	}

	void Engine :: updateNode(Node* node) {
		if (!mapRenderer || !players.size()) return;

		auto sloc = mapRenderer->getSettlementLocation(node);
		if (!sloc) {
			LOGIC_ERROR(std::string(__FUNCTION__) + " could not find object to update");
			return;
		}

		PlayerPiece* pp = nullptr;
		if (sloc->location->getChildren().size()) {
			Ogre::Node* n = sloc->location->getChild(0);
			pp = Ogre::any_cast<PlayerPiece*>(n->getUserObjectBindings().getUserAny("playerpiece"));
		}
		if (node->owner == -1) {
			if (pp) pp->returnToStock();
		} else {
			if (pp) {
				if (pp->owner->playerId == node->owner) {
					// Replace same player
					pp->returnToStock();
				} else {
					// Replace other player
					pp->returnToStock();
				}
			}
			auto newOwner = players[node->owner];
			auto newPiece = newOwner->getStockObject(node->type);
			if (!newPiece) {
				LOGIC_ERROR(std::string(__FUNCTION__) + ": Cannot place a piece that is not in stock");
				return;
			}
			newPiece->moveSubPosition(sloc->location, Ogre::Vector3(0, 0, 0));
		}
	}

	void Engine :: updateEdge(Edge* edge) {
		if (!mapRenderer || !players.size()) return;

		auto rloc = mapRenderer->getRoadLocation(edge);
		if (!rloc) {
			LOGIC_ERROR(std::string(__FUNCTION__) + " could not find object to update");
			return;
		}
		PlayerPiece* pp = nullptr;
		if (rloc->location->getChildren().size()) {
			Ogre::Node* n = rloc->location->getChild(0);
			pp = Ogre::any_cast<PlayerPiece*>(n->getUserObjectBindings().getUserAny("playerpiece"));
		}
		if (edge->owner == -1) {
			if (pp) pp->returnToStock();
		} else {
			if (pp) {
				if (pp->owner->playerId == edge->owner) {
					// Replace same player
					pp->returnToStock();
				} else {
					// Replace other player
					pp->returnToStock();
				}
			}
			auto newOwner = players[edge->owner];
			auto newPiece = newOwner->getStockObject(edge->type);
			if (!newPiece) {
				LOGIC_ERROR(std::string(__FUNCTION__) + ": Cannot place a piece that is not in stock");
				return;
			}
			newPiece->moveSubPosition(rloc->location, Ogre::Vector3(0, 0, 0));
			newPiece->setRotation(0);
		}
	}

    bool Engine :: mouseMoved(const OgreBites::MouseMotionEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mouseMoved(evt);
    	}
    	return true;
    }

    bool Engine :: mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mouseWheelRolled(evt);
    	}
    	return true;
    }

    bool Engine :: mousePressed(const OgreBites::MouseButtonEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mousePressed(evt);
    	}
    	return true;
    }

    bool Engine :: mouseReleased(const OgreBites::MouseButtonEvent& evt) {
    	if (cameraControls) {
    		cameraControls->mouseReleased(evt);
    	}
    	return true;
    }

    bool Engine :: keyPressed(const OgreBites::KeyboardEvent& evt) {
    	if (evt.keysym.sym == OgreBites::SDLK_F1) {
    		enableLightMovement = !enableLightMovement;
    	}
    	return true;
    }

	void Engine :: updateWindowSize(int width, int height) {
		mainEngine->window->resize(width, height);
	}

	Engine :: Engine(std::string windowName) : enableLightMovement(false) {
		mainEngine = this;
		root = OgreRootPtr(new Ogre::Root());

		if (!root->restoreConfig()) {
			root->showConfigDialog(OgreBites::getNativeConfigDialog());
		}
		root->initialise(false);

		Ogre::RenderWindowDescription rwDesc;
		rwDesc.width = 600;
		rwDesc.height = 400;
		rwDesc.useFullScreen = false;
		rwDesc.name = "default";
		rwDesc.miscParams["externalWindowHandle"] = windowName;

		window = root->createRenderWindow(rwDesc);


		// Resources
		auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();

		resgrpman->createResourceGroup("map", false);
		resgrpman->addResourceLocation("ogre_resources/", "FileSystem", "map");
		resgrpman->addResourceLocation("ogre_resources/textures/", "FileSystem", "map");
		resgrpman->addResourceLocation("ogre_resources/meshes/", "FileSystem", "map");
		resgrpman->addResourceLocation("ogre_resources/shaders/", "FileSystem", "map");
		resgrpman->initialiseAllResourceGroups();


		// Create base scene objects
		mainScene = root->createSceneManager(
				Ogre::DefaultSceneManagerFactory::FACTORY_TYPE_NAME,
				"Main Scene");
		mainScene->setAmbientLight(Ogre::ColourValue(0.1, 0.1, 0.1));
		cameraControls = CameraControls::Ptr(new pogre::CameraControls());


		spotLight = mainScene->createLight("spot-light");
		spotLight->setDiffuseColour(1, 0.9, 0.9);
		spotLight->setSpecularColour(1, 1, 0.9);
		spotLight->setAttenuation(2, .1, 1, 1);

		spotLightNode = mainScene->getRootSceneNode()->createChildSceneNode(
				"spot-light-location", Ogre::Vector3(0.4, 0, 0.4));
		spotLightNode->attachObject(spotLight);

		// Extra features
		std::cout << "init shader man " << Ogre::RTShader::ShaderGenerator::initialize() << std::endl;
		auto rtshare = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
		rtshare->addSceneManager(mainScene);
	}

	Engine :: ~Engine() {
		players.clear();
		mapRenderer.reset();
		cameraControls.reset();
		root.reset();
		if (mainEngine == this) mainEngine = nullptr;
	}
}
