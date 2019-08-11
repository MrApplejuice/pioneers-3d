#include "main.h"

#include "engine.h"

#include <iostream>
#include <vector>
#include <typeinfo>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

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
		bool emitScrollEvent = false;
		int mouseX, mouseY;
		bool pressed;
		int xbutton;
		int scrollTicks;

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
			if ((xe->xbutton.button == Button4) || (xe->xbutton.button == Button5)) {
				emitScrollEvent = true;
				scrollTicks = xe->xbutton.button == Button4 ? -1 : 1;
			} else {
				emitButtonPressEvent = true;
				pressed = xe->type == ButtonPress;
				xbutton = xe->xbutton.button;
				mouseX = xe->xbutton.x;
				mouseY = xe->xbutton.y;
			}
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

			if (pogre::mainEngine) {
				pogre::mainEngine->mouseMoved(mme);
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

			if (pogre::mainEngine) {
				if (pressed) {
					pogre::mainEngine->mousePressed(mbe);
				} else {
					pogre::mainEngine->mouseReleased(mbe);
				}
			}
		}
		if (emitScrollEvent) {
			OgreBites::MouseWheelEvent mwe;
			mwe.type = OgreBites::MOUSEWHEEL;
			mwe.y = scrollTicks;

			if (pogre::mainEngine) {
				pogre::mainEngine->mouseWheelRolled(mwe);
			}
		}

		return GDK_FILTER_CONTINUE;
	}

	static void _ogreb_init_ogre() {
		using namespace pogre;

		GdkWindow* gdkwin = gtk_widget_get_window((GtkWidget*) gtk_window);
		Display* disp = gdk_x11_display_get_xdisplay(gdk_window_get_display(gdkwin));
		guint32 screen = gdk_x11_screen_get_current_desktop(gdk_window_get_screen(gdkwin));
		Window xid = GDK_WINDOW_XID(gdkwin);

		char windowDesc[128];
		windowDesc[128 - 1] = 0;
		snprintf(windowDesc, 128 - 1, "%llu:%u:%lu", (unsigned long long) disp, screen, xid);
		std::cout << "Window id: " << windowDesc << std::endl;

		new Engine(windowDesc);
	}

	static gboolean _ogreb_render_handler() {
		using namespace pogre;

		gint64 now = g_get_real_time();
		if (!pogre::mainEngine) {
			_ogreb_init_ogre();
			animationTimerValue = now;
		}

		const Ogre::Real seconds = (now - animationTimerValue) / 1000000.0;
		animationTimerValue = now;
		if (pogre::mainEngine) pogre::mainEngine->render(seconds);

		return TRUE;
	}

	static void	_ogreb_resize(GtkWidget* widget,
			GdkRectangle* allocation,
			gpointer user_data) {
		if (pogre::mainEngine) {
			pogre::mainEngine->updateWindowSize(allocation->width, allocation->height);
		}
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

		g_signal_connect(winwid, "size-allocate", G_CALLBACK(_ogreb_resize), NULL);
	}

	static gboolean	animate(gpointer user_data) {
		_ogreb_render_handler();
		return G_SOURCE_CONTINUE;
	}

	VoidFunction pogre_setup_gtk_mainloop(VoidFunction old) {
		g_timeout_add(20, &animate, NULL);
		return old;
	}

	void ogreb_start_game() {
		if (pogre::mainEngine) pogre::mainEngine->startNewGame();
	}

	void ogreb_show_map(Map* map) {
		if (pogre::mainEngine) pogre::mainEngine->loadNewMap(map);
	}

	void ogreb_cleanup() {
		using namespace pogre;
		if (mainEngine) delete mainEngine;
	}
}
