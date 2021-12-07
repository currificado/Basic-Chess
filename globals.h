#include <memory.h>
#include <curses.h>
//#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
//#include <dos.h>
#include <time.h>

void ShowAll(int ply);//

#define U64 uint64_t

#define A1	0
#define B1	1
#define C1	2
#define D1	3
#define E1	4
#define F1	5
#define G1	6
#define H1	7

#define A2	8
#define B2	9
#define C2	10
#define D2	11
#define E2	12
#define F2	13
#define G2	14
#define H2	15

#define A3	16
#define B3	17
#define C3	18
#define D3	19
#define E3	20
#define F3	21
#define G3	22
#define H3	23

#define A4	24
#define B4	25
#define C4	26
#define D4	27
#define E4	28
#define F4	29
#define G4	30
#define H4	31

#define A5	32
#define B5	33
#define C5	34
#define D5	35
#define E5	36
#define F5	37
#define G5	38
#define H5	39

#define A6	40
#define B6	41
#define C6	42
#define D6	43
#define E6	44
#define F6	45
#define G6	46
#define H6	47

#define A7	48
#define B7	49
#define C7	50
#define D7	51
#define E7	52
#define F7	53
#define G7	54
#define H7	55

#define A8	56
#define B8	57
#define C8	58
#define D8	59
#define E8	60
#define F8	61
#define G8	62
#define H8	63

#define NORTH 0
#define NE 1
#define EAST 2
#define SE 3
#define SOUTH 4
#define SW 5
#define WEST 6
#define NW 7

#define P 0
#define N 1
#define B 2
#define R 3
#define Q 4
#define K 5
#define EMPTY 6

#define White 0
#define Black 1

#define MAX_PLY 64
#define MOVE_STACK 2000
#define GAME_STACK 2000

#define HASH_SCORE    100000000
#define CAPTURE_SCORE 10000000

#define MTY_WHI_SQR 1
#define MTY_BLK_SQR 2
#define WHI_SQR_W 3
#define WHI_SQR_B 4
#define BLK_SQR_W 5
#define BLK_SQR_B 6

/* Estructura que contiene la información de una jugada durante el análisis de una variante */
typedef struct {
	int start;  // casilla origen
	int dest;   // casilla destino
	int promote;// pieza a la que se promocionó si la jugada fue promoción (0 en caso contrario)
	int score;  // puntaje de la jugada (el motor analiza primero las jugadas con más alto puntaje)
} move_data;

/* an element of the history stack, with the information
   necessary to take a move back. */
typedef struct {
	int start;// casilla origen
	int dest;// casilla destino
	int promote;// pieza a la que se promocionó si la jugada fue promoción (0 en caso contrario)
	int capture;// pieza capturada si la jugada fue una captura (EMPTY en caso contrario)
	int fifty;// contador de jugadas desde que hubo movimiento de peón o captura
	int castle_q[2];// permiso de enrocar largo (para Blancas[0]/Negras[1])
	int castle_k[2];// permiso de enrocar corto (para Blancas[0]/Negras[1])
	U64 hash;// clave
	U64 lock;// cerrojo
} game;

extern int piece_value[6]; // array con el valor de las piezas en centipeones (100 centipeones = 1 peón)
extern int pawn_mat[2]; // valor material de peones en centipeones
extern int piece_mat[2]; // valor material del resto de las piezas en centipeones

extern int king_endgame[2][64]; // valor de la posición del rey en finales
extern int passed[2][64]; // puntaje de los peones pasados

extern char piece_char[6]; // letra correspondiente a cada pieza

extern int side,xside; // side es a quien le toca mover (0=Blancas, 1=Negras), xside es el bando contrario
extern int fifty; // contador de jugadas desde que hubo captura o movimiento de peón
extern int ply,hply; // ply es el contador de jugadas para la variante actual, hply es el contador global

extern int nodes; // cantidad de nodos analizados en una variante

extern int board[64]; // arreglo con la disposición de las piezas en el tablero
extern int color[64]; // color de la pieza que ocupa ese escaque (0=Negras, 1=Blancas, 6=Vacío)
extern int startpos_board[64]; // con esto se inicializará `board`
extern int startpos_color[64]; // con esto se inicializará `color`
extern int kingloc[2]; // posición de los reyes

/* En el análisis de una variante, cuando ocurre una poda alfa-beta 
   (y es un movimiento que no es captura), se guarda la jugada que la 
   ocasionó en este arreglo, de la forma:
   history[casilla inicial][casilla final] += profundidad
   (Se incrementa el puntaje a través de la profundidad en que ocurrió 
   la poda.)
   Cuando se generan las jugadas con Gen(), a las jugadas que no son 
   captura se les asigna el score de history. Hay que tener presente 
   que sólo se tiene en cuenta la casilla inicial y final, no el tipo 
   de pieza. Esto hace que la eficiencia sea un poco peor, ya que a 
   una jugada se le puede asignar un puntaje creyendo que era otra. 
   De cualquier forma, son pocos los casos en que ocurre esto.
*/
extern int history[64][64];

/* Valor de las piezas, i.e. 
   Valor material + Puntaje dependiendo de la posición 
   El primer índice es Blancas/Negras, 
   el segundo es la pieza y 
   el tercero es la casilla donde se encuentra 
*/
extern int square_score[2][6][64];

extern const int row[64]; // da el número de fila de una casilla
extern const int col[64]; // da el número de columna de una casilla

extern move_data move_list[MOVE_STACK]; // array con todas las moves de una variante
extern int first_move[MAX_PLY]; // first_move[i] da el índice de move_list donde arrancan las jugadas de la ply i
extern game game_list[GAME_STACK]; // lista de todas las jugadas que tuvieron lugar en la partida

extern int hash_start,hash_dest; // variables globales donde se guarda la jugada extraída de la tabla de hash
extern U64 currentkey,currentlock; // clave y cerrojo de la posición actual

extern int fixed_time; // "booleano" para cuando hay un tiempo máximo personalizado
extern int fixed_depth; // "booleano" para cuando hay una profundidad máxima personalizada
extern int max_time; // tiempo máximo de búsqueda
extern int start_time; // marca de tiempo inicial
extern int stop_time; // marca de tiempo de parada (va a ser el tiempo inicial + tiempo máximo)
extern int max_depth; // máxima profundidad

/* Dada una casilla 0<=x<=63, qrb_moves[x] es un array de 9 elementos
   con las casillas adyacentes alcanzables por la dama (-1 marca el 
   fin de la lista) 
*/
extern int qrb_moves[64][9];
extern int knight_moves[64][9]; // casillas alcanzables por el caballo (-1 marca el fin de la lista)
extern int king_moves[64][9]; // casillas alcanzables por el rey (-1 marca el fin de la lista)

extern unsigned int hashpositions[2]; // contador de la cantidad de posiciones en la tabla de hash (hay uno para cada bando)
extern U64 collisions; // contador de colisiones (nunca se usa esta variable en el código)

extern int turn; // contador de turnos (plys jugadas)

extern int Flip[64]; // da la casilla que es imagen especular respecto a la mitad del tablero (no como si el tablero se hubiera girado 180 grados)

/* `rank` da el número de fila para cada casilla 
(dependiendo si es desde la perspectiva de las blancas 
o de las negras, que es el primer índice) */
extern int rank[2][64];

extern int OtherSide[2]; // OtherSide[0] = 1, OtherSide[1] = 0

/* Cambio en el número de casilla por avanzar un peón.
Si es Blancas, se incrementa 8 y si es Negras, se decrementa 8. */
extern int ForwardSquare[2];
/* Cambio en el número de casilla por avanzar un peón 2 escaques (primer movimiento de peón).
Si es Blancas, se incrementa 16 y si es Negras, se decrementa 16. */
extern int Double[2];
/* Cambio en el número de casilla por una captura de peón a la izquierda.
Si es Blancas, se incrementa 7 y si es Negras, se decrementa 9. */
extern int Left[2];
/* Cambio en el número de casilla por una captura de peón a la derecha.
Si es Blancas, se incrementa 9 y si es Negras, se decrementa 7. */
extern int Right[2];

//init.cpp
void InitBoard();
void SetTables();
void NewPosition();
void Alg(int a,int b);
void Algebraic(int a);
void SetMoves();

//search.cpp
void think();
int Search(int alpha, int beta, int depth);
int CaptureSearch(int alpha,int beta);
int ReCaptureSearch(int,const int);
int reps2();
void Sort(const int from);
void CheckUp();

//gen.cpp
void Gen();
void GenEp();
void GenCastle();
void GenPawn(const int x);
void GenKnight(const int sq);
void GenBishop(const int x,const int dir);
void GenRook(const int x,const int dir);
void GenQueen(const int x,const int dir);
void GenKing(const int x);
void AddMove(const int x,const int sq);
void AddMove(const int x,const int sq,const int score);
int GenRecapture(int);
void GenCaptures();
void CapPawn(const int x);
void CapKnight(const int sq);
void CapBishop(const int x,const int dir);
void CapRook(const int x,const int dir);
void CapQueen(const int x,const int dir);
void CapKing(const int sq);
void AddCapture(const int x,const int sq,const int score);

//attack.cpp
bool Attack(const int s,const int sq);
int LowestAttacker(const int s,const int x);

//update.cpp
void UpdatePawn(const int s,const int p,const int start,const int dest);
void UpdatePiece(const int s,const int p,const int start,const int dest);
void RemovePiece(const int s,const int p,const int sq);
void AddPiece(const int s,const int p,const int sq);
int MakeMove(const int,const int);
void TakeBack();
int MakeRecapture(const int,const int);
void UnMakeRecapture();

//eval.cpp
int Eval();

//hash.cpp
void RandomizeHash();
int Random(const int x);
void AddKey(const int,const int,const int);
U64 GetKey();
U64 GetLock();
void Free();
void FreeAllHash();
void AddHash(const int s, const move_data m);
bool LookUp(const int s);
int GetHashPercent();

void AddPawnHash(const int s1, const int s2, const int wq, const int wk, const int bq, const int bk);
int GetHashPawn0();
int GetHashPawn1();
int LookUpPawn();
void AddPawnKey(const int s,const int x);

//main.cpp
int main();
int GetTime();
char *MoveString(int from,int to,int promote);
void NewPosition();
int ParseMove(char *s);
void DisplayBoard();
int reps();

int GetHashDefence(const int s,const int half);
