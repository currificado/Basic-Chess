#include "globals.h"

int ReverseSquare[2] = {-8,8};

game *g;
/*

UpdatePiece updates the Hash Table key, the board and the table_score (the incremental
evaluation) whenever a piece moves.

*/
void UpdatePiece(const int s, const int p, const int start, const int dest)
// s es side, p es la pieza, start es la casilla origen y dest la casilla destino
{
	AddKey(s,p,start); // "deshace" el último movimiento (o sea, vuelve las claves al valor que tenían antes de que la pieza p estuviera en start)
	AddKey(s,p,dest); // "hace" que el último movimiento se refleje en las claves
	board[dest]=p; // en la casilla destino ahora está la pieza p
	color[dest]=s; // la pieza p es de color s
	board[start]=EMPTY; // la casilla origen se fija vacía
	color[start]=EMPTY; // la casilla de origen no está ocupada por ningún bando
	if(p==K) // si la pieza que se movió es el rey
		kingloc[s] = dest; // actualiza la posición del rey de s
}
/*

RemovePiece updates the Hash Table key, the board and the table_score (the incremental
evaluation) whenever a piece is removed.

*/
void RemovePiece(const int s, const int p, const int sq)
{
	AddKey(s,p,sq); // actualiza currenthash y currentlock
	board[sq]=EMPTY; // borra la pieza en la casilla pasada como parámetro
	color[sq]=EMPTY; // dicha casilla ahora no está ocupada por ningún bando
}
/*

AddPiece updates the Hash Table key, the board and the table_score (the incremental
evaluation) whenever a piece is added.

*/
void AddPiece(const int s, const int p, const int sq)
{
	AddKey(s,p,sq); // actualiza currenthash y currentlock
	board[sq]=p; // pone una pieza p en el escaque sq
	color[sq]=s; // el escaque sq ahora está ocupado por una pieza de bando s
}
/*

MakeMove updates the board whenever a move is made.
If the King moves two squares then it sees if castling is legal.
If a pawn moves and changes file s without making a capture, then its an en passant capture
and the captured pawn is removed.
If the move is a capture then the captured piece is removed from the board.
If castling permissions are effected then they are updated.
If a pawn moves to the last rank then its promoted. The pawn is removed and a queen is added.
If the move leaves the King in check (for example, if a pinned piece moved), then the move is taken back.

*/
int MakeMove(const int start, const int dest)
{// start es la casilla origen, dest es la casilla destino; retorna si hubo éxito o no al hacer la jugada
	if (abs(start - dest) ==2 && board[start] == K)// intento de enroque, ya que la pieza movida es rey y se movió lateralmente 2 escaques
	{
		if (Attack(xside,start)) // si la casilla de origen está siendo atacada por el bando contrario (o sea, si el rey está en jaque)
				return false; // retorna falso
		
		if(dest==G1) // si la CASILLA DESTINO es G1, o sea, si ENROQUE CORTO 0-0 DE LAS BLANCAS
		{
			if (Attack(xside,F1)) // si la casilla intermedia f1 está siendo atacada por el bando contrario
				return false; // retorna falso
			UpdatePiece(side,R,H1,F1); // si no, actualizo la posición de la torre de h1 a f1
		}
		else if(dest==C1) // si la CASILLA DESTINO es C1, o sea, si ENROQUE LARGO 0-0-0 DE LAS BLANCAS
		{
			if (Attack(xside,D1)) // si la casilla intermedia d1 está siendo atacada por el bando contrario
				return false; // retorna falso
			UpdatePiece(side,R,A1,D1); // si no, actualizo la posición de la torre de a1 a d1
		}
		else if(dest==G8) // si la CASILLA DESTINO es G8, o sea, si ENROQUE CORTO 0-0 DE LAS NEGRAS
		{
			if (Attack(xside,F8)) // si la casilla intermedia f8 está siendo atacada por el bando contrario
				return false; // retorna falso
			UpdatePiece(side,R,H8,F8); // si no, actualizo la posición de la torre de h8 a f8
		}
		else if(dest==C8) // si la CASILLA DESTINO es C8, o sea, si ENROQUE LARGO 0-0-0 DE LAS NEGRAS
		{	
			if (Attack(xside,D8)) // si la casilla intermedia d8 está siendo atacada por el bando contrario
				return false; // retorna falso
			UpdatePiece(side,R,A8,D8); // si no, actualiza la posición de la torre de a8 a d8
		}
	}/*
	    No se chequea que la casilla destino NO esté amenazada ya que en ese caso, 
	    tras el enroque, el rey estaría en jaque, y esto es algo que se ya controla más adelante.
	 */

	g = &game_list[hply]; // g es puntero al índice hply de game_list, o sea, *g representa ese nodo
	g->start = start; // define casilla origen 
	g->dest = dest; // define casilla destino
	g->capture = board[dest]; // si board[dest] == EMPTY, es que no es captura (o captura al paso); en caso contrario representa la pieza capturada 
	g->fifty = fifty; // define contador de las 50 jugadas
	g->hash = currentkey; // setea hash del nodo *g
	g->lock = currentlock; // setea lock del nodo *g

	ply++; // incrementa el número de ply de la variante actual
	hply++; // incrementa el número de ply global

	g = &game_list[hply]; // g ahora apunta al nodo siguiente de game_list
	// hace que este nodo "herede" los permisos de enroque del nodo anterior
	// si la jugada actual fue enroque, movimiento de rey o de una torre, se cambiará más adelante
	g->castle_q[0] = game_list[hply-1].castle_q[0];
	g->castle_q[1] = game_list[hply-1].castle_q[1];
	g->castle_k[0] = game_list[hply-1].castle_k[0];
	g->castle_k[1] = game_list[hply-1].castle_k[1];

	fifty++; // incrementa contador global de las 50 jugadas

	if (board[start] == P) // si fue movimiento de peón
	{
		fifty = 0; // reinicializa el contador de las 50 jugadas en 0
		if (board[dest] == EMPTY && col[start] != col[dest]) // si el peón cambió de columna y la casilla destino estaba vacía (fue captura al paso)
		{
			RemovePiece(xside,P,dest + ReverseSquare[side]); // borra el peón (que fue capturado al paso) del bando contrario xside
			//éste se encontrará en dest+8 si side=1 (captura al paso de negras) [el peón negro que capturó queda debajo del blanco capturado]
			//éste se encontrará en dest-8 si side=0 (captura al paso de blancas) [el peón blanco que capturó queda encima del peón negro capturado]
		}
	}

	if(board[dest]<6) // si la casilla destino está ocupada, o sea, si es captura
	{
		fifty = 0; // reinicializa el contador de las 50 jugadas en 0 
		RemovePiece(xside,board[dest],dest); // borra la pieza que había en la casilla destino
	}

	if (board[start]==P && (row[dest]==0 || row[dest]==7)) // si la jugada actual es promoción
	{
		RemovePiece(side,P,start); // borra el peón que había en octava fila (si es blancas) o primera fila (si es negras)
		AddPiece(side,Q,dest); // agrega la dama en la casilla destino
		g->promote = Q; // fija el atributo promote de *g en Q
	}
	else // si es una jugada cualquiera que no es promoción
	{
		g->promote = 0; // fija el atributo promote de *g en 0
		UpdatePiece(side,board[start],start,dest); // actualiza el tablero (board, color, etc)
	}

	if(dest == A1 || start == A1) // si la casilla de destino es a1 (se capturó la torre) o la de partida es a1 (movió la torre)
		g->castle_q[0] = 0; // las blancas no pueden enrocar largo
	else if(dest == H1 || start == H1) // si la casilla de destino es h1 (se capturó la torre) o la de partida es h1 (movió la torre)
		g->castle_k[0] = 0; // las blancas no puede enrocar corto
	else if(start == E1) // si la de casilla de origen es e1 (movió el rey)
	{
		g->castle_q[0] = 0; // las blancas no pueden enrocar largo
		g->castle_k[0] = 0; // ni tampoco corto
	}

	if(dest == A8 || start == A8) // si la casilla de destino es a8 (se capturó la torre) o la de origen es a8 (movió la torre)
		g->castle_q[1] = 0; // las negras no pueden enrocar largo
	else if(dest == H8 || start == H8) // si la casilla de destino es h8 (se capturó la torre) o la de partida es h8 (movió la torre)
		g->castle_k[1] = 0; // las negras no pueden enrocar corto
	else if(start == E8) // si la casilla de origen es e8 (movió el rey)
	{
		g->castle_q[1] = 0; // las negras no pueden enrocar largo
		g->castle_k[1] = 0; // ni tampoco corto
	}

	side ^= 1; // side  =  side xor 1 : si side=1 pasa a valer 0, si side=0 pasa a valer 1
	xside ^= 1;// xside = xside xor 1 : si xside=1 pasa a valer 0, si xside=0 pasa a valer 1
	if (Attack(side,kingloc[xside])) // SI EL REY DEL BANDO QUE ACABA DE REALIZAR LA JUGADA QUEDA EN JAQUE
	{
		TakeBack(); // revierte todos los cambios anteriores
		return false; // retorna falso
	}
	return true; // si todo ocurrió satisfactoriamente y llegó hasta aquí, retorna éxito
}
/*

TakeBack is the opposite of MakeMove.

*/
void TakeBack()
{
	side ^= 1; // invierte side
	xside ^= 1; // invierte xside
	ply--; // decrementa el número de ply de la variante actual
	hply--; // decrementa el número de ply global

	game *m = &game_list[hply]; // m es puntero al nodo anterior de game_list (ya que hply ya fue decrementado)
	int start = m->start; // toma la casilla de origen
	int dest = m->dest; // toma la casilla de destino

	fifty = m->fifty; // el contador de las 50 jugadas es el inmediatamente anterior

	/* si en la casilla destino hay un peón (lo que significa que en la última jugada se movió un peón), 
	   la columna de la casilla origen es diferente de la columna de la casilla destino,
	   y no hubo captura en la casilla destino, es que fue una CAPTURA AL PASO */
	if (board[dest]==P && m->capture == EMPTY && col[start] != col[dest])
	{
		AddPiece(xside,P,dest + ReverseSquare[side]); // agrega un peón del bando contrario una casilla inmediatamente anterior de la destino
	}
	if(game_list[hply+1].promote == Q) // si es promoción
	{
		AddPiece(side,P,start); // agrega un peón en séptima/segunda fila
		RemovePiece(side,board[dest],dest); // borra la dama de la casilla destino 
	}
	else // si es cualquier otra jugada
	{
		UpdatePiece(side,board[dest],dest,start); // actualiza el tablero deshaciendo la última jugada
	}
	if (m->capture != EMPTY) // si fue captura
	{
		AddPiece(xside,m->capture,dest); // agrega la pieza capturada del bando contrario
	}
	if (abs(start - dest) == 2 && board[start] == K) // si fue movimiento lateral del rey de dos escaques (enroque)
	{
		if(dest==G1)
			UpdatePiece(side,R,F1,H1); // deshace enroque corto de las blancas
		else if(dest==C1)
			UpdatePiece(side,R,D1,A1); // deshace enroque largo de las blancas
		else if(dest==G8)
			UpdatePiece(side,R,F8,H8); // deshace enroque corto de las negras
		else if(dest==C8)
			UpdatePiece(side,R,D8,A8); // deshace enroque largo de las negras
	}
}
/*

MakeRecapture is simpler than MakeMove because there is no castling involved and it
doesn't include en passant capture and promotion.
It the capture is illegal it is taken back.

*/
int MakeRecapture(const int start, const int dest)
// esta función está pensada especialmente para la captura
{
	game_list[hply].start = start; // define casilla origen del nodo actual (último de game_list)
	game_list[hply].dest = dest; // define casilla destino del nodo actual (último de game_list)
	game_list[hply].capture = board[dest]; // la pieza capturada es la que había en la casilla destino
	ply++; // incrementa el número de ply de la variante actual
	hply++; // incrementa el número de ply global

	board[dest] = board[start]; // la pieza que hay en la casilla destino es la misma que había en la casilla origen
	color[dest] = color[start]; // por tanto su color es igual
	board[start] = EMPTY; // la casilla origen está ahora vacía
	color[start] = EMPTY; // en la casilla origen no hay pieza de ningún color

	if(board[dest]==K) // si la pieza que capturó fue el rey
		kingloc[side] = dest; // actualiza la posición del rey de side
  
	side ^= 1; // invierte side
	xside ^= 1; // invierte xside
	if (Attack(side,kingloc[xside])) // si el bando contrario (que ahora es side) da jaque
	{
		UnMakeRecapture(); // deshace la captura
		return false; // retorna falso
	}
	return true; // si llega hasta aquí, la captura fue exitosa
}
/*

UnMakeRecapture is very similar to MakeRecapture.

*/
void UnMakeRecapture()
{
	side ^= 1; // invierte side, o sea, side pasa a ser el que recién movió
	xside ^= 1; // invierte xside
	ply--; // decrementa el número de ply de la variante actual
	hply--; // decrementa el número de ply global

	int start = game_list[hply].start; // start es la casilla inicial del movimiento anterior
	int dest = game_list[hply].dest; // dest es la casilla final del movimiento anterior

	board[start] = board[dest]; // en la casilla inicial, está la pieza que ahora está en la casilla final
	color[start] = color[dest]; // el color de la pieza que ocupa la casilla inicial es el mismo que el de la pieza que ocupa la casilla final (porque son la misma pieza)
	board[dest] = game_list[hply].capture; // restauro la pieza que había en la casilla final
	color[dest] = xside; // es del color del bando contrario

	if(board[start]==K) // si el que había capturado era el rey
		kingloc[side] = start; // actualiza la posición de side
}

