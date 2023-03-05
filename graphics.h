#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

void openwindow(int width, int height, SDL_Window **pwindow, SDL_Renderer **prender, TTF_Font **pfont);

void loadpicture(SDL_Renderer *render, SDL_Texture **ptexture, SDL_Rect dest, SDL_Surface **psurface, char *str);

void openmenu(SDL_Renderer *render, int width, int height, TTF_Font *font);

void text(SDL_Renderer *render, TTF_Font *font, char *str, SDL_Rect textrect);

void presentpicture(SDL_Renderer *render, SDL_Texture *texture, SDL_Rect dest);

void presentreplay(SDL_Renderer *render, TTF_Font *font);

void rendermap(SDL_Renderer *render);

#endif // GRAPHICS_H_INCLUDED

