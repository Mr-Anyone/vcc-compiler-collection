#include "SDL3/SDL.h"
#include <stdio.h>
#include <stdlib.h>

void *sdl_alloc_event() { return malloc(sizeof(SDL_Event)); }

void sdl_free_event(void *event) { return free(event); }

void *sdl_get_keyboard_event(void *sdl_event) {
  SDL_Event *event = (SDL_Event *)sdl_event;
  return &event->key;
}

bool sdl_is_event(void* event, int kind){
    bool return_type =
     ((SDL_Event*)event)->type == kind;
    return return_type;
} 
