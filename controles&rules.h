#ifndef CONTROLESRULES_H_INCLUDED
#define CONTROLESRULES_H_INCLUDED

typedef enum {start, leaderboards, quit} Menustate;

Menustate menuinput(bool *quitprog);

void shipcontrol(bool buttonup,bool buttonleft,bool buttonright, SDL_Rect *dest, double moonvel, double engvelx, double engvely, double *xspeed, double *yspeed);

bool replayinput(bool *quitprog);

bool input_text(char *dest, size_t hossz, SDL_Rect rect, SDL_Color background, SDL_Color text, TTF_Font *font, SDL_Renderer *render);

void rules(bool *lost, bool *won, SDL_Rect dest, int width, int height, int fuel, int xspeed, int yspeed);

bool leaderboardsevent(bool *quitprog);

#endif // CONTROLES@RULES_H_INCLUDED
