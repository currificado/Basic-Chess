
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <inttypes.h>

#include <iostream>

#include <sys/timeb.h>

#include "globals.h"

void ShowHelp(); // procedimiento para desplegar la ayuda
void SetUp(); // inicialización del programa
void xboard(); // procedimiento análogo al main() para manejo del protocolo WinBoard/XBoard (también llamado CECP)

int board_color[64] = 
{
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1,
	1, 0, 1, 0, 1, 0, 1, 0,
	0, 1, 0, 1, 0, 1, 0, 1
}; // arreglo con los colores de los escaques

int LoadDiagram(char* file,int); // procedimiento para cargar una posición mediante un diagrama FEN
void CloseDiagram(); // procedimiento para cerrar el archivo diagrama_file (en caso de estar definido)

FILE *diagram_file; // archivo con el diagrama FEN
char fen_name[256]; // variable para almacenar el nombre de un archivo FEN

int flip = 0; // flip = 1 cuando haya que mostrar el tablero invertido, flip = 0 si debe mostrarse normal

int computer_side; // computer_side = 0 si el motor es las blancas, computer_side = 1 si el motor es las negras, computer_side = 6 si no es blancas ni negras

int fixed_time; // "booleano" para cuando hay un tiempo máximo personalizado
int fixed_depth; // "booleano" para cuando hay una profundidad máxima personalizada
int max_time; // tiempo máximo
int start_time; // tiempo inicial
int stop_time; // tiempo de parada
int max_depth; // profundidad máxima
int turn = 0; // contador de turnos (plys jugadas)

void PrintResult(); // procedimiento que chequea si se llegó al final de la partida
void NewGame(); // inicialización de un nuevo juego
void SetMaterial(); // carga el material para ambos bandos

extern int move_start,move_dest;

int main()
{
	// despliega texto inicial con el nombre de la programa, versión, etc.
	printf("\n");
	printf("Bills Basic Chess Engine\n");
	printf("Version 1.01, 15/1/19\n");
	printf("Copyright 2019 Bill Jordan\n");
	printf("\n");
	printf("\"help\" displays a list of commands.\n");
	printf("\n");

	char s[256]; // variable que contendrá el string introducido por el usuario
	char sText[256]; // texto ingresado por el usuario como nombre del archivo FEN
	char sFen[256]; // va a ser el texto anterior más la extensión ".fen"

	int m; // m=1 indicará éxito en el parsing de una jugada (texto no reconocido como ningún comando)
	int t; // tiempo de procesamiento de cada jugada
	// int lookup;

	double nps; // nodos por segundo

	fixed_time = 0; // pone en 0 la bandera de tiempo máximo personalizado

	SetUp(); // inicialización del programa

	while(true)
	{
		if (side == computer_side) // si es turno del motor de ajedrez, realiza la jugada antes de desplegar el prompt
		{
			think(); // búsqueda en profundidad

			currentkey = GetKey(); // actualiza la clave de la posición actual
			currentlock = GetLock(); // actualiza el cerrojo de la posición actual
			//lookup = LookUp(side);
			LookUp(side);

			if(move_start != 0 || move_dest != 0)
			{
				hash_start = move_start;
				hash_dest = move_dest;
				Alg(hash_start,hash_dest);printf("\n"); // muestra la jugada escogida por el motor de ajedrez
			}
			else // (move_start==0 && move_dest==0)
			{	
				printf("(no legal moves)\n");
				computer_side = EMPTY; // hace que la computadora no sea blancas ni negras
				DisplayBoard(); // muestra el tablero
				Gen(); // genera todos los movimientos posibles
				continue;
			} 

			printf("\n hash %d ",hashpositions[0]); // 0, ya que hashpositions[0] nunca se incrementa
			printf(" hash %d ",hashpositions[1]); // 0, ya que hashpositions[1] nunca se incrementa
			printf(" collisions %" PRIu64 " " ,collisions); // 0, ya que la variable collisions nunca se usa
			printf("\n");
			collisions = 0;

			printf("Computer's move: %s\n", MoveString(hash_start,hash_dest,0));printf("\n");
			MakeMove(hash_start,hash_dest);

			SetMaterial();

			t = GetTime() - start_time;
			printf("\nTime: %d ms\n", t);
			if(t>0)
				nps = (double)nodes / (double)t;
			else
				nps=0;
			nps *= 1000.0;

			printf("\nNodes per second: %d\n", (int)nps);
			ply = 0;

			first_move[0] = 0;
			Gen();
			PrintResult();
			printf(" turn "); printf("%d\n",turn++);
			DisplayBoard();
			continue;
		}

		printf("Enter move or command> "); // despliega el prompt y pasa a leer el comando de STDIN

		if (scanf("%s", s) == EOF)
			return 0;

		if (!strcmp(s, "d")) // si es `d` (de "DISPLAY")
		{
			DisplayBoard(); // muestra el tablero
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "f")) // si es `f` (de "FLIP")
		{
			flip = 1 - flip; // invierte flip: si flip = 0, pasa a valer 1; si flip = 1, pasa a valer 0
			DisplayBoard(); // muestra el tablero
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "go")) // si es `GO`
		{
			computer_side = side; // hace que computer_side coincida con el bando al que toca mover
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "help")) // si es `HELP`
		{
			ShowHelp(); // despliega la ayuda
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "moves")) // si es `MOVES`
		{
			printf("Moves \n");
			for (int i = 0; i < first_move[1]; ++i) // recorre move_list hasta first_move[1] que es donde empiezan las jugadas de la siguiente ply
			{
				printf("%s",MoveString(move_list[i].start,move_list[i].dest,move_list[i].promote));
				printf("\n");
			}
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "new")) // si es `NEW`
		{
			NewGame(); // inicialización para un nuevo juego
			computer_side = EMPTY;
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "on") || !strcmp(s, "p")) // si es `on` o `p` (de "PLAY")
		{
			computer_side = side; // hace lo mismo que `go`
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "off"))  // si es `OFF` (para "apagar") el motor
		{
			computer_side = EMPTY; // hace que la computadora no sea blancas ni negras
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "quit")) // si es `QUIT`
		{
			printf("Program exiting\n"); // muestra "Program exiting"
			break; // sale del while, con lo cual se ejecutará "return 0" final de main()
		}
		if (!strcmp(s, "sb")) // si es `sb` de ("SET BOARD" from position)
		{
			sFen[0] = 0;
			//strcat(sFen,"c:\\bscp\\");
			scanf("%s", sText);
			strcat(sFen,sText);
			strcat(sFen,".fen");
			LoadDiagram(sFen,1);
			continue;
		}
		if (!strcmp(s, "sd")) // si es `sd` (de "SET DEPTH")
		{
			scanf("%d", &max_depth); // lee max_depth
			max_time = 1 << 25; // configura un tiempo máximo gigante (ya que el límite será la profundidad ingresada por el usuario)
			fixed_depth = 1; // pone en 1 la bandera de profundidad personalizada
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "st")) // si es `st` (de "SET TIME")
		{
			scanf("%d", &max_time); // lee max_time
			max_time *= 1000; // multiplica por 1000 (ya que el usuario ingresó segundos pero el programa usa la variable en milisegundos)
			max_depth = MAX_PLY; // configura la profundidad como el máximo posible (ya que el límite será el tiempo fijo ingresado por el usuario)
			fixed_time = 1; // pone en 1 la bandera de tiempo máximo personalizado
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "sw")) // si es `sw` (de "SWITCH")
		{
			side = 1-side; // invierte side
			xside = 1-xside; // invierte xside
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "undo")) // si es `undo`
		{
			if (!hply) // si hply = 0, no precisa deshacer nada
				continue;
			computer_side = EMPTY; // hace que la computadora no sea blancas ni negras
			turn--; // decrementa el contador de turnos
			TakeBack(); // revierte la última jugada
			ply = 0; // pone el contador de ply en 0 (el de la variante en curso, no el global)
			if(first_move[0] != 0)
				first_move[0] = 0; // indica que las jugadas de la ply 0 empiezan en índice 0 de move_list
			Gen(); // genera todos los movimientos posibles
			continue; // continúa a la siguiente iteración
		}
		if (!strcmp(s, "xboard"))
		{
			xboard();
			break;
		}

			ply = 0;
			first_move[0] = 0;
			Gen();
			m = ParseMove(s);
			if (m == -1 || !MakeMove(move_list[m].start,move_list[m].dest))
			{
				printf("Illegal move. \n");
				printf(s);printf(" \n");
				MoveString(move_list[m].start,move_list[m].dest,move_list[m].promote);
				if (m == -1)printf(" m = -1 \n");
			}
			if(game_list[hply].promote >0 && (row[move_list[m].dest]==0 || row[move_list[m].dest]==7))
			{
				RemovePiece(xside,Q,move_list[m].dest);
				if(s[4]=='n' || s[4]=='N')
					AddPiece(xside,N,move_list[m].dest);
				else if(s[4]=='b' || s[4]=='B')
					AddPiece(xside,B,move_list[m].dest);
				else if(s[4]=='r' || s[4]=='r')
					AddPiece(xside,R,move_list[m].dest);
				else AddPiece(xside,Q,move_list[m].dest);
			}
	}
	Free(); // libera la memoria alojada para las tablas de hash
	return 0;
}

int ParseMove(char *s)
{
	int start, dest, i;

	if (s[0] < 'a' || s[0] > 'h' ||
			s[1] < '0' || s[1] > '9' ||
			s[2] < 'a' || s[2] > 'h' ||
			s[3] < '0' || s[3] > '9')
		return -1;

	start = s[0] - 'a';
	start += ((s[1] - '0') - 1)*8;
	dest = s[2] - 'a';
	dest += ((s[3] - '0') - 1)*8;

	for (i = 0; i < first_move[1]; ++i)
		if (move_list[i].start == start && move_list[i].dest == dest)
		{
			return i;
		}
	return -1;
}

void DisplaySquare(int colors, char piece)
{
	attron(COLOR_PAIR(colors));
		if (piece == ' ')
			printw("  ");
		else
			printw(" %c", piece);
	attroff(COLOR_PAIR(colors));
}

/* 
DisplayBoard() displays the board using the curses library
*/
void DisplayBoard()
{
	int i;

	initscr();
	scrollok( stdscr, TRUE );
	start_color();
	init_pair(MTY_WHI_SQR,COLOR_WHITE,COLOR_WHITE);
	init_pair(MTY_BLK_SQR,COLOR_BLACK,COLOR_BLACK);
	init_pair(WHI_SQR_W,COLOR_YELLOW,COLOR_WHITE);
	init_pair(WHI_SQR_B,COLOR_BLUE,COLOR_WHITE);
	init_pair(BLK_SQR_W,COLOR_YELLOW,COLOR_BLACK);
	init_pair(BLK_SQR_B,COLOR_BLUE,COLOR_BLACK);

	for (int j = 0; j < 64; ++j)
	{
		if(flip==0)
		{
			i = Flip[j];
			if (j % 8 == 0)
				printw("\n%d ", row[i]+1);
		}
		else if (flip==1)
		{
			i = 63-Flip[j];
			if (j % 8 == 0)
				printw("\n%d ", row[j]+1);
		}
		switch (color[i])
		{
			case EMPTY:
				if(board_color[i]==0)
					DisplaySquare(MTY_WHI_SQR, ' ');
				else
					DisplaySquare(MTY_BLK_SQR, ' ');
				break;
			case 0:
				if(board_color[i]==0)
					DisplaySquare(WHI_SQR_W, piece_char[board[i]]);
				else
					DisplaySquare(BLK_SQR_W, piece_char[board[i]]);
				break;
			case 1:
				if(board_color[i]==0)
					DisplaySquare (WHI_SQR_B, piece_char[board[i]] + ('a' - 'A'));
				else
					DisplaySquare (BLK_SQR_B, piece_char[board[i]] + ('a' - 'A'));
				break;
		}

	}
	if (flip==0)
		printw("\n\n   a b c d e f g h\n\n");
	else if(flip==1)
		printw("\n\n   h g f e d c b a\n\n");

	printw("Press 'q' to return to menu.\n");

	refresh();
	noecho();
	while (getch() !='q');
	erase();
	endwin();
}
/*
xboard() is a substitute for main() that is XBoard
and WinBoard compatible. 
*/

void xboard()
{
	int computer_side;
	char line[256], command[256];
	int m;
	//int post = 0;
	//int lookup;

	signal(SIGINT, SIG_IGN);
	printf("\n");
	NewGame();
	fixed_time = 0;
	computer_side = EMPTY;

	while(true) 
	{
		fflush(stdout);
		if (side == computer_side) 
		{
			think();
			SetMaterial();
			Gen();
			currentkey = GetKey();
			currentlock = GetLock();
			//lookup = LookUp(side);
			LookUp(side);

			if(move_start != 0 || move_dest != 0)
			{
				hash_start = move_start;
				hash_dest = move_dest;
			}
			else
				printf(" lookup=0 ");

			move_list[0].start = hash_start;
			move_list[0].dest = hash_dest;
			printf("move %s\n", MoveString(hash_start,hash_dest,0));

			MakeMove(hash_start,hash_dest);

			ply = 0;
			Gen();
			PrintResult();
			continue;
		}
		if (!fgets(line, 256, stdin))
			return;
		if (line[0] == '\n')
			continue;
		sscanf(line, "%s", command);
		if (!strcmp(command, "xboard"))
			continue;
		if (!strcmp(command, "new")) 
		{
			NewGame();
			computer_side = 1;
			continue;
		}
		if (!strcmp(command, "quit"))
			return;
		if (!strcmp(command, "force")) 
		{
			computer_side = EMPTY;
			continue;
		}
		if (!strcmp(command, "white")) 
		{
			side = 0;
			xside = 1;
			Gen();
			computer_side = 1;
			continue;
		}
		if (!strcmp(command, "black")) 
		{
			side = 1;
			xside = 0;
			Gen();
			computer_side = 0;
			continue;
		}
		if (!strcmp(command, "st")) 
		{
			sscanf(line, "st %d", &max_time);
			max_time *= 1000;
			max_depth = MAX_PLY;
			fixed_time = 1;
			continue;
		}
		if (!strcmp(command, "sd")) 
		{
			sscanf(line, "sd %d", &max_depth);
			max_time = 1 << 25;
			continue;
		}
		if (!strcmp(command, "time")) 
		{
			sscanf(line, "time %d", &max_time);
			if(max_time < 200)
				max_depth = 1;
			else
			{
				max_time /= 2;
				max_depth = MAX_PLY;
			}
			continue;
		}
		if (!strcmp(command, "otim")) 
		{
			continue;
		}
		if (!strcmp(command, "go")) 
		{
			computer_side = side;
			continue;
		}
		if (!strcmp(command, "random")) 
			continue;
		if (!strcmp(command, "level")) 
			continue;
		if (!strcmp(command, "hard")) 
			continue;
		if (!strcmp(command, "easy")) 
			continue;
		if (!strcmp(command, "hint")) 
		{
			think();
			currentkey = GetKey();
			currentlock = GetLock();
			//lookup = LookUp(side);
			LookUp(side);
			if(hash_start==0 && hash_dest==0)
				continue;
			printf("Hint: %s\n", MoveString(hash_start,hash_dest,0));
			continue;
		}
		if (!strcmp(command, "undo")) 
		{
			if (!hply)
				continue;
			TakeBack();
			ply = 0;
			Gen();
			continue;
		}
		if (!strcmp(command, "remove")) 
		{
			if (hply < 2)
				continue;
			TakeBack();
			TakeBack();
			ply = 0;
			Gen();
			continue;
		}
		if (!strcmp(command, "post")) 
		{
			//post = 2;
			continue;
		}
		if (!strcmp(command, "nopost")) 
		{
			//post = 0;
			continue;
		}

		first_move[0] = 0;
		Gen();

		m = ParseMove(line);
		if (m == -1 || !MakeMove(move_list[m].start,move_list[m].dest))
			printf("Error (unknown command): %s\n", command);
		else 
		{
			ply = 0;
			Gen();
			PrintResult();
		}
	}
}

void PrintResult()
{
	int i;
	int flag=0;

	SetMaterial();
	Gen();
	for (i = 0; i < first_move[1]; ++i)
		if (MakeMove(move_list[i].start,move_list[i].dest))
		{
			TakeBack();
			flag=1;
			break;
		}

	if(pawn_mat[0]==0 && pawn_mat[1]==0 && piece_mat[0]<=300 && piece_mat[1]<=300)
	{
		printf("1/2-1/2 {Stalemate}\n");

		NewGame();
		computer_side = EMPTY;
		return;
	}
	if (i == first_move[1] && flag==0)
	{
		Gen();
		printf(" end of game ");
		
		if (Attack(xside,kingloc[side]))
		{
			if (side == 0)
			{
				printf("0-1 {Black mates}\n");
			}
			else
			{
				printf("1-0 {White mates}\n");
			}
		}
		else
		{
			printf("1/2-1/2 {Stalemate}\n");
		}
		NewGame();
		computer_side = EMPTY;
	}
	else if (reps() >= 3)
	{
		printf("1/2-1/2 {Draw by repetition}\n");
		NewGame();
		computer_side = EMPTY;
	}
	else if (fifty >= 100)
	{
		printf("1/2-1/2 {Draw by fifty move rule}\n");
		NewGame();
		computer_side = EMPTY;
	}
	if(turn>300)
	{
		printf("1/2-1/2 {>300 moves}\n");
		NewGame();
		computer_side = EMPTY;
		return;
	}
}

int reps()
{
	int r = 0;

	for (int i = hply - 1; i >= hply-fifty; i-=2)
		if (game_list[i].hash == currentkey && game_list[i].lock == currentlock)
			++r;
	return r;
}

int LoadDiagram(char* file,int num)
{
	int x;
	static int count=1;
	char ts[200];

	diagram_file = fopen(file, "r");
	if (!diagram_file)
	{
		printf("Diagram missing.\n");
		return -1;
	}

	strcpy(fen_name,file);

	for(x=0;x<num;x++)
	{
		fgets(ts, 256, diagram_file);
		if(!ts) break;
	}

	for(x=0;x<64;x++)
	{
		board[x]=EMPTY;
		color[x]=EMPTY;
	}
	int c=0,i=0,j;

	while(ts)
	{
		if(ts[c]>='0' && ts[c]<='8')
			i += ts[c]-48;
		if(ts[c]=='\\')
			continue;
		j=Flip[i];

		switch(ts[c])
		{
			case 'K': board[j]=K; color[j]=0;i++;
			kingloc[0]=j;break;
			case 'Q': board[j]=Q;color[j]=0;i++;break;
			case 'R': board[j]=R; color[j]=0;i++;break;
			case 'B': board[j]=B; color[j]=0;i++;break;
			case 'N': board[j]=N; color[j]=0;i++;break;
			case 'P': board[j]=P; color[j]=0;i++;break;
			case 'k': board[j]=K; color[j]=1;i++;

					kingloc[1]=j;break;
			case 'q': board[j]=Q;color[j]=1;i++;break;
			case 'r': board[j]=R; color[j]=1;i++;break;
			case 'b': board[j]=B; color[j]=1;i++;break;
			case 'n': board[j]=N; color[j]=1;i++;break;
			case 'p': board[j]=P; color[j]=1;i++;break;
		}
		c++;
		if(ts[c]==' ')
		break;
		if(i>63)
		break;
	}
	if(ts[c]==' ' && ts[c+2]==' ')
	{
		if(ts[c+1]=='w')
		{
			side=0;xside=1;
		}
		if(ts[c+1]=='b')
		{
			side=1;xside=0;
		}
	}

	game_list[0].castle_q[0] = 0;
	game_list[0].castle_q[1] = 0;
	game_list[0].castle_k[0] = 0;
	game_list[0].castle_k[1] = 0;

	while(ts[c])
	{
		switch(ts[c])
		{
			case '-': break;
			case 'K':if(board[E1]==5 && color[E1]==0) game_list[0].castle_q[0] = 1;break;
			case 'Q':if(board[E1]==5 && color[E1]==0) game_list[0].castle_q[1] = 1;break;
			case 'k':if(board[E8]==5 && color[E8]==1) game_list[0].castle_k[0] = 1;break;
			case 'q':if(board[E8]==5 && color[E8]==1) game_list[0].castle_k[1] = 1;break;
			default:break;
		}
	c++;
	}

	CloseDiagram();
	DisplayBoard();
	NewPosition();
	Gen();
	printf(" diagram # %d \n",num+count);
	count++;
	if(side==0)
	printf("White to move\n");
	else
	printf("Black to move\n");
	printf(" %s \n",ts);
	return 0;
}

void CloseDiagram()
{
	if (diagram_file)
		fclose(diagram_file);
	diagram_file = NULL;
}

void ShowHelp()
{
	printf("d - Displays the board.\n");
	printf("f - Flips the board.\n");
	printf("go - Starts the engine.\n");
	printf("help - Displays help on the commands.\n");
	printf("moves - Displays of list of possible moves.\n");
	printf("new - Starts a new game .\n");
	printf("off - Turns the computer player off.\n");
	printf("on or p - The computer plays a move.\n");
	printf("sb - Loads a fen diagram.\n");
	printf("sd - Sets the search depth.\n");
	printf("st - Sets the time limit per move in seconds.\n");
	printf("sw - Switches sides.\n");
	printf("quit - Quits the program.\n");
	printf("undo - Takes back the last move.\n");
	printf("xboard - Starts xboard.\n");
}

void SetUp()
{
	RandomizeHash(); // crea las tablas de hash para blancas y negras
	FreeAllHash(); // inicializa en cero la cantidad de elementos que hay en las tablas de hash
	SetTables(); // llena las tablas necesarias para evaluar una posición
	SetMoves(); // llena las tablas necesarias para los movimientos de las piezas
	InitBoard(); // inicializa el tablero y las variables globales para el comienzo del juego
	Gen(); // deja en move_list todos los movimientos posibles a partir de la posición inicial
	computer_side = EMPTY; // 
	max_time = 1 << 25; // define el tiempo máximo como 2^25 segundos = 33554432 segundos (aprox. 388 días)
	max_depth = 4; // define en 4 la profundidad máxima
}

void NewGame()
{
	InitBoard();
	first_move[0] = 0;
	turn = 0;
	fifty = 0;
	ply = 0;
	hply = 0;
	Gen();
}

void SetMaterial()
{
	pawn_mat[0]=0;
	pawn_mat[1]=0;
	piece_mat[0]=0;
	piece_mat[1]=0;
	for(int x=0;x<64;x++)
	{
		if(board[x]<6)
		{
			if(board[x]==5)
				kingloc[color[x]] = x;
			if(board[x]==0)
				pawn_mat[color[x]] += 100;
			else
				piece_mat[color[x]] += piece_value[board[x]];
		}
	}
}

int GetTime()
{
	struct timeb timebuffer;
	ftime(&timebuffer);
	return (timebuffer.time * 1000) + timebuffer.millitm;
}

char *MoveString(int start,int dest,int promote)
{
	static char str[6];

	char c;

	if (promote > 0) {
		switch (promote) {
			case N:
				c = 'n';
				break;
			case B:
				c = 'b';
				break;
			case R:
				c = 'r';
				break;
			default:
				c = 'q';
				break;
		}
		sprintf(str, "%c%d%c%d%c",
				col[start] + 'a',
				row[start] + 1,
				col[dest] + 'a',
				row[dest] + 1,
				c);
	}
	else
		sprintf(str, "%c%d%c%d",
				col[start] + 'a',
				row[start] + 1,
				col[dest] + 'a',
				row[dest] + 1);
	return str;
}
