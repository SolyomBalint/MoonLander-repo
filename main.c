#include <SDL.h>
#include <SDL_image.h>
#include <SDL2_gfxPrimitives.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "graphics.h"
#include "controles&rules.h"
#include "debugmalloc.h"

/*az idő méréséhez szükséges struktúra, a sec1 a másodperc első, míg a sec2 a másodperc második digitjét tartja számon.*/
typedef struct Time{
    int min, sec1, sec2;
} Time;

/*A játék állás visszatöltését lehetővé tevő láncolt lista struktúrája. 10 mp-nként eltáolja a játékállás: pillanatnyi x,y sebessegég
és az űrhajó pozíciója a térképen (SDL_Rect struktúra első két tagja)*/
typedef struct Replay{
    Time time;
    double xspeed, yspeed;
    int x, y;
    struct Replay *next;
} Replay;

/*A fájlkezelést megegyszerűsítő struktúra. A felhasználó nevek max 8 karakter hosszúak lehetnek,
így elég a végjeles string.*/
typedef struct Userlist{
    char username[9];
    int timeval;
} Userlist;

/*Függvény a főjátékot működtető ciklushoz. 50 fps-et generál.*/
Uint32 timer(Uint32 ms, void *param){
    SDL_Event event;
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    return ms;
}
/*Bemenetén csak a replay funkció működéséhez szükséges pointer van, ami egy láncolt lista
első elemére mutat. A függvény felszabadítja ezt a listát, nincs visszatérési értéke.*/
void freelist(Replay *replay){
    Replay *step;
    step = replay;
    while(step != NULL){
        Replay *del = step->next;
        free(step);
        step = del;
    }
    replay = NULL;
}
/*A képernyő bal felső sarkában jeleníti meg az aktuális x,y sebességet, és az eltelt időt. Bemenetein nem változtat, kimenete nincs.*/
void displaydata(SDL_Renderer *render, TTF_Font *font, Time time, double xspeed, double yspeed, int fuel){
    SDL_Rect dest = {0, 0, 0, 0};
    yspeed *= (-1);
    char str[5];
    sprintf(str,"%d:%d%d", time.min, time.sec1, time.sec2);
    str[4] = '\0';
    text(render, font, str, dest);
    dest.y = 29;
    char str1[13];
    sprintf(str1,"xspeed: %.2f", xspeed);
    text(render, font, str1, dest);
    dest.y+=29;
    sprintf(str1,"yspeed: %.2f", yspeed);
    text(render, font, str1, dest);
    sprintf(str1, "Fuel: %d", fuel);
    dest.y += 29;
    text(render, font, str1, dest);
}

/*Ez a függvény valósítja meg a vissza töltés nagy részét. Bemenetén van a szövegek, és képek megjelenítéséhez szükséges
pointerek: render, font, window. Ezenkívűl a játék állást elmentő láncolt lista első tagjára mutató pointer,
illetve az éppen aktuális idő és sebesség. Ha a játékos érévnyes visszajátszási pontot választ a függvény
átállítja a dest struktúra x és y változóit a kiválasztott időpontéra, ugyan ezt teszi a sebességgel is.*/
bool Replaymenu(SDL_Renderer *render, TTF_Font *font, Replay *replay, SDL_Window *window, SDL_Rect *dest, double *xspeed, double *yspeed, Time *time){
    SDL_RenderClear(render);
    SDL_Rect rect = {500, 0, 0, 0};
    text(render, font,"Go back to:", rect);
    char str[35];
    int i=1, j=0;
    Replay *pstep = replay;
    /*29 pixelenként lépkedve lefele kiírja a láncolt listában eltárolt visszajátszási adatokat.*/
    while(pstep != NULL){
        rect.y+=29;
        sprintf(str,"%d%d. Time: %d:%d%d", j, i, pstep->time.min, pstep->time.sec1, pstep->time.sec2);
        str[14] = '\0';
        text(render, font, str, rect);
        ++i;
        if(i>9){
            i=0;
            ++j;
        }
        pstep = pstep->next;
    }
    SDL_RenderPresent(render);
    char text[10];
    SDL_Color white = {255,255,255}, black = {0,0,0};
    /*A felhasználó inputjára vár, azzal kapcoslatban, hogy meddig szeretné visszatekerni*/
    rect.x = 0, rect.y = 0, rect.h = 40, rect.w = 400;
    input_text(text, 100, rect, black, white, font, render);
    int whereinlist=0;;
    /*egy stringben található számok értékét adja össze*/
    char *p = text;
    while(*p){
        if(isdigit(*p)){
            int val = strtof(p, &p);
            whereinlist += val;
        }
        else
            ++p;
    }

    Replay *contgame = replay;
    int k=1;
    while(k<whereinlist){
        contgame = contgame->next;
        if(contgame == NULL)
            return false;
        ++k;
    }
    dest->x = contgame->x;
    dest->y = contgame->y;
    *xspeed = contgame->xspeed;
    *yspeed = contgame->yspeed;

    return true;
}
/*Elleőnrzi egy fájl sorának hosszát*/
int linelength(FILE *const lbout){
    int c, count;
    count = 0;
    for ( ;; ){
        c = fgetc(lbout);
        if(c == '\n' || c == EOF)
            break;
        ++count;
    }
    return count;
}

/*Megnézi, hogy milyen hosszú az adott fájl (hány sorból áll)*/
int filesize(FILE *const lbout){
    int c, count;
    count = 0;
    for( ;; ){
        c = fgetc(lbout);
        if(c == '\n')
            ++count;
        if(c == EOF){
            break;
        }
    }
    return count;
}
/*Visszaadja egy stringben szereplő számok összegének értékét.*/
int stringvalue(char *text){
    char *p = text;
    int sum = 0;
    while(*p){
        if(isdigit(*p)){
            int val = strtof(p, &p);
            sum += val;
        }
        else
            ++p;
    }
    return sum;
}
/*A teljes fájlkezelést lebonyolító függvény*/
void writeleaderboards(char *username, int fuel){
    FILE *lbout;
    lbout = fopen("leaderboards.txt", "rt");
    bool  partoflist = false;
    int arraylength;
    int newvalue = fuel;
    arraylength = filesize(lbout);
    bool adding = false;
    /*Ha  nincsen feltöltve a dicsőséglista (max 10) hozzá ad egyet a hosszához, hogy oda később beírja az új felgasználót*/
    if(arraylength < 10){
        arraylength += 1;
        adding = true;
    }
    /*miután az arraylength rendelkezik a fájl hosszával (hány sorból áll) lefoglal egy dinamikus tömböt
    ami Userlist struktúrából áll.*/
    Userlist *filecontent = (Userlist*) malloc((arraylength)*sizeof(Userlist));
    /*amennyiben nincsen teljesen feltöltve a lista*/
    if(arraylength < 10 || adding == true){
        int k=0;
        fseek(lbout, 0, SEEK_SET);
        /*A txt fájlban található edatokat beolvassa a dinamikusan lefoglalat tömbbe.*/
        while(k < arraylength-1){
            int line = linelength(lbout)+1;
            fseek(lbout, -line-1, SEEK_CUR);
            char *currentline = (char*) malloc(line*sizeof(char));
            fgets(currentline, line, lbout);
            fseek(lbout, 2, SEEK_CUR);
            filecontent[k].timeval = stringvalue(currentline);
            int counter = 0;
            for(; currentline[counter] != ':'; ++counter){
                filecontent[k].username[counter] = currentline[counter];
                filecontent[k].username[counter+1] = '\0';
            }
            free(currentline);
            ++k;
            }
        /*Az utolsó helyre beirja az új elemet*/
        filecontent[arraylength-1].timeval = newvalue;
        for(int j=0; username[j] != '\0'; ++j){
            filecontent[arraylength-1].username[j] = username[j];
            filecontent[arraylength-1].username[j+1] = '\0';
        }
    }
    /*Amennyiben tele van a lista*/
    else{
        int k=0;
        fseek(lbout, 0, SEEK_SET);
        /*szintén bemásolja a tömmbe az adatokat.*/
        while(k < arraylength){
            int line = linelength(lbout)+1;
            fseek(lbout, -line-1, SEEK_CUR);
            char *currentline = (char*) malloc(line*sizeof(char));
            fgets(currentline, line, lbout);
            fseek(lbout, 2, SEEK_CUR);
            filecontent[k].timeval = stringvalue(currentline);
            int counter = 0;
            for(; currentline[counter] != ':'; ++counter){
                filecontent[k].username[counter] = currentline[counter];
                filecontent[k].username[counter+1] = '\0';
            }
            free(currentline);
            ++k;
        }
            /*Ellőnrzi, hogy van a a listán már az új username, és, hogy jobb-e a pontja
            ha az felülírja.*/
            for(int i=0; i<arraylength; ++i){
                int identical = strcmp(username, filecontent[i].username);
                if( identical == 0){
                    if(newvalue>filecontent[i].timeval){
                        filecontent[i].timeval = newvalue;
                        partoflist = true;
                    }
                }
            }
            /*Amennyiben nem tagja a listának az új username, ellenőrzi, hogy van-e veleki akinél
            több pontja van, amennyiben igen, az utolsó helyre beírja az új nevet(hiszen az utolsó így is
            úgy is kiesik.*/
            if(partoflist == false){
                for(int i=0; i < arraylength; ++i){
                    if(newvalue>filecontent[i].timeval){
                        partoflist = true;
                        filecontent[arraylength-1].timeval = newvalue;
                        for(int j=0; username[j] != '\0'; ++j){
                            filecontent[arraylength-1].username[j] = username[j];
                            filecontent[arraylength-1].username[j+1] = '\0';
                        }
                    }
                    if(partoflist == true)
                        break;
                }
            }
        }
        /*növekvő sorrendben rendezi a tömböt*/
        for(int i = 0; i < arraylength - 1; ++i){
                int minindex = i;
                for(int j = i+1; j < arraylength; ++j){
                    if(filecontent[j].timeval < filecontent[minindex].timeval)
                        minindex = j;
                }
                if(minindex != i){
                    Userlist temp = filecontent[minindex];
                    filecontent[minindex] = filecontent[i];
                    filecontent[i] = temp;
                }
            }
    /*a tömb új tartalmát beirja a txt fájlba*/
    FILE *lbin = fopen("leaderboards.txt", "wt");
        for(int i = 0; i < arraylength; ++i){
            fprintf(lbin, "%s: %d points\n", filecontent[i].username, filecontent[i].timeval);
        }
    free(filecontent);
    fclose(lbin);
    fclose(lbout);
}

int main(int argc, char *argv[]){
    /*A fizikai számok SI-ben vannak megadva (m, kg, s, m/s, m/s2 stb.), de az 50 fps-es időzítőhöz vannak igazítva.*/
    SDL_Window *window;
    SDL_Renderer *render;
    TTF_Font *font;
    SDL_Surface *surface;
    SDL_Texture *texture;
    int width=1280, height=720;
    double moonvel = 0.1, engvelx = 0.06, engvely = 0.2;
    double xspeed=0, yspeed=0;
    int fuel = 1500;
    int y_pos=50, x_pos=100;
    char username[9];
    SDL_Rect dest = {x_pos, y_pos, 36, 128};
    /*idő méréséhez, és a replay funkció működtetéséhez szükséges változók.*/
    Time time = {0, 0, 0};
    int counter = 0;
    Replay *replay = NULL;
    /*Menü megnyitása, és inputra várás*/
    openwindow(width, height, &window, &render, &font);
    SDL_RenderPresent(render);
    SDL_TimerID id = SDL_AddTimer(20, timer, NULL);
    /*Logikai változók*/
    bool usedreplay = false;
    bool quitprog = false;
    bool buttonleft=false, buttonright=false, buttonup=false;
    bool won = false, lost = false;
    while(!quitprog){
        SDL_SetRenderDrawColor(render, 0, 0, 0, 1);
        openmenu(render, width, height, font);
        SDL_Event event;
        Menustate state = menuinput(&quitprog);
        /*A kapott input alapján vagy elindítja a játékot, megnyitja a leaderboardsot
        vagy kilép a programból.*/
        switch(state){
            case start:
                loadpicture(render, &texture, dest, &surface,"spaceshipidle.xcf");
                SDL_RenderPresent(render);
                /*a játék fő ciklusa, kilépésig futni fog, illetve speciális esetekben breakel kilép(győzelem, vagy vereség esetén)*/
                while(!quitprog){
                    SDL_WaitEvent(&event);
                    /*replay funkció*/
                    if(lost == true && replay != NULL && usedreplay == false){
                        bool succes;
                        lost = false;
                        buttonleft = false;
                        buttonright = false;
                        buttonup = false;
                        SDL_Rect rect = {580, 360, 0, 0};
                        text(render, font, "You Failed", rect);
                        SDL_RenderPresent(render);
                        sleep(3);
                        /*választás, hogy akar-e visszatölteni*/
                        presentreplay(render, font);
                        bool rep = replayinput(&quitprog);
                        if(rep == true){
                            /*hova akar visszatölteni*/
                            usedreplay = true;
                            succes=Replaymenu(render, font, replay, window, &dest, &xspeed, &yspeed, &time);
                            freelist(replay);
                            if(succes == false){
                                text(render, font, "invalid input, Returning to menu in 3", rect);
                                SDL_RenderPresent(render);
                                sleep(3);
                                break;
                            }
                        }
                        else
                            break;
                    }
                    /*győzelem esetén*/
                    if(won == true){
                        won = false;
                        SDL_Rect rect = {width/2-100, height/2-50, 400, 40};
                        text(render, font, "Successful Landing", rect);
                        SDL_RenderPresent(render);
                        sleep(2);
                        SDL_RenderClear(render);
                        text(render, font, "Enter an username! max 8 char. and no numbers!", rect);
                        SDL_Color white = {255,255,255}, black = {0,0,0};
                        rect.y = 420;
                        input_text(username, 9, rect, black, white, font, render);
                        writeleaderboards(username, fuel);
                        break;
                    }
                    /*a gép által generált event. Egyetlen egy időzítő alapján működik a teljes játék, így ezen az if-en belül hívódik meg
                    az összes olyan függvény, ami a játék megjelenítéséhez feltétlenül szükséges.*/
                    if(event.type == SDL_USEREVENT){
                        SDL_RenderClear(render);
                        rendermap(render);
                        shipcontrol(buttonup, buttonleft, buttonright, &dest, moonvel, engvelx, engvely, &xspeed, &yspeed);
                        presentpicture(render, texture, dest);
                        /*számláló óra működtetése, mivel az üzemenyag hamarabb elfogy, minthogy, elérné
                        a 10 percet, így nem szükséges tovább írni.*/
                        if(counter<50)
                            ++counter;
                        else{
                            counter=0;
                            if(time.sec2<9)
                                ++time.sec2;
                            else{
                                time.sec2=0;
                                ++time.sec1;
                                /*játék állsá mentése*/
                                if(usedreplay == false){
                                    Replay *pnew = (Replay*) malloc(sizeof(Replay));
                                    if(time.sec1 == 6){
                                        pnew->time.min = time.min+1;
                                        pnew->time.sec1 = 0;
                                        pnew->time.sec2 = time.sec2;
                                       }
                                       else{
                                        pnew->time.min = time.min;
                                        pnew->time.sec1 = time.sec1;
                                        pnew->time.sec2 = time.sec2;
                                       }
                                        pnew->xspeed = xspeed;
                                        pnew->yspeed = yspeed;
                                        pnew->x = dest.x;
                                        pnew->y = dest.y;
                                        pnew->next = NULL;
                                    if(replay == NULL)
                                        replay = pnew;
                                    else{
                                        Replay *step = replay;
                                        while(step->next != NULL){
                                            step = step->next;
                                        }
                                        step->next = pnew;
                                    }
                                }
                            }
                            if(time.sec1==6){
                                time.sec1=0;
                                ++time.min;
                            }
                        }
                        /*üzemanyag fogyási sebessége*/
                        if(buttonleft || buttonright || buttonup){
                            fuel -= 1;
                        }
                        rules(&lost, &won, dest, width, height, fuel, xspeed, yspeed);
                        displaydata(render, font, time, xspeed, yspeed, fuel);
                        SDL_SetRenderDrawColor(render, 0, 0, 0, 1);
                        SDL_RenderPresent(render);

                    }
                    /*tényleges user event(fel-,jobbra-,balra nyilak). Ellenőrzi a kapott inputot, és az alapján tölti be a megfelelő képeket, illetve állítja át a bool változókat.*/
                    if(event.type == SDL_KEYDOWN){
                        if(event.key.keysym.sym == SDLK_ESCAPE)
                            break;
                        if(event.key.keysym.sym == SDLK_LEFT)
                            buttonleft = true;
                        if(event.key.keysym.sym == SDLK_RIGHT)
                            buttonright = true;
                        if(event.key.keysym.sym == SDLK_UP)
                            buttonup = true;
                        if(buttonup){
                            loadpicture(render, &texture, dest, &surface,"spaceshipup.xcf");
                        }
                        if(buttonleft){
                            loadpicture(render, &texture, dest, &surface,"spaceshipleft.xcf");
                        }
                        if(buttonright){
                            loadpicture(render, &texture, dest, &surface,"spaceshipright.xcf");
                        }
                        if(buttonleft && buttonup){
                            loadpicture(render, &texture, dest, &surface,"spaceshipup&left.xcf");
                        }
                        if(buttonright && buttonup){
                            loadpicture(render, &texture, dest, &surface,"spaceshipup&right.xcf");
                        }
                    }
                    if(event.type == SDL_KEYUP){
                        if(event.key.keysym.sym == SDLK_LEFT)
                            buttonleft = false;
                        if(event.key.keysym.sym == SDLK_RIGHT)
                            buttonright = false;
                        if(event.key.keysym.sym == SDLK_UP)
                            buttonup = false;
                        if(!buttonleft && !buttonright && !buttonup)
                            loadpicture(render, &texture, dest, &surface,"spaceshipidle.xcf");
                        if(buttonleft && !buttonright && !buttonup)
                            loadpicture(render, &texture, dest, &surface,"spaceshipleft.xcf");
                        if(buttonright && !buttonleft && !buttonup)
                            loadpicture(render, &texture, dest, &surface,"spaceshipright.xcf");
                        if(buttonup && !buttonleft && !buttonright)
                            loadpicture(render, &texture, dest, &surface,"spaceshipup.xcf");
                    }
                    /*Ha a játékos replay után is veszít*/
                    if(lost == true && usedreplay == true){
                        SDL_Rect pos = {580, 360, 0, 0};
                        text(render, font, "You Failed Again", pos);
                        SDL_RenderPresent(render);
                        sleep (3);
                        break;
                    }
                    /*ha a játékos 10 mp-nél hamarabb veszít*/
                    if(lost == true && time.sec1 == 0 && time.min < 1){
                        lost = false;
                        buttonleft = false;
                        buttonright = false;
                        buttonup = false;
                        usedreplay = true;
                        SDL_Rect rect = {580, 360, 0, 0};
                        text(render, font, "You Failed", rect);
                        rect.y = 389;
                        rect.x = 580;
                        text(render, font, "No Replay before 10 sec", rect);
                        SDL_RenderPresent(render);
                        sleep(3);
                        break;
                    }
                    /*x-el való kilépés*/
                    if(event.type == SDL_QUIT){
                        freelist(replay);
                        quitprog = true;
                    }
                }
                /*játék vége utáni resetelés.*/
                if(!usedreplay)
                    freelist(replay);
                replay = NULL;
                buttonleft = false;
                buttonright = false;
                buttonup = false;
                time.min = 0;
                time.sec1 = 0;
                time.sec2 = 0;
                dest.x = 50;
                dest.y = 100;
                yspeed = 0;
                xspeed = 0;
                fuel = 1500;
                lost = false;
                usedreplay = false;
                SDL_DestroyTexture(texture);

            break;
            case leaderboards:
                /*fájlból beolvas, majd kiír, végül escape után visszatér a főmenübe.*/
                SDL_RenderClear(render);
                SDL_Rect pos = {580, 20, 0, 0};
                text(render, font, "Best players:", pos);
                FILE *fp = fopen("leaderboards.txt", "rt");
                int arraylength = filesize(fp);
                int k = 0;
                fseek(fp, 0, SEEK_SET);
                while(k < arraylength){
                    int line = linelength(fp)+1;
                    fseek(fp, -line-1, SEEK_CUR);
                    char *currentline = (char*) malloc(line*sizeof(char));
                    fgets(currentline, line, fp);
                    fseek(fp, 2, SEEK_CUR);
                    pos.y += 29;
                    text(render, font, currentline, pos);
                    ++k;
                    free(currentline);
                }
                fclose(fp);
                pos.y += 29;
                text(render, font,"Press Escape To return menu", pos);
                SDL_RenderPresent(render);
                leaderboardsevent(&quitprog);
            break;
            case quit:
                TTF_CloseFont(font);
                SDL_RemoveTimer(id);
                SDL_DestroyRenderer(render);
                SDL_DestroyTexture(texture);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 0;
            break;
        }
    }

    TTF_CloseFont(font);
    SDL_RemoveTimer(id);
    SDL_DestroyRenderer(render);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
