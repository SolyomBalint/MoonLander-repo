#include <SDL.h>
#include <SDL_image.h>
#include <SDL2_gfxPrimitives.h>
#include <math.h>
#include <stdlib.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include "debugmalloc.h"

/*Megnyit egy teljesen üres adott méretû fekete ablakot, lépésenként ellenőrzi, hogy
ne legyen betöltési hiba, amennyiben van kilép. Az itt lefoglalt területek felszabadítása a hívó kötelessége.*/
void openwindow(int width, int height, SDL_Window **pwindow, SDL_Renderer **prender, TTF_Font **pfont){
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
        SDL_Log("nem indíthato: %s", SDL_GetError());
        exit(1);
    }
    SDL_Window *window = SDL_CreateWindow("Moon Lander map_test", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, width, height, 0);
        if(window==NULL)
            exit(1);
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if(render==NULL)
            exit(1);
    TTF_Init();
    TTF_Font *font = TTF_OpenFont("LiberationSerif-Regular.ttf", 28);
    if(font == NULL){
        SDL_Log("Nem nyert! %s\n", TTF_GetError());
        exit(1);
    }
    SDL_RenderClear(render);
    *pfont = font;
    *pwindow = window;
    *prender = render;
}

/*Képet tölt be, létrehoz egy surface-t amire betölti a játék mappájában található képet
a neve alapján. Ezután a texturere tölti, így a megjelenítés lehetésgessé válik. Ezután
nincsen szükség a surfacere, így azt felszabdítjuk. Végül pedig letisztítja a teljes képeternyőt
és kirajzolja a korábban betölött képet. A texture felszabadítása a hívó feladata. */
void loadpicture(SDL_Renderer *render, SDL_Texture **ptexture, SDL_Rect dest, SDL_Surface **psurface, char *str){
    SDL_Surface *surface = IMG_Load(str);
    if(!surface){
        SDL_DestroyRenderer(render);
        SDL_Quit();
    }
    *psurface=surface;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    SDL_FreeSurface(surface);
    if(!texture){
        SDL_DestroyRenderer(render);
        SDL_Quit();
    }
    *ptexture=texture;
    SDL_RenderCopy(render, texture, NULL, &dest);
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
}

/*Csak a graphics.c számára látható, feliratokat elkészítő függvény. Inputja igényli
a openwindow függvéyn által definiált font és render pointerekt, egy sztringet(ezt lesz kiírva)
és egy SDL_Rect formátumú változót, amiben benne van, hogy a felirat hol helyezkedejn el.
A függvényben nincsen SDL_RenderPresent, a létrehozott Surface felszabadításáról gondoskodik.*/
void text(SDL_Renderer *render, TTF_Font *font, char *str, SDL_Rect textrect){
    SDL_Color feher = {255, 255, 255};
    SDL_Surface *textsurface = TTF_RenderUTF8_Solid(font, str, feher);
    SDL_Texture *texttexture = SDL_CreateTextureFromSurface(render, textsurface);
    textrect.w = textsurface->w;
    textrect.h = textsurface->h;
    SDL_RenderCopy(render, texttexture, NULL, &textrect);
    SDL_FreeSurface(textsurface);
    SDL_DestroyTexture(texttexture);
}
/* Megnyitja  a menüt, semminek nem változtatja meg a értéket a függvényen kívűl. Rendelkezik
SDL_RenderPresenttel.*/
void openmenu(SDL_Renderer *render, int width, int height, TTF_Font *font){
    SDL_RenderClear(render);
    int x=width/2, y=height/4, r=80;
    circleRGBA(render, x, y, r, 255, 255, 255, 255);
    SDL_Rect textplace = {600, 160, 0, 0};
    text(render, font, "START", textplace);
    y=(height/4)*2;
    circleRGBA(render, x, y, r, 255, 255, 255, 255);
    textplace.y = 340;
    textplace.x = 570;
    text(render, font, "Leaderboards", textplace);
    y=(height/4)*3;
    circleRGBA(render, x, y, r, 255, 255, 255, 255);
    textplace.x = 610;
    textplace.y = 520;
    text(render, font, "EXIT", textplace);
    SDL_SetRenderDrawColor(render, 0, 0, 0, 1);
    SDL_RenderPresent(render);
}
/*Ez a függvény kirajzolja a képernyőre a már betöltött képet (de nem renderel!). Előnye a loadpictureel szemben, hogy nem tölt be minden egyes hívásnál
egy képet, hanem a már betölött képet kirajzolja, így gyorsabb. De a képen nem tud változtatni.*/
void presentpicture(SDL_Renderer *render, SDL_Texture *texture, SDL_Rect dest){
    SDL_RenderCopy(render, texture, NULL, &dest);
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);
}
/*Amennyiben a replay funkció meghívódik ez a függvény jeleníti meg a lehetséges választási
opciókat a felhasználó számára. Bemenetén SDL_Renderer és TTF_Font van, semminek nem változtatja
meg az értékét. */
void presentreplay(SDL_Renderer *render, TTF_Font *font){
    SDL_Rect dest = {580, 360, 0, 0};
    SDL_RenderClear(render);
    text(render, font, "Back to Menu: Press b", dest);
    dest.y = 389;
    text(render, font, "Replay (no saves): Press r", dest);
    SDL_RenderPresent(render);
}
/*Játék közben renderereli le a játék pályát.*/
void rendermap(SDL_Renderer *render){
    SDL_Rect dest1 = {0, 520, 600, 200};
    boxRGBA(render, 0, 520, 600, 720, 211, 211, 211, 255);
    rectangleRGBA(render, 0, 520, 600, 720, 211, 211, 211, 255);
    dest1.x = 600, dest1.y = 620, dest1.w = 60, dest1.h = 100;
    boxRGBA(render, dest1.x, dest1.y, dest1.x + dest1.w, dest1.y+dest1.h, 211, 211, 211, 255);
    rectangleRGBA(render, dest1.x, dest1.y, dest1.x + dest1.w, dest1.y+dest1.h, 211, 211, 211, 255);
    dest1.x = 660, dest1.y = 470, dest1.w = 620, dest1.h = 250;
    boxRGBA(render, dest1.x, dest1.y, dest1.x + dest1.w, dest1.y+dest1.h, 211, 211, 211, 255);
    rectangleRGBA(render, dest1.x, dest1.y, dest1.x + dest1.w, dest1.y+dest1.h, 211, 211, 211, 255);
    dest1.x = 20, dest1.y = 500, dest1.w = 20, dest1.h = 30;
    /*a halált jelentő "tüskék" megrajzolása*/
    while(dest1.x < 600){
        SDL_RenderDrawRect(render, &dest1);
        SDL_RenderFillRect(render, &dest1);
        dest1.x += 40;
    }
    dest1.x = 660, dest1.y = 450;
    while(dest1.x != 1260){
        SDL_RenderDrawRect(render, &dest1);
        SDL_RenderFillRect(render, &dest1);
        dest1   .x += 40;
    }
}
