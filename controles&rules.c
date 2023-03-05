#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL_ttf.h>
#include "controles&rules.h"
#include "debugmalloc.h"

/*Mindig a menü megjelenítése után hívódik meg, feladata, hogy a megfelelő menüpontot nyissa meg, ezt koordináta geometriai számolásokkal éri el.*/
Menustate menuinput(bool *quitprog){
    SDL_Event event;
    while(event.type != SDL_QUIT){
        SDL_WaitEvent(&event);
            if(event.type == SDL_MOUSEBUTTONDOWN){
                int x, y;
                SDL_GetMouseState(&x, &y);
                if(sqrt(pow(x-640, 2)+pow(y-180, 2))<80)
                    return start;
                if(sqrt(pow(x-640, 2)+pow(y-360, 2))<80)
                    return leaderboards;
                if(sqrt(pow(x-640, 2)+pow(y-540, 2))<80)
                    return quit;
            }
    }
    *quitprog=true;
    return start;
}
/*Ez a függvény számolja ki, hogy tick-enként az űrhajónak pontosan hol kell elhelyezkednie a képernyőn, és a pointerként kapott SDL_Rect struktúrát változtatja meg.*/
void shipcontrol(bool buttonup,bool buttonleft,bool buttonright, SDL_Rect *dest, double moonvel, double engvelx, double engvely, double *xspeed, double *yspeed){
    /*balra a negatív irány*/
    if(buttonleft == true && buttonup == true){
        *xspeed-=engvelx;
        *yspeed-=(engvely-moonvel);
    }
    if(buttonright == true && buttonup == true){
            *xspeed+=engvelx;
            *yspeed-=(engvely-moonvel);
    }
    if(buttonleft == true){
        *xspeed-=engvelx;
        *yspeed+=moonvel;
    }
    if(buttonright == true){
        *xspeed+=engvelx;
        *yspeed+=moonvel;
    }
    if(buttonup == true){
        *yspeed-=(engvely-moonvel);
    }
    if(!buttonleft && !buttonright && !buttonup){
        *yspeed+=moonvel;
    }
    dest->y+=*yspeed;
    dest->x+=*xspeed;
}
/*Amennyiben a replay funkciót használják, ez a függvény fogadja az inputot a billentyűzetről.
Kilépés esetén false-al, replay esetén true-val tér vissza.*/
bool replayinput(bool *quitprog){
    SDL_Event event;
    while(event.type != SDL_QUIT){
        SDL_WaitEvent(&event);
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_b)
                return false;
            if(event.key.keysym.sym == SDLK_r)
                return true;
        }
    }
    *quitprog = true;
    return false;
}
/*Szöveg begépelését teszi lehetővé, a bemenetén szereplő stringbe írja a bemeneti szöveget.*/
bool input_text(char *dest, size_t length, SDL_Rect rect, SDL_Color background, SDL_Color text, TTF_Font *font, SDL_Renderer *render){
    char composition[SDL_TEXTEDITINGEVENT_TEXT_SIZE];
    composition[0] = '\0';
    char textandcomposition[length + SDL_TEXTEDITINGEVENT_TEXT_SIZE + 1];
    int maxw = rect.w - 2;
    int maxh = rect.h - 2;

    dest[0] = '\0';

    bool enter = false;
    bool close = false;

    SDL_StartTextInput();
    while(!close && !enter){
        boxRGBA(render, rect.x, rect.y, rect.x + rect.w - 1, rect.y + rect.h -1, background.r, background.g, background.b, 255);
        rectangleRGBA(render, rect.x, rect.y, rect.x + rect.w - 1, rect.y + rect.h -1, text.r, text.g, text.b, 255);
        int w;
        strcpy(textandcomposition, dest);
        strcat(textandcomposition, composition);
        if(textandcomposition[0] != '\0'){
            SDL_Surface *sub = TTF_RenderUTF8_Blended(font, textandcomposition, text);
            SDL_Texture *sub_t = SDL_CreateTextureFromSurface(render, sub);
            SDL_Rect aim = {rect.x, rect.y, sub->w < maxw ? sub->w : maxw, sub->h < maxh ? sub->h : maxh};
            SDL_RenderCopy(render, sub_t, NULL, &aim);
            SDL_FreeSurface(sub);
            SDL_DestroyTexture(sub_t);
            w = aim.w;
        }
        else{
            w = 0;
        }
        if(w < maxw){
            vlineRGBA(render, rect.x + w + 2, rect.y + 2, rect.y + rect.h - 3, text.r, text.g, text.b, 192);
            SDL_RenderPresent(render);

            SDL_Event event;
            SDL_WaitEvent(&event);
            switch(event.type){
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_BACKSPACE){
                        int textlen = strlen(dest);
                        do{
                            if(textlen == 0){
                                break;
                            }
                            if((dest[textlen-1] & 0x80) == 0x00){
                                dest[textlen-1] = 0x00;
                                break;
                            }
                            if((dest[textlen-1] & 0xC0) == 0x80){
                                dest[textlen-1] = 0x00;
                                textlen--;
                            }
                            if((dest[textlen-1] & 0xC0) == 0xC0){
                                dest[textlen-1] = 0x00;
                                break;
                            }
                        } while(true);
                    }
                    if(event.key.keysym.sym == SDLK_RETURN){
                        enter = true;
                        break;
                    }
                    break;
                case SDL_TEXTINPUT:
                    if(strlen(dest) + strlen(event.text.text) < length){
                        strcat(dest, event.text.text);
                    }
                    composition[0] ='\0';
                    break;
                case SDL_TEXTEDITING:
                    strcpy(composition, event.edit.text);
                    break;
                case SDL_QUIT:
                    SDL_PushEvent(&event);
                    close = true;
                    break;
            }
        }
    }
    SDL_StopTextInput();
    return enter;
}
/*Ellenrőzi, hogy a játékos nyert, vagy vesztett, ennek megfelelően változtatja meg
a győzelemre vagy a avereségre utaló logikai értékeket.*/
void rules(bool *lost, bool *won, SDL_Rect dest, int width, int height, int fuel, int xspeed, int yspeed){
    if(dest.x <= 0)
        *lost = true;
    if(dest.y <= 0)
        *lost = true;
    if((dest.x + 36) >= width)
        *lost = true;
    if(fuel == 0)
        *lost = true;
    if(dest.x <= 600 && dest.y+104 >= 500)
        *lost = true;
    if(dest.x+36 >= 660 && dest.y+104 >= 450)
        *lost = true;
    if(dest.x >= 600 && dest.x+36 <= 660 && dest.y+104 >= 620 && xspeed < 1 && yspeed < 2)
        *won = true;
     if(dest.x >= 600 && dest.x+36 <= 660 && dest.y+104 >= 620 && (xspeed > 1 || yspeed > 2))
        *lost = true;
    return;
}
/*A dicsőséglista megtekintése után ez a függvény várja az inputot (amivel visszatérít a menübe)*/
bool leaderboardsevent(bool *quitprog){
    SDL_Event event;
    while(event.type != SDL_QUIT){
        SDL_WaitEvent(&event);
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_ESCAPE)
                return true;
        }
    }
    *quitprog = true;
    return true;
}
