#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <client.h>

typedef void(*VoidFunction)(void);

void ogreb_init();
VoidFunction pogre_setup_gtk_mainloop(VoidFunction old);
void ogreb_start_game();
void ogreb_show_map(Map* map);
void ogreb_cleanup();

#ifdef __cplusplus
}
#endif
