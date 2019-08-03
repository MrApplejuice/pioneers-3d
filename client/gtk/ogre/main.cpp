#include "main.h"

#include <iostream>
#include <memory>
#include <vector>
#include <typeinfo>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <Ogre.h>
#include <OgreWindowEventUtilities.h>
#include <Bites/OgreBitesConfigDialog.h>
#include <Bites/OgreInput.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>

#include <SDL.h>

namespace pogre {
	typedef std::shared_ptr<Ogre::Root> OgreRootPtr;

	OgreRootPtr root;
	Ogre::RenderWindow* window;

	Ogre::SceneManager* mainScene;

	class WindowListener : public OgreBites::WindowEventListener {
	public:
		virtual void windowClosed(Ogre::RenderWindow* rw) override {
			gtk_main_quit();
		}

		virtual void windowMoved(Ogre::RenderWindow* rw) {
			std::cout << "Window moved" << std::endl;
		}
	};

	class CameraControls : public OgreBites::InputListener {
	private:
		OgreRootPtr _r;

		Ogre::Camera* camera;
		Ogre::Viewport* viewport;

		Ogre::SceneNode* location;

		bool rightGrabbed;
	public:
		typedef std::shared_ptr<CameraControls> Ptr;

		virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt) override {
			if (evt.button == OgreBites::BUTTON_RIGHT) {
				rightGrabbed = true;
			}
			return true;
		}

		virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override {
			if (evt.button == OgreBites::BUTTON_RIGHT) {
				rightGrabbed = false;
			}
			return true;
		}

		virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override {
			if (rightGrabbed) {
				location->setPosition(
						Ogre::Vector3(-evt.xrel, evt.yrel, 0) * 0.0005 + location->getPosition());
			}
			return true;
		}

		CameraControls() {
			_r = root;
			rightGrabbed = false;

			camera = mainScene->createCamera("camera");
			viewport = window->addViewport(camera);
			viewport->setDimensions(0, 0, 1, 1);
			viewport->setBackgroundColour(Ogre::ColourValue::Red);

			camera->setAutoAspectRatio(true);
			camera->setNearClipDistance(.5);
			camera->setFarClipDistance(10);

			location = mainScene->getRootSceneNode()->createChildSceneNode("camera location");
			location->attachObject(camera);
			location->setPosition(0, -.5, .5);

			location->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TransformSpace::TS_WORLD);
		}

		virtual ~CameraControls() {
			mainScene->getRootSceneNode()->removeAndDestroyChild(location);
			mainScene->destroyCamera(camera);
		}
	};

	class MapRenderer {
	private:
		::Map* theMap;
	public:
		typedef std::shared_ptr<MapRenderer> Ptr;

		MapRenderer(::Map* _map) : theMap(_map) {
			std::cout << "init shader man " << Ogre::RTShader::ShaderGenerator::initialize() << std::endl;
			auto rtshare = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
			rtshare->addSceneManager(mainScene);

			auto resgrpman = Ogre::ResourceGroupManager::getSingletonPtr();

			resgrpman->createResourceGroup("map", false);
			resgrpman->addResourceLocation("ogre_resources/", "FileSystem", "map");
			resgrpman->addResourceLocation("ogre_resources/textures/", "FileSystem", "map");
			resgrpman->initialiseAllResourceGroups();

			auto m = root->getMeshManager()->createPlane("hex", "map", Ogre::Plane(0, 0, 1, 0), .1, .1);
			auto e = mainScene->createEntity(m);
			auto l = mainScene->getRootSceneNode()->createChildSceneNode();
			l->attachObject(e);

			auto matman = Ogre::MaterialManager::getSingletonPtr();
			auto mat = matman->getByName("base_hex", "map");

			resgrpman->loadResourceGroup("map");

			e->setMaterial(mat);

//			auto mat = Ogre::MaterialManager::getSingleton().create("hex", "map");
//			mat->setCullingMode(Ogre::CullingMode::CULL_NONE);
		}

		virtual ~MapRenderer() {

		}
	};

	CameraControls::Ptr cameraControls;
	MapRenderer::Ptr mapRenderer;
}

extern "C" {
	static GtkWindow* gtk_window;

	static gdouble _mm_relX_origin = -1;
	static gdouble _mm_relY_origin = -1;
	static gint64 animationTimerValue;

	static GdkFilterReturn _ogreb_xevent_filter(GdkXEvent *_xevent,
            GdkEvent *event,
            gpointer data) {
		XEvent* xe = (XEvent*) _xevent;

		bool emitMotion = false;
		bool emitButtonPressEvent = false;
		int mouseX, mouseY;
		bool pressed;
		int xbutton;

		if (xe->type == EnterNotify) {
			emitMotion = true;
			mouseX = xe->xcrossing.x;
			mouseY = xe->xcrossing.y;
			_mm_relX_origin = mouseX;
			_mm_relY_origin = mouseY;
		} else if (xe->type == MotionNotify) {
			emitMotion = true;
			mouseX = xe->xmotion.x;
			mouseY = xe->xmotion.y;
		} else if (xe->type == LeaveNotify) {
			_mm_relX_origin = -1;
			_mm_relY_origin = -1;
		} else if ((xe->type == ButtonPress) || (xe->type == ButtonRelease)) {
			emitButtonPressEvent = true;
			pressed = xe->type == ButtonPress;
			xbutton = xe->xbutton.button;
			mouseX = xe->xbutton.x;
			mouseY = xe->xbutton.y;
		}

		if (emitMotion) {
			OgreBites::MouseMotionEvent mme;
			mme.windowID = 0;
			mme.type = OgreBites::MOUSEMOTION;
			mme.x = mouseX;
			mme.y = mouseY;
			mme.xrel = _mm_relX_origin >= 0 ? mme.x - _mm_relX_origin : 0;
			mme.yrel = _mm_relY_origin >= 0 ? mme.y - _mm_relY_origin : 0;

			_mm_relX_origin = mouseX;
			_mm_relY_origin = mouseY;

			if (pogre::cameraControls) {
				pogre::cameraControls->mouseMoved(mme);
			}
		}
		if (emitButtonPressEvent) {
			OgreBites::MouseButtonEvent mbe;
			mbe.type = pressed ? OgreBites::MOUSEBUTTONDOWN : OgreBites::MOUSEBUTTONUP;
			mbe.clicks = 1;
			mbe.x = mouseX;
			mbe.y = mouseY;
			switch (xbutton) {
			case Button1: mbe.button = OgreBites::BUTTON_LEFT; break;
			case Button2: mbe.button = OgreBites::BUTTON_MIDDLE; break;
			case Button3: mbe.button = OgreBites::BUTTON_RIGHT; break;
			default: ;
			}

			if (pogre::cameraControls) {
				if (pressed) {
					pogre::cameraControls->mousePressed(mbe);
				} else {
					pogre::cameraControls->mouseReleased(mbe);
				}
			}
		}

		return GDK_FILTER_CONTINUE;
	}

	static void _ogreb_init_ogre() {
		using namespace pogre;

		// Ogre
		root = OgreRootPtr(new Ogre::Root());

		if (!pogre::root->restoreConfig()) {
			pogre::root->showConfigDialog(OgreBites::getNativeConfigDialog());
		}
		root->initialise(false);
		Ogre::RenderWindowDescription rwDesc;
		rwDesc.width = 600;
		rwDesc.height = 400;
		rwDesc.useFullScreen = false;
		rwDesc.name = "default";

		GdkWindow* gdkwin = gtk_widget_get_window((GtkWidget*) gtk_window);
		Display* disp = gdk_x11_display_get_xdisplay(gdk_window_get_display(gdkwin));
		guint32 screen = gdk_x11_screen_get_current_desktop(gdk_window_get_screen(gdkwin));
		Window xid = GDK_WINDOW_XID(gdkwin);

		char windowDesc[128];
		windowDesc[128 - 1] = 0;
		snprintf(windowDesc, 128 - 1, "%llu:%u:%lu", (unsigned long long) disp, screen, xid);
		std::cout << "Window id: " << windowDesc << std::endl;

		rwDesc.miscParams["externalWindowHandle"] = windowDesc;

		window = root->createRenderWindow(rwDesc);


		mainScene = root->createSceneManager(
				Ogre::DefaultSceneManagerFactory::FACTORY_TYPE_NAME,
				"Main Scene");

		mainScene->setAmbientLight(Ogre::ColourValue(.5, .5, .5));

		cameraControls = CameraControls::Ptr(new pogre::CameraControls());

		mapRenderer = MapRenderer::Ptr(new pogre::MapRenderer(nullptr));
	}

	static gboolean _ogreb_render_handler() {
		using namespace pogre;

		gint64 now = g_get_real_time();
		if (!root) {
			_ogreb_init_ogre();
			animationTimerValue = now;
		}

		const Ogre::Real seconds = (now - animationTimerValue) / 1000000.0;
		animationTimerValue = now;
		if (pogre::root) pogre::root->renderOneFrame(seconds);

		return TRUE;
	}

	void ogreb_init() {
		// Gtk bits
		GtkWidget* winwid = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window = reinterpret_cast<GtkWindow*>(winwid);
		gtk_window_set_title(gtk_window, "Pioneers 3D Game Window");
		gtk_window_set_default_size(gtk_window, 600, 400);

		gtk_widget_show_all(winwid);

		gdk_window_add_filter(
				gtk_widget_get_window((GtkWidget*) gtk_window),
				_ogreb_xevent_filter,
				NULL);
	}

	static gboolean	animate(gpointer user_data) {
		_ogreb_render_handler();
		return G_SOURCE_CONTINUE;
	}

	VoidFunction pogre_setup_gtk_mainloop(VoidFunction old) {
		g_timeout_add(10, &animate, NULL);
		return old;
	}

	void ogreb_show_map(Map* map) {
		using namespace pogre;
		//mapRenderer = MapRenderer::Ptr(new MapRenderer(map));
	}

	void ogreb_cleanup() {
		pogre::cameraControls.reset();
		pogre::root.reset();

		SDL_Quit();
	}
}
