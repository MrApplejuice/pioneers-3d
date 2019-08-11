cmake_minimum_required(VERSION 3.12)

project(pioneers-3d)

find_package(SDL2 REQUIRED)

find_package(GTK3 REQUIRED
    COMPONENTS gtk)
if(NOT GTK3_FOUND)
	message(FATAL_ERROR "GTK3 not found")
endif()

find_package(OGRE REQUIRED)


find_library(
	LIBGIO_2_0_LIBRARY
	NAMES gio-2.0
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)

find_library(
	LIBPANGO_LIBRARY
	NAMES pango-1.0
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)

find_library(
	LIBPANGOCAIRO_LIBRARY
	NAMES pangocairo-1.0
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)

find_library(
	LIBAVAHI_COMMON_LIBRARY
	NAMES avahi-common
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)
find_library(
	LIBAVAHI_CLIENT_LIBRARY
	NAMES avahi-client
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)
find_library(
	LIBAVAHI_GLIB_LIBRARY
	NAMES avahi-glib
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)

find_library(
	NOTIFY_LIBRARY
	NAMES notify
	PATHS 
		/usr/lib/
		/usr/lib/x86_64-linux-gnu/)

configure_file(
	common/gtk/config-gnome.h
	include/gnome.h)
configure_file(
	common/gtk/config-gnome.c
	include/gnome.c)


set(P3D_OGRE_SRC
	client/gtk/ogre/player.h
	client/gtk/ogre/player.cpp
	client/gtk/ogre/map_renderer.h
	client/gtk/ogre/map_renderer.cpp
	client/gtk/ogre/engine_base.h
	client/gtk/ogre/engine_base.cpp
	client/gtk/ogre/engine.h
	client/gtk/ogre/engine.cpp
	client/gtk/ogre/main.h
	client/gtk/ogre/main.cpp
)

set(P3D_COMMON_SRC
	common/cost.c
	common/game-list.c
	common/game.c
	common/state.c
	common/random.c
	common/quoteinfo.c
	common/network.c
	common/driver.c
	common/set.c
	common/buildrec.c
	common/map_query.c
	common/notifying-string.c
	common/deck.c
	
	common/gtk/common_gtk.c
	common/gtk/scrollable-text-view.c
	common/gtk/aboutbox.c
	common/gtk/game-settings.c
	common/gtk/theme.c
	common/gtk/map-icons.c
	common/gtk/metaserver.c
	common/gtk/game-rules.c
	common/gtk/polygon.c
	common/gtk/guimap.c
	common/gtk/select-game.c
	common/gtk/player-icon.c
	common/gtk/colors.c
	
	common/log.c
	common/cards.c
	common/map.c
	common/common_glib.c

	client/common/build.c
	client/common/callback.c
	client/common/client.c
	client/common/develop.c
	client/common/main.c
	client/common/player.c
	client/common/resource.c
	client/common/robber.c
	client/common/setup.c
	client/common/stock.c
	client/common/turn.c
	
	client/common/client.h
	client/callback.h

	common/quoteinfo.h
	common/game-list.h
	common/cost.h
	common/random.h
	common/notifying-string.h
	common/network.h
	common/cards.h
	common/driver.h
	common/state.h
	common/notifying-string-private.h
	common/gtk/polygon.h
	common/gtk/common_gtk.h
	common/gtk/game-rules.h
	common/gtk/select-game.h
	common/gtk/player-icon.h
	common/gtk/guimap.h
	common/gtk/scrollable-text-view.h
	common/gtk/game-settings.h
	common/gtk/aboutbox.h
	common/gtk/theme.h
	common/gtk/map-icons.h
	common/gtk/metaserver.h
	common/gtk/colors.h
	common/set.h
	common/common_glib.h
	common/log.h
	common/authors.h
	common/game.h
	common/buildrec.h
	common/deck.h
	common/version.h
	common/map.h
)


set(P3D_GTK_SRC
	${CMAKE_CURRENT_BINARY_DIR}/include/gnome.h
	${CMAKE_CURRENT_BINARY_DIR}/include/gnome.c

	client/gtk/connect.c
	client/gtk/settingscreen.c
	client/gtk/gui.c
	client/gtk/quote-view.c
	client/gtk/develop.c
	client/gtk/player.c
	client/gtk/discard.c
	client/gtk/state.c
	client/gtk/resource-table.c
	client/gtk/identity.c
	client/gtk/histogram.c
	client/gtk/monopoly.c
	client/gtk/legend.c
	client/gtk/frontend.c
	client/gtk/notification.c
	client/gtk/quote.c
	client/gtk/name.c
	client/gtk/trade.c
	client/gtk/interface.c
	client/gtk/gameover.c
	client/gtk/avahi.c
	client/gtk/offline.c
	client/gtk/chat.c
	client/gtk/audio.c
	client/gtk/avahi-browser.c
	client/gtk/gold.c
	client/gtk/resource.c
	client/gtk/resource-view.c
	client/gtk/plenty.c
	client/gtk/callbacks.c
	client/gtk/resource-view.h
	client/gtk/notification.h
	client/gtk/audio.h
	client/gtk/gui.h
	client/gtk/resource-table.h
	client/gtk/avahi.h
	client/gtk/histogram.h
	client/gtk/resource-view-private.h
	client/gtk/avahi-browser.h
	client/gtk/quote-view.h
	client/gtk/frontend.h
)

add_executable(pioneers-3d
	${P3D_COMMON_SRC}
	${P3D_GTK_SRC}
	${P3D_OGRE_SRC})

set_property(TARGET pioneers-3d PROPERTY CXX_STANDARD 17)

target_include_directories(pioneers-3d
	PUBLIC 
		${GTK3_INCLUDE_DIRS}
		${SDL2_INCLUDE_DIRS}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/client
		${CMAKE_CURRENT_SOURCE_DIR}/common
		${CMAKE_CURRENT_SOURCE_DIR}/common/gtk
		${CMAKE_CURRENT_SOURCE_DIR}/client/common
		${CMAKE_CURRENT_BINARY_DIR}/include)

target_compile_definitions(
	pioneers-3d
	PUBLIC
		"DATADIR=\"${CMAKE_INSTALL_PREFIX}/share\""
		"THEMEDIR=\"${CMAKE_INSTALL_PREFIX}/share/games/pioneers/themes\""
		"LOCALEDIR=\"${CMAKE_INSTALL_PREFIX}/share/locale\""
		"PIONEERS_DIR_DEFAULT=\"${CMAKE_INSTALL_PREFIX}/share/games/pioneers\""
		"PIONEERS_SERVER_CONSOLE_PROGRAM_NAME=\"pioneers-server-console\""
		"PIONEERS_SERVER_GTK_PROGRAM_NAME=\"pioneers-server-gtk\""
		"PIONEERS_CLIENT_GTK_PROGRAM_NAME=\"pioneers\""
		"PIONEERS_AI_PROGRAM_NAME=\"pioneersai\"")

target_link_libraries(pioneers-3d 
	${GTK3_LIBRARIES}
	${LIBGIO_2_0_LIBRARY}
	${LIBPANGO_LIBRARY}
	${LIBPANGOCAIRO_LIBRARY}
	${LIBAVAHI_CLIENT_LIBRARY}
	${LIBAVAHI_COMMON_LIBRARY}
	${LIBAVAHI_GLIB_LIBRARY}
	${NOTIFY_LIBRARY}
	${SDL2_LIBRARIES}
	m
	OgreMain
	OgreBites
	OgreRTShaderSystem)

