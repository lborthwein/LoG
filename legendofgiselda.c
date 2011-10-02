
#define _XOPEN_SOURCE 500

#include <curses.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


// Number of lives to give the player
#define LIVES 3;

// Structure to store a unit's position
typedef struct pos
{
    int y;
    int x;
} pos;

// Stores an enemy type's information and dimensions
typedef struct enemy {
    char *up;
    char *down;
    char *left;
    char *right;
    int w;
    int h;
} enemy;


static void finish(int sig);
void movech(int dir);
bool chngdir(int dir);
void *attack(void *arg);
void *chmover(void *arg);
void *music(void *arg);
void *enemymover(void *arg);
void drawchar(int cy, int cx);
struct pos scanunderg(int c);
void areachange (char d);
void makeenemy(int eny, int enx, int enplace);
int drawenemy(int eny, int enx, struct enemy type, int diren, bool first);
bool moveenemy(int numen, struct enemy type, int diren);
void delenemy(int eny, int enx, struct enemy type, int diren);
bool enemydead (int eny, int enx, struct enemy type, int diren);
void chardead(struct pos p);
void scandead (void);
void killenemy (int eny, int enx, struct enemy type, int diren);


// Position in dungeon; to start at a later point alter these initial
// values (and be sure the x and y values provide a valid start for
// your character on the chose area)
int dungeony = 0;  // 0, 10 default -- 4,12 boss
int dungeonx = 10;

bool quit = FALSE;

int x = 40, y = 30, ry, rx, dir = 1, rdir;
char *livesnote = " LIVES LEFT";
char *upface = "|&&&&$()$} \\/  ";
char *downface = "  /\\ {$()$&&&&|";
char *leftface = "&{.  &(I>*__.  ";
char *rightface = "  .__*<I)&  .}&";

// Keeps track of enemies by giving each a unique number with encounter
// and then storing their types and directions in typecounter and encounter
enemy *typecounter; 
int enemyface[20], encounter = 0;

// Enemy art
char *bigheadf = "& ^^^^^ &| Y Y |&|  _  |& -- -- -->~Y~&&&&&&^&&&";
char *bigheadb = " ^^^^^ &|^^^^^|&| ^^^ |& -- -- &&&~Y~<--&&&^&&&&";

char *hapsadf = "&&__&&~(`')~&&++&&";
char *hapsadb = "&&__&&~(^^)~&&++&&";

char *spinshh = "&&&&&&>0<&&&&&&";
char *spinshdr = "&&&/_&&0&&7&&&&";
char *spinshv = "&&v&&&&0&&&&^&&";
char *spinshdl = "_\\&&&&&0&&&&&|\\";

char *scorpf = "&_&&&/ \\&&@-[]-&-[]-&<`'>&()()";
char *scorpb = "()()&<..>&-[]-&-[]-?&&\\_/&&&&&";

char *wifef = "&@@@@&@|,,|@&|__|&&-/\\-&&/__\\&";
char *wifeb = "&@@@@&@@@@@@&|__|&&-/\\-&&/__\\&";

char *babyf = "&88&|,,|&gg&";
char *babyb = "&88&|  |&gg&";

char *owlf = "&,_,&(0,0)/\\./\\&'&`&";
char *owlb = "&,_,&(   )/\\_/\\&'&`&";

char *bossf = "&&<_A_>&&(* *)\\1/(-)&&|_\\ /_&|&&V&&&|&/&\\&";
char *bossb = "<_A_>&&(   )&&&( )\\1/_\\ /_|&&&V&&|&&/&\\&|&";

char *bomb = "\\\\\\||///\\\\*##*//-*####*-//*##*\\\\///||\\\\\\";

char *lost = "GAME OVER";
char *congrats = "YOU WIN!!!";
char *paused = "PAUSED";

// Undergrid is where all information regarding gameplay is stored
// (ie where an enemy or a wall is, where your character is, etc)
char undergrid[50][80];

bool chdead = FALSE;
int lives = LIVES;

// How much health the boss at the end of the game has
int bosslife = 3;

/* Initialize mutex variables:
 *
 * stdscrl locks the stdscr, the struct dictating what
 * is to be printed onscreen next time a refresh occurs
 *
 * ugl locks the undergrid
 *
 * movel locks move functions for the character and enemies
 * to make sure nothing occurs in between scandead() calls
 * and while the game is paused
 */
pthread_mutex_t stdscrl, ugl, movel;


int
main(int argc, char *argv[])
{
    int i; 
    
    // make pthreads
    pthread_t chmovert;
    pthread_t musict;
    pthread_t enemymovert;

    // initialize typecounter
    typecounter = (enemy*) calloc(10, sizeof(struct enemy));
    
    (void) initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    (void) nonl();         /* tell curses not to do NL->CR/NL on output */
    (void) cbreak();       /* take input chars one at a time, no wait for \n */
    (void) noecho();       /* don't echo input */
    nodelay(stdscr, FALSE); /* wait for keystrokes */
    

    if (has_colors())
    {
        start_color();


        /*
         * Simple color pair assignment
         */
        init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_CYAN);
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
        init_pair(15, COLOR_WHITE, COLOR_BLUE);
    }

    // Start the bgm
    pthread_create(&musict, NULL, music, NULL);

    // Initialize starting area
    areachange('s');
    
    // Begin the character moving thread
    pthread_create(&chmovert, NULL, chmover, NULL);
    
    // Begin the enemy moving thread
    pthread_create(&enemymovert, NULL, enemymover, NULL);

    for (;;)
    {
        // Subtract 1 life if character is hit and redraw stage
        // or make it game over if character is out of lives
        if (chdead == TRUE){
            lives--;
            if (lives > 0){
                x = rx;
                y = ry;
                dir = rdir;
                areachange('s');
                chdead = FALSE;
                mvaddch(0, 2, (lives + 48) | A_BOLD);
                for (i=0; i<11; i++){
                    mvaddch(0, 3+i, livesnote[i]);
                }
            }
            else {
            pthread_cancel(musict);
            pthread_cancel(chmovert);
            pthread_cancel(enemymovert);
            for (i=0; i<9; i++){
                mvaddch(20, 35+i, lost[i]);
                refresh();
                sleep(1);
            }
            break;
            }
        }
        
        // Display victory animation if boss is dead
        if (bosslife == 0){
            pthread_cancel(musict);
            pthread_cancel(chmovert);
            pthread_cancel(enemymovert);
            for (i=0; i<10; i++){
                mvaddch(13, 35+i, congrats[i] | A_BLINK | A_BOLD);
                refresh();
                sleep(1);
            }
            break;
        }
        
        // If players quits (by pressing q), game exits
        if (quit == TRUE)
            break;
    }
    // Destroy locks and exit curses mode
    pthread_mutex_destroy(&stdscrl);
    pthread_mutex_destroy(&ugl);
    pthread_mutex_destroy(&movel);
    finish(0);
}

static void finish(int sig)
{
    endwin();
    return;
}


/* CHARACTER MOVING THREAD
 * and associated functions
 */

void *chmover (void * arg)
{
    int c = 0, i;
    for(;;){
        c = getch();     /* refresh, accept single keystroke of input */

        /* process the command keystroke */

        // if q pressed, quit game
        if(c == 'q'){
            quit = TRUE;
            break;
        }

        // if p pressed, stall all processing of movement or screen change
        // until p pressed again, or game is quit with q (pause the game)
        if(c == 'p'){
            pthread_mutex_lock(&movel);
            pthread_mutex_lock(&stdscrl);
            for (i=0; i<6; i++){
                mvaddch(0, 2+i, paused[i] | A_BLINK);
            }
            for(;;){
                c = getch();
                if (c == 'q'){
                    quit = TRUE;
                    pthread_mutex_unlock(&movel);
                    pthread_mutex_unlock(&stdscrl);
                    break;
                }
                if (c == 'p'){
                    pthread_mutex_unlock(&movel);
                    pthread_mutex_unlock(&stdscrl);
                    for (i=0; i<6; i++){
                        mvaddch(0, 2+i, ' ');
                    }
                    break;
                }
            }
        }

        // Try to change characters direction if a direction key
        // is pressed other than the one the character is currently
        // facing (indicated by the variable dir). Otherwise, move
        // the character one space if the direction was indeed the
        // same direction he was facing
        pthread_mutex_lock(&movel);
        if(chngdir(c)){
            movech(c);
            pthread_mutex_lock(&stdscrl);
            refresh();
            pthread_mutex_unlock(&stdscrl);
        }
        pthread_mutex_unlock(&movel);

        // If space bar pressed, attack in the direction the character
        // is facing
        if (c == ' '){
            attack(NULL);
        }
        
        // Scan for any dead enemies or if the character has died, and if so
        // take appropriate action (this is also done once for each potentially
        // harmful frame of the attack function)
        scandead();
        
    }
    return NULL;
}




bool
chngdir(int c)
{
    int a = 0, b = 1, i, j;
    switch (c) {
      case KEY_UP: /* user pressed up arrow key*/ 
        if(dir == 1){
            return TRUE;
        }
        else{
            pthread_mutex_lock(&stdscrl);
            pthread_mutex_lock(&ugl);
            move(y-1,x-2);
            for (i=0; i<3; i++){
                for(j=0; j<5; j++){
                    if (upface[a] == '&'){
                        addch(' ');
                        undergrid[y-1+i][x-2+j] = ' ';
                        a++;
                        continue;
                    }
                    addch(upface[a] | COLOR_PAIR(2));
                    if (undergrid[y-1+i][x-2+j] != 'c' &&
                        upface[a] != '|' && upface[a] != '}')
                        undergrid[y-1+i][x-2+j] = 'b'; 
                    if (upface[a] == '|' || upface[a] == '}' 
                        || upface[a] == ' ')
                        undergrid[y-1+i][x-2+j] = 'z'; 
                    a++;
                }
                move(y-1+b, x-2);
                b++;
            }
            a = 0;
            b = 1;
            dir = 1;
            move(0,0);
            pthread_mutex_unlock(&stdscrl);
            pthread_mutex_unlock(&ugl);
            return FALSE;
        }
        break;
      case KEY_DOWN:  /* user pressed down arrow key */
        if(dir == 2){
            return TRUE;
        }
        else{
            pthread_mutex_lock(&stdscrl);
            pthread_mutex_lock(&ugl);
            move(y-1,x-2);
            for (i=0; i<3; i++){
                for(j=0; j<5; j++){
                    if (downface[a] == '&'){
                        addch(' ');
                        undergrid[y-1+i][x-2+j] = ' ';                        
                        a++;
                        continue;
                    }
                    addch(downface[a] | COLOR_PAIR(2));
                    if (undergrid[y-1+i][x-2+j] != 'c' &&
                        downface[a] != '|' && downface[a] != '{')
                        undergrid[y-1+i][x-2+j] = 'b';     
                    if (downface[a] == '|' || downface[a] == '{'
                        || downface[a] == ' ')
                        undergrid[y-1+i][x-2+j] = 'z';
                    a++;
                }
                move(y-1+b, x-2);
                b++;
            }
            a = 0;
            b = 1;
            dir = 2;
            move(0,0);
            pthread_mutex_unlock(&stdscrl);
            pthread_mutex_unlock(&ugl);
            return FALSE;
        }
        break;
      case KEY_LEFT:  /* user pressed left arrow key */
        if(dir == 3){
            return TRUE;
        }
        else{
            pthread_mutex_lock(&stdscrl);
            pthread_mutex_lock(&ugl);
            move(y-1,x-2);
            for (i=0; i<3; i++){
                for(j=0; j<5; j++){
                    if (leftface[a] == '&'){
                        addch(' ');
                        undergrid[y-1+i][x-2+j] = ' ';
                        a++;
                        continue;
                    }
                    addch(leftface[a] | COLOR_PAIR(2));
                    if (undergrid[y-1+i][x-2+j] != 'c' &&
                        leftface[a] != '_' && leftface[a] != '{')
                        undergrid[y-1+i][x-2+j] = 'b';
                    if (leftface[a] == '_' || leftface[a] == '{'
                        || leftface[a] == ' ')
                        undergrid[y-1+i][x-2+j] = 'z';
                    a++;
                }
                move(y-1+b, x-2);
                b++;
            }
            a = 0;
            b = 1;
            dir = 3;
            move(0,0);
            pthread_mutex_unlock(&stdscrl);
            pthread_mutex_unlock(&ugl);
            return FALSE;
        }
            break;
      case KEY_RIGHT:  /* user pressed right arrow key */
        if(dir == 4){
            return TRUE;
        }
        else{
            pthread_mutex_lock(&stdscrl);
            pthread_mutex_lock(&ugl);
            move(y-1,x-2);
            for (i=0; i<3; i++){
                for(j=0; j<5; j++){
                    if (rightface[a] == '&'){
                        addch(' ');
                        undergrid[y-1+i][x-2+j] = ' ';
                        a++;
                        continue;
                    }
                    addch(rightface[a] | COLOR_PAIR(2));
                    if (undergrid[y-1+i][x-2+j] != 'c' &&
                        rightface[a] != '_' && rightface[a] != '}')
                        undergrid[y-1+i][x-2+j] = 'b';
                    if (rightface[a] == '_' || rightface[a] == '}'
                        || rightface[a] == ' ')
                        undergrid[y-1+i][x-2+j] = 'z';
                    a++;
                }
                move(y-1+b, x-2);
                b++;
            }
            a = 0;
            b = 1;
            dir = 4;
            move(0,0);
            pthread_mutex_unlock(&stdscrl);
            pthread_mutex_unlock(&ugl);
            return FALSE;
        }
            break;
            
    }
    return FALSE;
}


void
movech(int c)
{
    int a = 0, b = 1, i, j;
    switch (c) {
      case KEY_UP: /* user pressed up */ 
        for (j=0; j<5; j++){
            if (undergrid[y-2][x-2+j] == 'u'){
                areachange('u');
            }
            if (undergrid[y-2][x-2+j] == 'w'){
                goto nomove;
            }
        }
        move(y-2,x-2);
        pthread_mutex_lock(&stdscrl);
        pthread_mutex_lock(&ugl);
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (upface[a] == '&'){
                    move(y-2 + i,x-1 + j);
                    a++;
                    continue;
                }
                addch(upface[a] | COLOR_PAIR(2));
                if (undergrid[y-2+i][x-2+j] != 'c' &&
                    upface[a] != '|' && upface[a] != '}')
                    undergrid[y-2+i][x-2+j] = 'b'; 
                if (upface[a] == '|' || upface[a] == '}' 
                    || upface[a] == ' ')
                    undergrid[y-2+i][x-2+j] = 'z'; 
                a++;
            }
                move(y-2+b, x-2);
                b++;
        }
        move(y+1, x-2);
        for (i=0; i<5; i++){
                addch(' ');
                undergrid[y+1][x-2+i] = ' ';
        }
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        undergrid[y][x] = 'b';
        y=y-1;
        undergrid[y][x] = 'c';
        pthread_mutex_unlock(&ugl);
        a=0;
        b=1;
        break;
      case KEY_DOWN:  /* user pressed down arrow key */
        for (j=0; j<5; j++){
            if (undergrid[y+2][x-2+j] == 'd'){
                areachange('d');
            }
            if (undergrid[y+2][x-2+j] == 'w'){
                goto nomove;
            }
        }
        pthread_mutex_lock(&stdscrl);
        pthread_mutex_lock(&ugl);
        move(y,x-2);
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (downface[a] == '&'){
                    move(y + i,x-1 + j);
                    a++;
                    continue;
                }
                addch(downface[a] | COLOR_PAIR(2));
                if (undergrid[y+i][x-2+j] != 'c' &&
                    downface[a] != '|' && downface[a] != '}')
                    undergrid[y+i][x-2+j] = 'b'; 
                if (downface[a] == '|' || downface[a] == '}' 
                    || downface[a] == ' ')
                    undergrid[y+i][x-2+j] = 'z';
                a++;
            }
            move(y+b, x-2);
            b++;
        }
        move(y-1, x-2);
        for (i=0; i<5; i++){
            addch(' ');
            undergrid[y-1][x-2+i] = ' ';
        }
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        undergrid[y][x] = 'b';
        y=y+1;
        undergrid[y][x] = 'c';
        pthread_mutex_unlock(&ugl);
        a=0;
        b=1;           
        break;
      case KEY_LEFT:  /* user pressed left arrow key */
        for (i=0; i<3; i++){
            if (undergrid[y-1+i][x-3] == 'l'){
                areachange('l');
            }
            if (undergrid[y-1+i][x-3] == 'w'){
                goto nomove;
            }
        }
        pthread_mutex_lock(&stdscrl);
        pthread_mutex_lock(&ugl);
        move(y-1,x-3);
        for (i=0; i<3; i++){
            for(j=0; j<6; j++){
                if(j==5){
                    addch(' ');
                    undergrid[y-1+i][x-3+j] = ' ';
                    continue;
                }
                if (leftface[a] == '&'){
                    move(y-1 + i,x-2 + j);
                    a++;
                    continue;
                }
                addch(leftface[a] | COLOR_PAIR(2));
                if (undergrid[y-1+i][x-3+j] != 'c' &&
                    leftface[a] != '_' && leftface[a] != '{')
                    undergrid[y-1+i][x-3+j] = 'b'; 
                if (leftface[a] == '_' || leftface[a] == '{' 
                    || leftface[a] == ' ')
                    undergrid[y-1+i][x-3+j] = 'z';
                a++;
            }
            move(y-1+b, x-3);
            b++;
        }
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        undergrid[y][x] = 'b';
        x=x-1;
        undergrid[y][x] = 'c';
        pthread_mutex_unlock(&ugl);
        a=0;
        b=1;  
        break;
      case KEY_RIGHT:  /* user pressed right */
        for (i=0; i<3; i++){
            if (undergrid[y-1+i][x+3] == 'r'){
                areachange('r');
            }
            if (undergrid[y-1+i][x+3] == 'w'){
                goto nomove;
            }
        }
        pthread_mutex_lock(&stdscrl);
        pthread_mutex_lock(&ugl);
        move(y-1,x-2);
        for (i=0; i<3; i++){
            for(j=0; j<6; j++){
                if(j==0){
                    addch(' ');
                    undergrid[y-1+i][x-2+j] = ' ';
                    continue;
                }
                if (rightface[a] == '&'){
                    move(y-1 + i,x-1 + j);
                    a++;
                    continue;
                }
                addch(rightface[a] | COLOR_PAIR(2));
                if (undergrid[y-1+i][x-2+j] != 'c' &&
                    rightface[a] != '_' && rightface[a] != '}')
                    undergrid[y-1+i][x-2+j] = 'b'; 
                if (rightface[a] == '_' || rightface[a] == '}' 
                    || rightface[a] == ' ')
                    undergrid[y-1+i][x-2+j] = 'z';
                a++;
            }
            move(y-1+b, x-2);
            b++;
        }
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        undergrid[y][x] = 'b';
        x=x+1;
        undergrid[y][x] = 'c';
        pthread_mutex_unlock(&ugl);
        a=0;
        b=1;  
        break; 
      case ERR:
        break;
    }
    usleep(80000);
    flushinp();
 nomove:
    return;
}


void *attack (void *arg)
{
    int i;
    switch (dir)
    {
      case 1:
        pthread_mutex_lock(&stdscrl);
        move(y-1, x-2);
        addch(' ');
        for (i=0; i<3; i++){
            if (undergrid[y-1+i][x-3] == 'l'
                || undergrid[y-1+i][x-3] == 'w'){
                goto shortswordup;
            }
        }
        move(y,x-4);
        if (undergrid[y][x-4] != 'l'
            && undergrid[y][x-4] != 'w'){    
            addch('-');
        }
        else{
            move(y,x-3);
        }
        addch('-');
        pthread_mutex_lock(&ugl);
        if (undergrid[y][x-4] != 'l'
            && undergrid[y][x-4] != 'w'){   
            undergrid[y][x-4] = 's';
        }
        undergrid[y][x-3] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);  
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y,x-4);
        if (undergrid[y][x-4] != 'l'
            && undergrid[y][x-4] != 'w'){    
            addch(' ');
        }
        else{
            move(y,x-3);
        }
        addch(' ');
        pthread_mutex_lock(&ugl);
        if (undergrid[y][x-4] != 'l'
            && undergrid[y][x-4] != 'w'){
            undergrid[y][x-4] = ' ';
        }
        undergrid[y][x-3] = ' ';
        pthread_mutex_unlock(&ugl);
        move(y-1,x-3);
        addch('\\');
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x-3] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0); 
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y-1,x-3);
        addch(' ');
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x-3] = ' ';
        pthread_mutex_unlock(&ugl);
      shortswordup:
        move(y-1,x-2);
        addch('|');
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x-2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);       
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y-1,x-2);
        addch(' ');
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x-2] = ' ';
        pthread_mutex_unlock(&ugl);
        addch('/');
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x-1] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y-1,x-1);
        addch(' ');
        move(0,0);
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x-1] = ' ';
        pthread_mutex_unlock(&ugl);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        mvaddch(y-1, x-2, '|' | COLOR_PAIR(2));
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        break;
      case 2:
        pthread_mutex_lock(&stdscrl);
        move(y+1, x+2);
        addch(' ');
        for (i=0; i<3; i++){
            if (undergrid[y-1+i][x+3] == 'r'
                || undergrid[y-1+i][x+3] == 'w'){
                goto shortsworddown;
            }
        }
        move(y,x+3);
        if (undergrid[y][x+4] != 'r'
            && undergrid[y][x+4] != 'w'){    
            addch('-');
        }
        addch('-');
        pthread_mutex_lock(&ugl);
        if (undergrid[y][x+4] != 'r'
            && undergrid[y][x+4] != 'w'){   
            undergrid[y][x+4] = 's';
        }
        undergrid[y][x+3] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);   
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);    
        move(y,x+3);
        if (undergrid[y][x+4] != 'r'
            && undergrid[y][x+4] != 'w'){
            addch(' ');
        }
        addch(' ');
        pthread_mutex_lock(&ugl);
        if (undergrid[y][x+4] != 'r'
            && undergrid[y][x+4] != 'w'){
            undergrid[y][x+4] = ' ';
        }
        undergrid[y][x+3] = ' ';
        pthread_mutex_unlock(&ugl);
        move(y+1,x+3);
        addch('\\');
        pthread_mutex_lock(&ugl);
        undergrid[y+1][x+3] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0); 
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);       
        move(y+1,x+3);
        addch(' ');
        pthread_mutex_lock(&ugl);
        undergrid[y+1][x+3] = ' ';
        pthread_mutex_unlock(&ugl);
      shortsworddown:
        move(y+1,x+2);
        addch('|');
        pthread_mutex_lock(&ugl);
        undergrid[y+1][x+2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0); 
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);       
        move(y+1,x+2);
        addch(' ');
        pthread_mutex_lock(&ugl);
        undergrid[y+1][x+2] = ' ';
        pthread_mutex_unlock(&ugl);
        move(y+1,x+1);
        addch('/');
        pthread_mutex_lock(&ugl);
        undergrid[y+1][x+1] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y+1,x+1);
        addch(' ');
        move(0,0);
        refresh();
        pthread_mutex_unlock(&stdscrl);
        usleep(50000);
        pthread_mutex_lock(&ugl);
        undergrid[y+1][x+1] = ' ';
        pthread_mutex_unlock(&ugl);
        pthread_mutex_lock(&stdscrl);
        mvaddch(y+1, x+2, '|' | COLOR_PAIR(2));
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        break;
      case 3:
        pthread_mutex_lock(&stdscrl);
        mvaddch(y+1, x-2, ' ');
        addch('.' | COLOR_PAIR(2));
        addch(' ');
        if (undergrid[y+2][x-2] == 'd'
            || undergrid[y+2][x-2] == 'w'){
            goto shortswordleft;
        }
        mvaddch(y+2,x-2, '/');
        pthread_mutex_lock(&ugl);
        undergrid[y+2][x-2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);        
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y+2,x-2);
        addch(' ');
        pthread_mutex_lock(&ugl);
        undergrid[y+2][x-2] = ' ';
        pthread_mutex_unlock(&ugl);
      shortswordleft:
        move(y+1,x-3);
        if (undergrid[y+1][x-3] != 'l'
            && undergrid[y+1][x-3] != 'w'){
            addch('_');
        }
        else {
            move(y+1,x-2);
        }
        addch('_');
        pthread_mutex_lock(&ugl);
        if (undergrid[y+1][x-3] != 'w'
            && undergrid[y+1][x-3] != 'l'){
            undergrid[y+1][x-3] = 's';
        }
        undergrid[y+1][x-2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);        
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y+1,x-3);
        if (undergrid[y+1][x-3] != 'l'
            && undergrid[y+1][x-3] != 'w'){
            addch(' ');
        }
        else{
            move(y+1,x-2);
        }
        addch(' ');
        pthread_mutex_lock(&ugl);
        if (undergrid[y+1][x-3] != 'w'
            && undergrid[y+1][x-3] != 'l'){
            undergrid[y+1][x-3] = ' ';
        }
        undergrid[y+1][x-2] = ' ';
        pthread_mutex_unlock(&ugl);
        move(y,x-2);
        addch('\\');
        pthread_mutex_lock(&ugl);
        undergrid[y][x-2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);        
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(75000);
        pthread_mutex_lock(&stdscrl);
        move(y,x-2);
        addch(' ');
        move(0,0);
        pthread_mutex_lock(&ugl);
        undergrid[y][x-2] = ' ';
        pthread_mutex_unlock(&ugl);
        refresh();
        pthread_mutex_unlock(&stdscrl);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        mvaddch(y+1, x-2, '_' | COLOR_PAIR(2));
        addch('_' | COLOR_PAIR(2));
        addch('.' | COLOR_PAIR(2));
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        break;
      case 4:
        pthread_mutex_lock(&stdscrl);
        mvaddch(y-1, x, ' ');
        addch('.' | COLOR_PAIR(2));
        addch(' ');
        if (undergrid[y-2][x+2] == 'u'
            || undergrid[y-2][x+2] == 'w'){
            goto shortswordright;
        }
        mvaddch(y-2,x+2, '/');
        pthread_mutex_lock(&ugl);
        undergrid[y-2][x+2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);        
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y-2,x+2);
        addch(' ');
        pthread_mutex_lock(&ugl);
        undergrid[y-2][x+2] = ' ';
        pthread_mutex_unlock(&ugl);
      shortswordright:
        move(y-1,x+2);
        if (undergrid[y-1][x+3] != 'r'
            && undergrid[y-1][x+3] != 'w'){
            addch('_');
        }
        addch('_');
        pthread_mutex_lock(&ugl);
        undergrid[y-1][x+2] = 's';
        if (undergrid[y-1][x+3] != 'w'
            && undergrid[y-1][x+3] != 'r'){
            undergrid[y-1][x+3] = 's';
        }
        pthread_mutex_unlock(&ugl);
        move(0,0);        
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        move(y-1,x+2);
        if (undergrid[y-1][x+3] != 'r'
            && undergrid[y+1][x+3] != 'w'){
            addch(' ');
        }
        addch(' ');
        pthread_mutex_lock(&ugl);
        if (undergrid[y-1][x+3] != 'w'
            && undergrid[y-1][x+3] != 'r'){
            undergrid[y-1][x+3] = ' ';
        }
        undergrid[y-1][x+2] = ' ';
        pthread_mutex_unlock(&ugl);
        move(y,x+2);
        addch('\\');
        pthread_mutex_lock(&ugl);
        undergrid[y][x+2] = 's';
        pthread_mutex_unlock(&ugl);
        move(0,0);        
        putchar('\007');
        refresh();
        pthread_mutex_unlock(&stdscrl);
        pthread_mutex_unlock(&movel);
        scandead();
        pthread_mutex_lock(&movel);
        usleep(75000);
        pthread_mutex_lock(&stdscrl);
        move(y,x+2);
        addch(' ');
        move(0,0);
        pthread_mutex_lock(&ugl);
        undergrid[y][x+2] = ' ';
        pthread_mutex_unlock(&ugl);
        refresh();
        pthread_mutex_unlock(&stdscrl);        
        usleep(50000);
        pthread_mutex_lock(&stdscrl);
        mvaddch(y-1, x, '.' | COLOR_PAIR(2));
        addch('_' | COLOR_PAIR(2));
        addch('_' | COLOR_PAIR(2));
        move(0,0);
        pthread_mutex_unlock(&stdscrl);
        break; 
    }
    pthread_mutex_unlock(&movel);
    usleep(200000);
    flushinp();
    return NULL;
}


/* ENEMY MOVING THREAD
 * and associated functions
 */

void *enemymover(void *arg)
{
    int check;
    pos pe;
    do
    {
        // For each enemy  with a number assigned by encounter, if
        // to the left of character move right or vice versa
        for (check = encounter; check>0; check--){
            pe = scanunderg(check);
            if(pe.y != 0 || pe.x != 0){
                if (pe.x<x){
                    if (moveenemy(check, typecounter[check], 4))
                        continue;
                }
                if (pe.x>x){
                    if (moveenemy(check, typecounter[check], 3))
                        continue;
                }
            }
        }
        scandead();
        usleep(200000);

        // For each enemy  with a number assigned by encounter, if
        // below character move up or vice versa (enemies must pause longer
        // for gameplay balance purposes)
        for (check = encounter; check>0; check--){
            pe = scanunderg(check);
            if(pe.y != 0 || pe.x != 0){
                if (pe.y<y){
                    if (moveenemy(check, typecounter[check], 2)){
                        usleep(100000);
                        continue;
                    }
                }
                if (pe.y>y){
                    if (moveenemy(check, typecounter[check], 1)){
                        usleep(100000);
                        continue;
                    }
                }
            }
        }
        scandead();
        usleep(200000);
    } while (chdead == FALSE);
    return NULL;
}

bool
moveenemy(int numen, struct enemy type, int diren)
{
    struct pos pe, pc; 
    int j, i, check;
    pe = scanunderg(numen);
    pc = scanunderg('c');
        
    pthread_mutex_lock(&ugl);

    switch (diren){
      case 1: /* enemy moving up */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (undergrid[pe.y-1-(type.h/2)+i][pe.x-(type.w/2)+j] == 'w'){
                    pthread_mutex_unlock(&ugl);
                    return FALSE;
                }
                for (check=encounter; check > 0; check--){
                    if (undergrid[pe.y-1-(type.h/2)+i][pe.x-(type.w/2)+j] == check && check != numen){
                        pthread_mutex_unlock(&ugl);
                        return FALSE;
                    }
                }
            }
        }
        delenemy(pe.y, pe.x, type, enemyface[numen]);    
        drawenemy(pe.y - 1, pe.x, type, 1, 0);
        enemyface[numen] = 1;
        undergrid[pe.y][pe.x] = ' ';
        undergrid[pe.y-1][pe.x] = numen;
        break;
      case 2: /* down */
        for(i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (undergrid[pe.y+1+(type.h/2)-i][pe.x-(type.w/2)+j] == 'w'){
                    pthread_mutex_unlock(&ugl);
                    return FALSE;
                }
                for (check=encounter; check>0; check--){
                    if (undergrid[pe.y+1+(type.h/2)-i][pe.x-(type.w/2)+j] == check && check != numen){
                        pthread_mutex_unlock(&ugl);
                        return FALSE;
                    }
                }
            }
        }
        delenemy(pe.y, pe.x, type, enemyface[numen]);    
        drawenemy(pe.y + 1, pe.x, type, 2, 0);
        enemyface[numen] = 2;
        undergrid[pe.y][pe.x] = ' ';
        undergrid[pe.y+1][pe.x] = numen;
        break;
      case 3: /* left */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (undergrid[pe.y-(type.h/2)+i][pe.x-1-(type.w/2)+j] == 'w'){
                    pthread_mutex_unlock(&ugl);
                    return FALSE;
                }
                for (check=encounter; check>0; check--){
                    if (undergrid[pe.y-(type.h/2)+i][pe.x-1-(type.w/2)+j] == check && check != numen){
                        pthread_mutex_unlock(&ugl);
                        return FALSE;
                    }
                }
            }
        }
        delenemy(pe.y, pe.x, type, enemyface[numen]);    
        if (type.left != NULL){
            drawenemy(pe.y, pe.x - 1, type, 3, 0);
            enemyface[numen] = 3;
            undergrid[pe.y][pe.x] = ' ';
            undergrid[pe.y][pe.x-1] = numen;
            break;
        }
        if (pe.y <= pc.y){
            drawenemy(pe.y, pe.x - 1, type, 2, 0);
            enemyface[numen] = 2;
        }
        else{
            drawenemy(pe.y, pe.x - 1, type, 1, 0);
            enemyface[numen] = 1;
        }
        undergrid[pe.y][pe.x] = ' ';
        undergrid[pe.y][pe.x-1] = numen;
        break;
      case 4: /* right */
        for (i=0; i<type.h; i++){
            for(j=0; j<type.w; j++){
                if (undergrid[pe.y-(type.h/2)+i][pe.x+1+(type.w/2)-j] == 'w'){
                    pthread_mutex_unlock(&ugl);
                    return FALSE;
                }
                for (check=encounter; check>0; check--){
                    if (undergrid[pe.y-(type.h/2)+i][pe.x+1+(type.w/2)-j] == check && check != numen){
                        pthread_mutex_unlock(&ugl);
                        return FALSE;
                    }
                }
            }
        }   
        delenemy(pe.y, pe.x, type, enemyface[numen]); 
        if (type.right != NULL){
            drawenemy(pe.y, pe.x + 1, type, 4, 0);
            enemyface[numen] = 4;
            undergrid[pe.y][pe.x] = ' ';
            undergrid[pe.y][pe.x+1] = numen;
            break;
        }
        if (pe.y <= pc.y){
            drawenemy(pe.y, pe.x + 1, type, 2, 0);
            enemyface[numen] = 2;
        }
        else{
            drawenemy(pe.y, pe.x + 1, type, 1, 0);
            enemyface[numen] = 1;
        }
        undergrid[pe.y][pe.x] = ' ';
        undergrid[pe.y][pe.x+1] = numen;
        break;
    }
    pthread_mutex_unlock(&ugl);
    move(0,0);
    return TRUE;
}

/* Draws an enemy at a certain x and y on the screen */

int
drawenemy(int eny, int enx, struct enemy type, int diren, bool first)
{
    int i, j, a = 0, b = 1;
    pthread_mutex_trylock(&stdscrl);
    move(eny-(type.h/2), enx-(type.w/2));
    switch (diren){
      case 1: /* enemy facing up */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.up[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(type.up[a] | COLOR_PAIR(COLOR_RED));
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
      case 2: /* down */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.down[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(type.down[a] | COLOR_PAIR(COLOR_RED));
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
      case 3: /* left */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.left[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(type.left[a] | COLOR_PAIR(COLOR_RED));
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
      case 4: /* right */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.right[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(type.right[a] | COLOR_PAIR(COLOR_RED));
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
    }
    pthread_mutex_unlock(&stdscrl);
    if (first == TRUE){
        encounter++;
        typecounter[encounter] = type;
        undergrid[eny][enx] = encounter;
        enemyface[encounter] = diren;
        
        
    }
    return 1;
}

/* Deletes select enemy from the screen */

void delenemy (int eny, int enx, struct enemy type, int diren)
{
    int i, j, a = 0, b = 1;
    pthread_mutex_lock(&stdscrl);
    move(eny-(type.h/2), enx-(type.w/2));
    switch (diren){
      case 1: /* enemy facing up */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.up[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(' ');
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
      case 2: /* down */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.down[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(' ');
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
      case 3: /* left */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.left[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(' ');
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
      case 4: /* right */
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.right[a] == '&'){
                    move(eny-(type.h/2) + i, enx-(type.w/2) + 1 + j);
                    a++;
                    continue;
                }
                addch(' ');
                a++;
            }
            move(eny-(type.h/2)+b, enx-(type.w/2));
            b++;
        }
        break;
    }
    move(0,0);
    pthread_mutex_unlock(&stdscrl);
    return;
}




/* SCAN THE UNDERGRID
 * to find any int and return its coordinates
 */

struct pos
scanunderg(int c)
{
    int i, j;
    struct pos p;
    for (i=0; i<50; i++){
        for (j=0; j<80; j++){
            if (undergrid[i][j] == c){
                p.y = i;
                p.x = j;
                return p;
            }
        }
    }
    p.y = 0;
    p.x = 0;
    return p;
}



/* MUSIC THREAD */

void *music(void *arg)
{
    int i, m;
    for(;;){
        for(m=0; m<4; m++){
            for (i=0; i<16; i++){
                if(i == 0 || i == 2 || i == 3 ||
                   i == 4 || i == 6 || i == 7 ||
                   i == 9 || i == 11 ||
                   i == 12){
                    pthread_mutex_lock(&stdscrl);
                    // Play terminal beep
                    putchar('\007');
                    refresh();
                    pthread_mutex_unlock(&stdscrl);
                }
                if(m==2 && i == 14){
                    pthread_mutex_lock(&stdscrl);
                    // Play terminal beep
                    putchar('\007');
                    refresh();
                    pthread_mutex_unlock(&stdscrl);
                }
                if(m==3 && i>12){
                    pthread_mutex_lock(&stdscrl);
                    // Play terminal beep
                    putchar('\007');
                    refresh();
                    pthread_mutex_unlock(&stdscrl);
                }
                

                usleep(125000);
                if (quit == TRUE)
                    goto end;
            }
        }
    }
 end:
    return NULL;
}

/* SCAN FOR DEAD ENEMIES OR CHARACTERS
 * and associated functions
 */

void scandead (void)
{
    pos p;
    //Check if character is dead by being moved into
    pthread_mutex_lock(&movel);
    for(;;){
        p = scanunderg('c');
        if (p.y != 0 || p.x != 0){
            chardead(p);
            break;
        }
    }
    pthread_mutex_unlock(&movel);
    int check;
    //Check if enemies are dead or if char is dead by moving into enemy
    for (check = encounter; check>0; check--){
        p = scanunderg(check);
        if(p.y != 0 || p.x != 0){
            if(enemydead(p.y, p.x, typecounter[check], enemyface[check])){
                killenemy(p.y, p.x, typecounter[check], enemyface[check]);
            } 
        }
    }    
    return;
}

/* Check if the character is dead by being moved into */

void
chardead(struct pos p)
{
    int a = 0, i, j;
    switch (dir) {
      case 1: /* link facing up */ 
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (upface[a] == '&' || upface[a] == '|' 
                    || upface[a] == '}'){
                    a++;
                    continue;
                }
                if(undergrid[y-1+i][x-2+j]!='b'){
                    if(undergrid[y-1+i][x-2+j]!='c'){
                        if(undergrid[y-1+i][x-2+j]!='s'){
                            if(undergrid[y-1+i][x-2+j]!='z'){
                            chdead = TRUE;   
                            }
                        }
                    }
                }
                a++;
            }
        }
        a = 0;
        break;
      case 2:  /* down */
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (downface[a] == '&'|| downface[a] == '|' 
                    || downface[a] == '{'){
                    a++;
                    continue;
                }
                if(undergrid[y-1+i][x-2+j]!='b'){
                    if(undergrid[y-1+i][x-2+j]!='c'){
                        if(undergrid[y-1+i][x-2+j]!='s'){
                            if(undergrid[y-1+i][x-2+j]!='z'){
                                chdead = TRUE;
                            }
                        }
                    }
                }
                a++;
            }
        }
        a = 0;
        break;
      case 3:  /* left */
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (leftface[a] == '&'|| leftface[a] == '_' 
                    || leftface[a] == '{'){
                    a++;
                    continue;
                }
                if(undergrid[y-1+i][x-2+j]!='b'){
                    if(undergrid[y-1+i][x-2+j]!='c'){
                        if(undergrid[y-1+i][x-2+j]!='s'){
                            if(undergrid[y-1+i][x-2+j]!='z'){
                                chdead = TRUE;
                            }
                        }
                    }
                }
                a++;
            }
        }
        a = 0;
        break;
      case 4:  /* right */
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (rightface[a] == '&'|| rightface[a] == '_' 
                    || rightface[a] == '}'){
                    a++;
                    continue;
                }
                if(undergrid[y-1+i][x-2+j]!='b'){
                    if(undergrid[y-1+i][x-2+j]!='c'){
                        if(undergrid[y-1+i][x-2+j]!='s'){
                            if(undergrid[y-1+i][x-2+j]!='z'){
                                chdead = TRUE;
                            }
                        }
                    }
                }
                a++;
            }
        }
        a = 0;
        break;
    }
    return;
}

/* Check if enemy is dead or if the character moved into an enemy */

bool
enemydead (int eny, int enx, struct enemy type, int diren)
{
    int i, j, a = 0;
    switch (diren){
      case 1:
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.up[a] == '&'){
                    a++;
                    continue;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 's'){
                    return TRUE;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 'b'){
                    chdead = TRUE;
                }
                a++;
            }
        }
        break;
      case 2:
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.down[a] == '&'){
                    a++;
                    continue;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 's'){
                    return TRUE;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 'b'){
                    chdead = TRUE;
                }
                a++;
            }
        }
        break;
      case 3:
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.left[a] == '&'){
                    a++;
                    continue;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 's'){
                    return TRUE;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 'b'){
                    chdead = TRUE;
                }
                a++;
            }
        }
        break;
      case 4:
        for (i=0; i<type.h; i++){
            for (j=0; j<type.w; j++){
                if (type.right[a] == '&'){
                    a++;
                    continue;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 's'){
                    return TRUE;
                }
                if(undergrid[eny-(type.h/2)+i][enx-(type.w/2)+j] == 'b'){
                    chdead = TRUE;
                }
                a++;
            }
        }
        break;
    }
    return FALSE;   
}

/* Performs kill animation and all necessary undergrid cleanup for enemy */

void 
killenemy (int eny, int enx, struct enemy type, int diren)
{
    int i, j, a=0, col;
    pthread_mutex_lock(&ugl);
    delenemy(eny, enx, type, diren);
    if(undergrid[eny][enx] != 'c'){
        undergrid[eny][enx] = ' ';
    }
    pthread_mutex_unlock(&ugl);
    pthread_mutex_lock(&stdscrl);
    for(i=0; i<5; i++){
        for(j=0; j<8; j++){
            if (bomb[a] == '#'){
                col = 15;
            }
            else{
                col = COLOR_MAGENTA;
            }
            mvaddch(eny-(type.h/2)+i, enx-(type.w/2)+j, bomb[a] | COLOR_PAIR(col));
            a++;
        }
    }
    putchar('\a');
    refresh();
    pthread_mutex_unlock(&stdscrl);
    if (dungeony == 4 && dungeonx == 12){
        if (bosslife > 0){
            areachange('s');
        }
        bosslife--;
    }
    return;
}


/* DRAWS NEW AREA
 * and the enemies who spawn there
 */

void
areachange (char d)
{
    switch (d){
      case 'u':
        dungeony++;
        y = 44;
        break;
      case 'd':
        dungeony--;
        y = 3;
        break;
      case 'l':
        dungeonx--;
        x = 75;
        break;
      case 'r':
        dungeonx++;
        x = 4;
        break;
      case 's':
        break;
    }
    
    ry = y;
    rx = x;
    rdir = dir;
    encounter = 0;
    char areafile[10];
    
    sprintf(areafile, "%d-%d.txt", dungeony, dungeonx);
    FILE *areaptr = fopen(areafile, "r");
    
    int i, j, a = 0;
    
    for(i=0; i<50; i++){
        fread(undergrid[i], sizeof(char), 80, areaptr);
    }
    fclose(areaptr);
    
    for(i=0; i<50; i++){
        for(j=0; j<80; j++){
            if (undergrid[i][j] == 'w')
                mvaddch(i, j, ' ' | COLOR_PAIR(COLOR_CYAN));
            else
                mvaddch(i, j, ' ');
        }
    }
    
    for(i=0; i<50; i++){
        for(j=0; j<80; j++){
            if (undergrid[i][j] == 'e'){
                makeenemy(i, j, a);
                a++;
            }
        }
    }
    
    drawchar(y, x);
    clearok(stdscr, TRUE);
    return;   
}

/* Spawns new enemy */

void
makeenemy(int eny, int enx, int enplace)
{
    int i;

    struct enemy bighead;
    bighead.up = bigheadb;
    bighead.down = bigheadf;
    bighead.left = NULL;
    bighead.right = NULL;
    bighead.w = 8;
    bighead.h = 6;

    struct enemy hapsad;
    hapsad.up = hapsadb;
    hapsad.down = hapsadf;
    hapsad.left = NULL;
    hapsad.right = NULL;
    hapsad.w = 6;
    hapsad.h = 3;

    struct enemy spinsh;
    spinsh.up = spinshv;
    spinsh.down = spinshh;
    spinsh.left = spinshdl;
    spinsh.right = spinshdr;
    spinsh.w = 5;
    spinsh.h = 3;
    
    struct enemy scorp;
    scorp.up = scorpb;
    scorp.down = scorpf;
    scorp.left = NULL;
    scorp.right = NULL;
    scorp.w = 5;
    scorp.h = 6;

    struct enemy wife = { wifeb, wifef, NULL, NULL, 6, 5 };

    struct enemy baby = { babyb, babyf, NULL, NULL, 4, 3 };
    
    struct enemy owl = { owlb, owlf, NULL, NULL, 5, 4 };

    struct enemy boss = { bossb, bossf, NULL, NULL, 7, 6 };

    struct enemy *enlayout[20][20][10];
    enlayout[1][10][0] = &scorp;
    enlayout[1][9][0] = &owl;
    enlayout[2][9][0] = &owl;
    enlayout[2][9][1] = &owl;
    enlayout[2][9][2] = &owl;
    enlayout[2][9][3] = &owl;
    enlayout[2][9][4] = &hapsad;
    enlayout[2][9][5] = &hapsad;
    enlayout[3][9][0] = &owl;
    enlayout[3][9][1] = &owl;
    enlayout[3][9][2] = &owl;
    enlayout[3][10][0] = &spinsh;
    enlayout[3][10][1] = &spinsh;
    enlayout[3][10][2] = &spinsh;
    enlayout[3][10][3] = &spinsh;
    enlayout[2][10][0] = &bighead;
    enlayout[2][10][1] = &bighead;
    enlayout[2][10][2] = &bighead;
    enlayout[2][10][3] = &bighead;
    enlayout[2][11][0] = &wife;
    enlayout[2][11][1] = &wife;
    enlayout[2][11][2] = &wife;
    enlayout[2][11][3] = &wife;
    for (i=0; i<5; i++){
        enlayout[2][12][i] = &scorp;
    }
    for (i=3; i<7; i++){
        enlayout[3][12][i] = &hapsad;
    }
    enlayout[3][12][0] = &bighead;    
    enlayout[3][12][1] = &wife;
    enlayout[3][12][2] = &baby;
    enlayout[4][12][0] = &boss;
    
    
    
    drawenemy(eny, enx, *enlayout[dungeony][dungeonx][enplace], 2, TRUE);
    return;
}

/* Spawns character (in new area of if killed) */
            
void
drawchar(int cy, int cx)
{
    y = cy;
    x = cx;
    int a = 0, b = 1, i, j;
    pthread_mutex_lock(&stdscrl);
    pthread_mutex_lock(&ugl);
    switch (dir) {
      case 1: /* up */ 
        move(y-1,x-2);
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (upface[a] == '&'){
                    addch(' ');
                    a++;
                    continue;
                }
                addch(upface[a] | COLOR_PAIR(2));
                if(i==1 && j==2)
                    undergrid[y-1+i][x-2+j] = 'c';
                else
                    undergrid[y-1+i][x-2+j] = 'b';
                a++;
            }
            move(y-1+b, x-2);
            b++;
        }
        a = 0;
        b = 1;
        move(0,0);
        break;
      case 2:  /* down */
        move(y-1,x-2);
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (downface[a] == '&'){
                    addch(' ');
                    a++;
                    continue;
                }
                addch(downface[a] | COLOR_PAIR(2));
                if(i==1 && j==2)
                    undergrid[y-1+i][x-2+j] = 'c';
                else
                    undergrid[y-1+i][x-2+j] = 'b';
                a++;
            }
            move(y-1+b, x-2);
            b++;
        }
        a = 0;
        b = 1;
        dir = 2;
        move(0,0);
        break;
      case 3:  /* left */
        move(y-1,x-2);
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (leftface[a] == '&'){
                    addch(' ');
                    a++;
                    continue;
                }
                addch(leftface[a] | COLOR_PAIR(2));
                if(i==1 && j==2)
                    undergrid[y-1+i][x-2+j] = 'c';
                else
                    undergrid[y-1+i][x-2+j] = 'b';
                a++;
            }
            move(y-1+b, x-2);
            b++;
        }
        a = 0;
        b = 1;
        dir = 3;
        move(0,0);
        break;
      case 4:  /* right */
        move(y-1,x-2);
        for (i=0; i<3; i++){
            for(j=0; j<5; j++){
                if (rightface[a] == '&'){
                    addch(' ');
                    a++;
                    continue;
                }
                addch(rightface[a] | COLOR_PAIR(2));
                if(i==1 && j==2)
                    undergrid[y-1+i][x-2+j] = 'c';
                else
                    undergrid[y-1+i][x-2+j] = 'b';
                a++;
            }
            move(y-1+b, x-2);
            b++;
        }
        a = 0;
        b = 1;
        dir = 4;
        move(0,0);
        break;       
    }
    pthread_mutex_unlock(&stdscrl);
    pthread_mutex_unlock(&ugl);
    return;
}
