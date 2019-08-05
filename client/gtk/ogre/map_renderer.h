#pragma once

#ifndef CLIENT_GTK_OGRE_MAP_RENDERER_H_
#define CLIENT_GTK_OGRE_MAP_RENDERER_H_

#include <memory>

#include <map.h>

namespace pogre {
	class MapRenderer {
	private:
		::Map* theMap;
	public:
		typedef std::shared_ptr<MapRenderer> Ptr;

		MapRenderer(::Map* _map);
		virtual ~MapRenderer();
	};
}

#endif /* CLIENT_GTK_OGRE_MAP_RENDERER_H_ */
