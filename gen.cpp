#include "globals.h"


// Puntajes de capturas
// El índice es la pieza capturada
// Se usa MVV-LVA (Most Valuable Victim, Least Valuable Attacker)
int px[6] = {0,10,20,30,40,0};
int nx[6] = {-3,7,17,27,37,0};
int bx[6] = {-3,7,17,27,37,0};
int rx[6] = {-5,5,15,25,35,0};
int qx[6] = {-9,1,11,21,31,0};
int kx[6] = {0,10,20,30,40,0};

/*
Cambio en el número de casilla por avanzar un peón.
Si es Blancas, se incrementa 8 y si es Negras, se decrementa 8.
*/
int ForwardSquare[2] = {8,-8};

/*
Cambio en el número de casilla por avanzar un peón 2 escaques (primer movimiento de peón).
Si es Blancas, se incrementa 16 y si es Negras, se decrementa 16. 
*/
int Double[2] = {16,-16};

/*
Cambio en el número de casilla por una captura de peón a la izquierda.
Si es Blancas, se incrementa 7 y si es Negras, se decrementa 9. 
*/
int Left[2] = {7,-9};

/*
Cambio en el número de casilla por una captura de peón a la derecha.
Si es Blancas, se incrementa 9 y si es Negras, se decrementa 7. 
*/
int Right[2] = {9,-7};

int OtherSide[2] = {1,0};

// puntero a la última jugada de move_list (movimientos a analizar en la variante actual)
int mc;

/*

Gen sees if an en passant capture or castling is possible.
It then loops through the board searching for pieces of one 
side and generates moves for them.

*/
void Gen()
{
	/* Esta función genera todos los posibles movimientos de una ply y
	   los guarda en move_list a partir de move_list[first_move[ply]] */

	mc = first_move[ply]; // mc es ahora puntero a la primera jugada de la ply en la move_list

	GenEp(); // agrega movimientos de capturas al paso si corresponde

	GenCastle(); // agrega posible movimientos de enroque si corresponde

	for(int x = 0;x < 64;x++)
	{
		if(color[x] == side)
		{
			switch(board[x])
			{
			case P: // movimientos de peón
				GenPawn(x);
				break;
			case N: // movimientos de caballo
				GenKnight(x);
				break;
			case B: // movimientos de alfil
				GenBishop(x,NE);
				GenBishop(x,SE);
				GenBishop(x,SW);
				GenBishop(x,NW);
				break;
			case R: // movimientos de torre
				GenRook(x,NORTH);
				GenRook(x,EAST);
				GenRook(x,SOUTH);
				GenRook(x,WEST);
				break;
			case Q: // movimientos de dama
				GenQueen(x,NE);
				GenQueen(x,SE);
				GenQueen(x,SW);
				GenQueen(x,NW);
				GenQueen(x,NORTH);
				GenQueen(x,EAST);
				GenQueen(x,SOUTH);
				GenQueen(x,WEST);
				break;
			case K: // movimientos de rey
				GenKing(x);
				break;
			default:
				break;
			}
		}
	}
	/* en este instante, mc apunta al primer índice libre en move_list
	   por tanto hago que first_move[ply+1] sea mc (o sea, las jugadas
	   de la ply+1 arrancan a partir de mc) */
	first_move[ply + 1] = mc;
}
/*

GenEp looks at the last move played and sees if it is a double pawn move.
If so, it sees if there is an opponent pawn next to it.
If there is, it adds the en passant capture to the move list.
Note that sometimes two en passant captures may be possible.

*/
void GenEp()
{
	int ep = game_list[hply - 1].dest; // obtiene la casilla destino de la última jugada, la llama ep

	/* si esa casilla está ocupada por un peón y el último movimiento fue un avance de dos escaques,
	   o sea, |posición final - posición inicial| = 16, me fijo si hay peones del rival que puedan
	   capturar al paso */
	if(board[ep] == 0 && abs(game_list[hply - 1].start - ep) == 16)
	{
		/* si ep no está en la columna 1 y
		   la casilla ep-1 es está ocupada por el bando del que toca jugar y
		   además en esa casilla hay un peón */
		if(col[ep] > 0 && color[ep-1]==side && board[ep-1]==P)
		{
			// agrega a la move_list la captura al paso a la derecha
			AddCapture(ep-1,ep+ForwardSquare[side],10);
		}
		/* si ep no está en la columna 8 y
		   la casilla ep+1 es está ocupada por el bando del que toca jugar y
		   además en esa casilla hay un peón */
		if(col[ep] < 7 && color[ep+1]==side && board[ep+1]==P)
		{
			// agrega a la move_list la captura al paso a la izquierda
			AddCapture(ep+1,ep+ForwardSquare[side],10);
		}
	}
}
/*

GenCastle generates a castling move if the King and Rook haven't moved and
there are no pieces in the way. Attacked squares are looked at in MakeMove. 

*/
void GenCastle()
{
	if(side==0) // si mueven las Blancas
	{
		if(game_list[hply].castle_k[side]) // si se puede enrocar en el flanco de rey
		{
			if(board[F1] == EMPTY && board[G1] == EMPTY)
			{
				AddMove(E1,G1);
			}
		}
		if(game_list[hply].castle_q[side]) // si se puede enrocar en el flanco de dama
		{
			if(board[B1] == EMPTY && board[C1] == EMPTY && board[D1] == EMPTY)
			{
				AddMove(E1,C1);
			}
		}
	}
	else // si mueven las Negras
	{
		if(game_list[hply].castle_k[side]) // si se puede enrocar en el flanco de rey
		{
			if(board[F8] == EMPTY && board[G8] == EMPTY)
			{
				AddMove(E8,G8);
			}
		}
		if(game_list[hply].castle_q[side]) // si se puede enrocar en el flanco de dama
		{
			if(board[B8] == EMPTY && board[C8] == EMPTY && board[D8] == EMPTY)
			{
				AddMove(E8,C8);
			}
		}
	}
}
/*

GenPawn generates single and double pawn moves and pawn 
captures for a pawn. 

*/
void GenPawn(const int x)
{//x es la casilla donde hay ubicado un peón
	if(board[x+ForwardSquare[side]] == EMPTY) //si la casilla de adelante está vacía
	{
		AddMove(x,x + ForwardSquare[side]); //agrego el avance de peón en un escaque
		if(rank[side][x]==1 && board[x + Double[side]] == EMPTY)
		//si x está en la columna 2 y la casilla 2 lugares más adelante TAMBIÉN está vacía
		{
			AddMove(x,x + Double[side]); //agrego el avance de peón en dos escaques
		}
	}
	if(col[x] > 0 && color[x + Left[side]] == OtherSide[side])
	//si x no está en la columna 1 y en la casilla NW hay una pieza del otro bando
	{
		AddCapture(x,x + Left[side],px[board[x + Left[side]]]);
	}
	if(col[x] < 7 && color[x + Right[side]] == OtherSide[side])
	//si x no está en la columna 8 y en la casilla NE hay una pieza del otro bando
	{
		AddCapture(x,x + Right[side],px[board[x + Right[side]]]);
	}
}
/*

GenKnight generates knight moves and captures by using the 
knight_moves look up table created in init.cpp. 

*/
void GenKnight(const int sq)
//sq es la casilla donde está ubicado el caballo
{
	int k = 0; //índice para recorrer el array de jugadas de caballo
	int sq2 = knight_moves[sq][k++]; //sq2 es una de las casillas destino
	while(sq2 > -1)
	{
		if(color[sq2] == EMPTY) //si la casilla destino está vacía
			AddMove(sq,sq2);
		else if(color[sq2] == xside) //si la casilla destino está ocupada por una pieza del bando contrario
			AddCapture(sq,sq2,nx[board[sq2]]);
		sq2 = knight_moves[sq][k++];
	}
}
/*

GenBishop generates bishop moves and 
captures for each diagonal. 

*/
void GenBishop(const int x,const int dir)
//x es la casilla donde hay ubicado un alfil, dir es la dirección en la se generarán las jugadas
{
	int sq = qrb_moves[x][dir]; // se para en la primera casilla en la dirección indicada
	while(sq > -1)
	{
		if(color[sq] != EMPTY) // si la casilla está ocupada
		{
			if(color[sq] == xside) // si en esa casilla hay una pieza del bando contrario
				AddCapture(x,sq,bx[board[sq]]); // agrega movimiento de captura
			break; // si el paso de la pieza está bloqueado, retorna inmediatamente (no continúa)
		}
		AddMove(x,sq); // esto corresponde si sq está vacía: agrega un movimiento común (sin ser captura)
		sq = qrb_moves[sq][dir]; // actutaliza sq, siendo ahora la nueva casilla a analizar en la dirección dir
	}
}
/*

GenRook generates straight moves and captures 
for each rank and file. 

*/
void GenRook(const int x,const int dir)
{
	int sq = qrb_moves[x][dir];
	while(sq > -1)
	{
		if(color[sq] != EMPTY)
		{
			if(color[sq] == xside)
			{
				AddCapture(x,sq,rx[board[sq]]); // lo único que cambia es el score: rx[board[sq]]
			}
			break;
		}
		AddMove(x,sq);
		sq = qrb_moves[sq][dir];
	}
}
/*

GenQueen generates queen moves and captures 
for line. 

*/
void GenQueen(const int x,const int dir)
{
	int sq = qrb_moves[x][dir];
	while(sq > -1)
	{
		if(color[sq] != EMPTY)
		{
			if(color[sq] == xside)
			{
				AddCapture(x,sq,qx[board[sq]]); // lo único que cambia es el score: qx[board[sq]]
			}
			break;
		}
		AddMove(x,sq);
		sq = qrb_moves[sq][dir];
	}
}
/*

GenKing generates king moves and captures by using the 
king_moves look up table created in init.cpp. 

*/
void GenKing(const int x)
{
	int k = 0;
	int sq = king_moves[x][k++];

	while(sq > -1)
	{
		if(color[sq] == EMPTY)
			AddMove(x,sq);
		else if(color[sq] == xside)
			AddCapture(x,sq,kx[board[sq]]);
		sq = king_moves[x][k++];
	}
}
/*

AddMove adds the start and dest squares of a move to the movelist. 
The score is the history value.

*/
void AddMove(const int x, const int sq)
{ // x es la casilla origen, sq es la casilla destino
	move_list[mc].start = x;
	move_list[mc].dest = sq;
	move_list[mc].score = history[x][sq];
	mc++;
}
/*

AddCapture adds the start and dest squares of a move to the 
movelist. 
CAPTURE_SCORE is added to the score so that captures will be 
looked at first.
The score is also added and will be used in move ordering.

*/
void AddCapture(const int x, const int sq, const int score)
{ // x es la casilla origen, sq es la casilla destino, score es el puntaje asociado
	move_list[mc].start = x;
	move_list[mc].dest = sq;
	move_list[mc].score = score + CAPTURE_SCORE;
	mc++;
}
/*

GenCaptures is very similar to Gen, except that only captures 
are being generated instead of all moves.

*/
void GenCaptures()
{
	first_move[ply + 1] = first_move[ply];
	mc = first_move[ply];

	for(int x = 0;x < 64;x++)
	{
		if(color[x] == side)
		{
			switch(board[x])
			{
			case P:
				CapPawn(x);
				break;
			case N:
				CapKnight(x);
				break;
			case B:
				CapBishop(x,NE);
				CapBishop(x,SE);
				CapBishop(x,SW);
				CapBishop(x,NW);
				break;
			case R:
				CapRook(x,EAST);
				CapRook(x,SOUTH);
				CapRook(x,WEST);
				CapRook(x,NORTH);
				break;
			case Q:
				CapQueen(x,NE);
				CapQueen(x,SE);
				CapQueen(x,SW);
				CapQueen(x,NW);
				CapQueen(x,EAST);
				CapQueen(x,SOUTH);
				CapQueen(x,WEST);
				CapQueen(x,NORTH);
				break;
			case K:
				CapKing(x);
				break;
			default:
				break;
			}
		}
	}
	first_move[ply + 1] = mc;
}
/*

CapPawn generates pawn captures.

*/
void CapPawn(const int x)
{
	if(col[x] > 0 && color[x + Left[side]] == OtherSide[side])
	{
		AddCapture(x,x + Left[side],px[board[x + Left[side]]]);
	}
	if(col[x] < 7 && color[x + Right[side]] == OtherSide[side])
	{
		AddCapture(x,x + Right[side],px[board[x + Right[side]]]);
	}
}
/*

CapKnight generates knight captures.

*/
void CapKnight(const int x)
{
	int k = 0;
	int sq = knight_moves[x][k++];
	while(sq > -1)
	{
		if(color[sq] == xside)
			AddCapture(x,sq,nx[board[sq]]);
		sq = knight_moves[x][k++];
	}
}
/*

CapBishop generates bishop captures.

*/
void CapBishop(const int x,const int dir)
{
	int sq = qrb_moves[x][dir];
	while(sq > -1)
	{
		if(color[sq] != EMPTY)
		{
			if(color[sq] == xside)
				AddCapture(x,sq,bx[board[sq]]);
			break;
		}
		sq = qrb_moves[sq][dir];
	}
}
/*

CapRook generates rook captures.

*/
void CapRook(const int x,const int dir)
{
int sq = qrb_moves[x][dir];
while(sq > -1)
{
	if(color[sq] != EMPTY)
	{
		if(color[sq] == xside)
		{
			AddCapture(x,sq,rx[board[sq]]);
		}
		break;
	}
	sq = qrb_moves[sq][dir];
}

}
/*

CapQueen generates queen captures.

*/
void CapQueen(const int x,const int dir)
{
int sq = qrb_moves[x][dir];
while(sq > -1)
{
	if(color[sq] != EMPTY)
	{
		if(color[sq] == xside)
		{
			AddCapture(x,sq,qx[board[sq]]);
		}
		break;
	}
	sq = qrb_moves[sq][dir];
}

}
/*

CapKing generates king captures.

*/
void CapKing(const int x)
{
	int k = 0;
	int sq = king_moves[x][k++];

	while(sq > -1)
	{
		if(color[sq] == xside)
			AddCapture(x,sq,kx[board[sq]]);
		sq = king_moves[x][k++];
	}
}
