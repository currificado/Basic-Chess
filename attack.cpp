#include "globals.h"

int LineCheck(const int s,const int sq,const int d,const int p);
bool LineCheck2(const int s,const int sq,const int d,const int p1,const int p2);
/* 

Attack returns true if one side attacks a given square and false if it doesn't.
It is used to tell if a King is in check, but can have other uses.

*/
bool Attack(const int s,const int x) // s es Blancas (=0) o Negras (=1); x es la casilla atacada (o no)
{
	if(s==0) // s es Blancas
	{
		if(row[x]>1) // si x está en la fila 3 o superior
		{
			if(col[x]<7 && color[x-7] == s && board[x-7] == 0)
			{ // si x no está en la columna 8 y la casilla de abajo a la derecha está ocupada por un peón blanco
				return true;
			}
			if(col[x]>0 && color[x-9] == s && board[x-9] == 0)
			{ // si x no está en la columna 1 y en la casilla de abajo a la izquierda hay un peón blanco
				return true;
			}
		}
	}
	else if(row[x]<6) // s es Negras y x está en la fila 5 o inferior
	{
		if(col[x]>0 && color[x+7] == s && board[x+7] == 0)
		{ // si x no está en la columna 1 y la casilla de arriba a la izquierda está ocupada por un peón negro
			return true;
		}
		if(col[x]<7 && color[x+9] == s && board[x+9] == 0)
		{ // si x no está en la columna 8 la casilla de arriba a la derecha está ocupada por un peón negro
			return true;
		}
	}
	// ahora que se fijó en las amenazas de peón, se fija en las de las otras piezas empezando por el caballo
	int k = 0;
	int sq = knight_moves[x][k]; // knight_moves[x] tiene la lista de casillas desde donde un caballo podría saltar a x
	
	while(sq > -1) // recorre la lista anterior
	{
		if(color[sq] == s && board[sq]==N) // si esa casilla está ocupada por una pieza de color s y además es un caballo
			return true;
		k++;
		sq = knight_moves[x][k];
	}
	
	if(LineCheck2(s,x,NE,B,Q)) return true; // amenaza de alfil o dama en dirección NE
	if(LineCheck2(s,x,NW,B,Q)) return true; // amenaza de alfil o dama en dirección NW
	if(LineCheck2(s,x,SW,B,Q)) return true; // amenaza de alfil o dama en dirección SW
	if(LineCheck2(s,x,SE,B,Q)) return true; // amenaza de alfil o dama en dirección SE

	if(LineCheck2(s,x,NORTH,R,Q)) return true; // amenaza de torre o dama en dirección N
	if(LineCheck2(s,x,SOUTH,R,Q)) return true; // amenaza de torre o dama en dirección S
	if(LineCheck2(s,x,EAST,R,Q)) return true; // amenaza de torre o dama en dirección E
	if(LineCheck2(s,x,WEST,R,Q)) return true; // amenaza de torre o dama en dirección W

	// si |columna de x - columna de la ubicación del rey de s| < 2 o 
	// |fila de x - fila de la ubicación del rey de s| < 2 quiere decir que x está en una 
	// casilla adyacente a la rey de s y por tanto es una casilla amenazada
	if(abs(col[x] - col[kingloc[s]])<2 && abs(row[x] - row[kingloc[s]])<2)
	{
		return true;
	}
	return false;
}
/*
 
LowestAttacker is similar to Attack. It returns the square the weakest attacker of the given side and given square.
It returns -1 if there are no attackers.
It is used to find the next piece that will recapture, but can have other uses.

*/
// esta es una función especular de Attack(), pero retorna la casilla de la pieza más débil que amenaza x
// por tanto, empieza chequeando peones, luego caballo, luego alfil, luego torre, luego dama y por último rey
int LowestAttacker(const int s,const int x)
{
	if(s==0)
	{
		if(row[x]>1)
		{
			if(col[x]<7 && color[x-7] == s && board[x-7] == 0)
			{
				return x-7;
			}
			if(col[x]>0 && color[x-9] == s && board[x-9] == 0)
			{
				return x-9;
			}
		}
	}
	else if(row[x]<6)
	{
		if(col[x]>0 && color[x+7] == s && board[x+7] == 0)
		{
			return x+7;
		}
		if(col[x]<7 && color[x+9] == s && board[x+9] == 0)
		{
			return x+9;
		}
	}

	int k = 0;
	int sq = knight_moves[x][k];

	while(sq > -1)
	{
		if(color[sq] == s && board[sq]==N)
			return sq;
		k++;
		sq = knight_moves[x][k];
	}

	sq = LineCheck(s,x,NE,B); if(sq>-1) return sq;
	sq = LineCheck(s,x,NW,B); if(sq>-1) return sq;
	sq = LineCheck(s,x,SW,B); if(sq>-1) return sq;
	sq = LineCheck(s,x,SE,B); if(sq>-1) return sq;

	sq = LineCheck(s,x,NORTH,R); if(sq>-1) return sq;
	sq = LineCheck(s,x,SOUTH,R); if(sq>-1) return sq;
	sq = LineCheck(s,x,EAST,R); if(sq>-1) return sq;
	sq = LineCheck(s,x,WEST,R); if(sq>-1) return sq;

	sq = LineCheck(s,x,NORTH,Q); if(sq>-1) return sq;
	sq = LineCheck(s,x,SOUTH,Q); if(sq>-1) return sq;
	sq = LineCheck(s,x,EAST,Q); if(sq>-1) return sq;
	sq = LineCheck(s,x,WEST,Q); if(sq>-1) return sq;

	sq = LineCheck(s,x,NE,Q); if(sq>-1) return sq;
	sq = LineCheck(s,x,NW,Q); if(sq>-1) return sq;
	sq = LineCheck(s,x,SW,Q); if(sq>-1) return sq;
	sq = LineCheck(s,x,SE,Q); if(sq>-1) return sq;

	if(abs(col[x] - col[kingloc[s]])<2 && abs(row[x] - row[kingloc[s]])<2)
	{
		return kingloc[s];
	}
	return -1;
}
/*

LineCheck searches a line in direction d for the given piece of the given side.
It returns -1 if there are none.

*/
// s es side, sq es la casilla origen, d es la dirección, p es la pieza (B=alfil, R=torre o Q=dama)
int LineCheck(const int s, int sq, const int d, const int p)
{
	sq = qrb_moves[sq][d]; // hace que sq sea la casilla adyacente a la pasada como parámetro en la dirección d
	while(sq > -1)
	{
		if(color[sq] != EMPTY)
		{
			// si esa casilla está ocupada por una pieza de color s y además es la pieza p, retorna la casilla donde está ubicada dicha pieza
			if(color[sq] == s && board[sq] == p)
				return sq;
			break;
		}
		sq = qrb_moves[sq][d]; // actualiza sq
	}
	return -1; // si no se encontró ninguna pieza p en la dirección d que amenace sq, retorna -1
}
/*

LineCheck2 searches a line in direction d for the given pieces of the given side.
On diagonals it searches for bishops and queens, while on ranks or files it
searches for rooks and queens.
It returns -1 if there are none.

*/
// s es side, sq es la casilla origen, d es la dirección,
// p1 y p2 son las piezas que pueden amenazar en esa posición relativa a la casilla:
// si d es North, South, East, West -> R (torre) y Q (dama)
// si d es NE, NW, SE, SW -> B (alfil) y Q (dama)
bool LineCheck2(const int s, int sq, const int d, const int p1, const int p2)
{
	sq = qrb_moves[sq][d];
	while(sq > -1)
	{
		if(color[sq] != EMPTY)
		{
			if((board[sq] == p1 || board[sq] == p2) && color[sq] == s)
				return true;
			break;
		}
		sq = qrb_moves[sq][d];
	}
	return false;
}

