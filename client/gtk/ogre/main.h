#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <client.h>

typedef void(*VoidFunction)(void);

void ogreb_init();
VoidFunction pogre_setup_gtk_mainloop(VoidFunction old);
void ogreb_cleanup();

// Game implementation
void ogreb_start_game();
void ogreb_show_map(Map* map);
void ogreb_map_node_updates(Node* node);
void ogreb_map_edge_updates(Edge* edge);

#ifdef __cplusplus
}
#endif
