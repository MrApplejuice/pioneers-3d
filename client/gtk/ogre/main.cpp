#include "main.h"

#include "engine.h"

#include <iostream>
#include <vector>
#include <typeinfo>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

extern "C" {
	static GtkWindow* gtk_window;
	static Window gtk_window_xid;

	static void _ogreb_init_ogre() {
		using namespace pogre;

		GdkWindow* gdkwin = gtk_widget_get_window((GtkWidget*) gtk_window);
		Display* disp = gdk_x11_display_get_xdisplay(gdk_window_get_display(gdkwin));
		guint32 screen = gdk_x11_screen_get_current_desktop(gdk_window_get_screen(gdkwin));

		char windowDesc[128];
		windowDesc[128 - 1] = 0;
		snprintf(windowDesc, 128 - 1, "%llu:%u:%lu", (unsigned long long) disp, screen, gtk_window_xid);

		std::cout << "Window id: " << windowDesc << std::endl;
		new Engine(windowDesc);
	}

	static gint64 animationTimerValue;
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

	static gboolean _ogreb_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
		std::cout << "KEY PRESSSS" << std::endl;
		return false;
	}

	static gdouble _mm_relX_origin = -1;
	static gdouble _mm_relY_origin = -1;

	static void _ogreb_emit_motion_event(gdouble mouseX, gdouble mouseY) {
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

	static void _ogreb_emit_button_press_event(int buttonIndex, bool pressed, gdouble mouseX, gdouble mouseY) {
		OgreBites::MouseButtonEvent mbe;
		mbe.type = pressed ? OgreBites::MOUSEBUTTONDOWN : OgreBites::MOUSEBUTTONUP;
		mbe.clicks = 1;
		mbe.x = mouseX;
		mbe.y = mouseY;
		switch (buttonIndex) {
		case 1: mbe.button = OgreBites::BUTTON_LEFT; break;
		case 2: mbe.button = OgreBites::BUTTON_MIDDLE; break;
		case 3: mbe.button = OgreBites::BUTTON_RIGHT; break;
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

	static gboolean _ogreb_on_mouse_entered(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
		_mm_relX_origin = event->x;
		_mm_relY_origin = event->y;
		_ogreb_emit_motion_event(event->x, event->y);
		return false;
	}

	static gboolean _ogreb_on_mouse_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
		_ogreb_emit_motion_event(event->x, event->y);
		return false;
	}

	static gboolean _ogreb_on_mouse_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
		_mm_relX_origin = -1;
		_mm_relY_origin = -1;
		return false;
	}

	static gboolean _ogreb_on_mouse_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
		_ogreb_emit_button_press_event(event->button, true, event->x, event->y);
		return false;
	}

	static gboolean _ogreb_on_mouse_button_released(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
		_ogreb_emit_button_press_event(event->button, false, event->x, event->y);
		return false;
	}

	static gboolean _ogreb_on_mouse_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
		OgreBites::MouseWheelEvent mwe;

		mwe.type = OgreBites::MOUSEWHEEL;
		mwe.y = ((event->direction == GDK_SCROLL_UP ? 1 : 0) + (event->direction == GDK_SCROLL_DOWN ? -1 : 0));

		if (pogre::mainEngine) {
			pogre::mainEngine->mouseWheelRolled(mwe);
		}

		return false;
	}

	void ogreb_init() {
		// Gtk bits
		GtkWidget* winwid = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window = reinterpret_cast<GtkWindow*>(winwid);
		gtk_window_set_title(gtk_window, "Pioneers 3D Game Window");
		gtk_window_set_default_size(gtk_window, 600, 400);

		gtk_widget_show_all(winwid);

		auto gdkwin = gtk_widget_get_window(winwid);
		gtk_window_xid = GDK_WINDOW_XID(gdkwin);

		gtk_widget_add_events(winwid, GDK_BUTTON_MOTION_MASK);
		gtk_widget_add_events(winwid, GDK_KEY_PRESS_MASK);
		gtk_widget_add_events(winwid, GDK_POINTER_MOTION_MASK);
		gtk_widget_add_events(winwid, GDK_ENTER_NOTIFY_MASK);
		gtk_widget_add_events(winwid, GDK_LEAVE_NOTIFY_MASK);
		gtk_widget_add_events(winwid, GDK_SCROLL_MASK);

		g_signal_connect(winwid, "size-allocate", G_CALLBACK(_ogreb_resize), NULL);
		g_signal_connect(G_OBJECT(winwid), "key-press-event", G_CALLBACK(_ogreb_on_key_press), NULL);
		g_signal_connect(G_OBJECT(winwid), "enter-notify-event", G_CALLBACK(_ogreb_on_mouse_entered), NULL);
		g_signal_connect(G_OBJECT(winwid), "motion-notify-event", G_CALLBACK(_ogreb_on_mouse_motion), NULL);
		g_signal_connect(G_OBJECT(winwid), "leave-notify-event", G_CALLBACK(_ogreb_on_mouse_leave), NULL);
		g_signal_connect(G_OBJECT(winwid), "button-press-event", G_CALLBACK(_ogreb_on_mouse_button_pressed), NULL);
		g_signal_connect(G_OBJECT(winwid), "button-release-event", G_CALLBACK(_ogreb_on_mouse_button_released), NULL);
		g_signal_connect(G_OBJECT(winwid), "scroll-event", G_CALLBACK(_ogreb_on_mouse_scroll), NULL);

		std::cout << "Registered it all!" << std::endl;
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
