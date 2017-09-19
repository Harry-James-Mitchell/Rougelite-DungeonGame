#ifndef IO_H
# define IO_H

#include "pc.h"

typedef struct dungeon dungeon_t;

void io_init_terminal(void);
void io_reset_terminal(void);
void io_display(dungeon_t *d);
void io_handle_input(dungeon_t *d);
void io_queue_message(const char *format, ...);
void io_calculate_offset(dungeon_t *d);
void io_update_offset(dungeon_t *d);
void display_shop(dungeon_t *d);

#endif
