#include "globals.h"

#define ISOLATED 20

int EvalPawn(const int x);
int EvalRook(const int s, const int x);
bool Pawns(const int s, const int file);
bool Pawns2(const int s, const int xs, const int start);

//arreglos para definir un "bonus" a los peones que protegen al rey (sea flanco de dama o flanco de rey respectivamente)
int queenside_pawns[2],kingside_pawns[2];

const int queenside_defence[2][64]=
{
{ // índice 0 (o sea, blancas) -> asigna puntajes adicionales a los peones de a2, b2 y c2 (también a3, b3 y c3)
	0, 0, 0, 0, 0, 0, 0, 0,
	8,10, 8, 0, 0, 0, 0, 0,
	8, 6, 8, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
},
{ // índice 1 (o sea, negras) -> asigna puntajes adicionales a los peones de a7, b7 y c7 (también a6, b6 y c6)
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	8, 6, 8, 0, 0, 0, 0, 0,
	8,10, 8, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
}};

const int kingside_defence[2][64]=
{
{ // índice 0 (o sea, blancas) -> asigna puntajes adicionales a los peones de f2, g2 y h2 (también f3, g3 y h3)
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 8,10, 8,
	0, 0, 0, 0, 0, 8, 6, 8,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	8, 6, 8, 0, 0, 8, 8, 8,
	0, 0, 0, 0, 0, 0, 0, 0
},
{ // índice 1 (o sea, negras) -> asigna puntajes adicionales a los peones de f7, g7 y h7 (también f6, g6 y h6)
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 8, 6, 8,
	0, 0, 0, 0, 0, 8,10, 8,
	0, 0, 0, 0, 0, 0, 0, 0
}};
/*

Eval() is simple. Firstly it adds the square scores for each piece of both sides.
If the opponent does not have a queen it adds the endgame score for each king.
If the opponent has a queen it adds the pawn defence score for each king.
It turn returns the side to moves score minus the opponent's score.
There are plenty of things that could be added to the eval function.

*/
int Eval()
{
	int score[2] = {0,0};
	int queens[2] = {0,0};

	queenside_pawns[0] = 0;
	queenside_pawns[1] = 0;
	kingside_pawns[0] = 0;
	kingside_pawns[1] = 0;

	for(int x = 0;x < 64;x++)
	{
		if(color[x] != EMPTY) // si la casilla no está vacía
		{
			score[color[x]] += square_score[color[x]][board[x]][x]; // incrementa el score con el valor de la pieza en x
			if(board[x] == P) // en caso de ser peón
			{
				score[color[x]] += EvalPawn(x); // le suma algún puntaje adicional si es peón pasado o le resta en caso de ser peón aislado 
			}
			else if(board[x] == R) // si es torre
			{
				score[color[x]] += EvalRook(color[x],x); // le suma un puntaje adicional si está en columna abierta o semi abierta
			}
			else if(board[x] == Q) // si es dama
			{
				queens[color[x]] = 1; // marco que ese bando tiene dama
			}
		}
	}

	if(queens[1]==0) // si las negras no tienen dama
		score[0] += king_endgame[0][kingloc[0]]; // suma puntaje asociado a la ubicación del rey blanco en finales
	else // si las negras tienen dama
	{
		if(col[kingloc[0]]>3) // si el rey blanco está ubicado en el flanco de rey
			score[0] += kingside_pawns[0]; // suma puntos por los peones defensores de f2,g2,h2
		else
			score[0] += queenside_pawns[0]; //suma puntos por los peones defensores de a2,b2,c2
	}

	if(queens[0]==0) // si las blancas no tienen dama
		score[1] += king_endgame[1][kingloc[1]]; // suma puntaje asociado a la ubicación del rey negro en finales
	else // si las blancas tienen dama
	{
		if(col[kingloc[1]]>3) // si el rey negro está ubicado en el flanco de rey
			score[1] += kingside_pawns[1]; // suma puntos por los peones defensores de f7,g7,h7
		else
			score[1] += queenside_pawns[1]; //suma puntos por los peones defensores de a7,b7,c7
	}

	// el resultado de la evaluación es el puntaje del bando pasado como parámetro menos el del adversario
	return score[side] - score[xside];
}
/*

EvalPawn() evaluates each pawn and gives a bonus for passed pawns
and a minus for isolated pawns.

*/
int EvalPawn(const int x)
{
	int score = 0;
	int s = color[x];
	int xs = OtherSide[s];

	if(!Pawns2(s,xs,x)) // si no hay peones que se opongan a la coronación (o sea, es peón pasado)
	{
		score += passed[s][x]; // suma puntaje por ser peón pasado
	}
	// si no hay peones en la columna a la izquierda y no hay peones en la columna derecha
	if( (col[x]==0 || !Pawns(s,col[x]-1)) && (col[x]==7 || !Pawns(s,col[x]+1)) )
		score -= ISOLATED; // en x hay un peón aislado, por tanto decrementa el puntaje por ser aislado

	kingside_pawns[s] += kingside_defence[s][x]; // si es un peón del flanco de rey protector del rey, lo valora más
	queenside_pawns[s] += queenside_defence[s][x]; // si es un peón del flanco de dama protector del rey, lo valora más
	// estos puntajes adicionales no están en score pero se cargan en esta recorrida y son eventualmente sumados si el otro bando tiene dama
	return score;
}
/*

EvalRook() evaluates each rook and gives a bonus for being
on an open file or half-open file.
*/
int EvalRook(const int s, const int x)
{
	int score = 0;
	if(!Pawns(s,col[x])) // si no hay peones de s en la columna de x
	{
		score = 10; // incrementa 10 puntos (porque es como mínimo, columna semi abierta)
		if(!Pawns(OtherSide[s],col[x])) // si además no hay peones del bando contrario
			score += 10; // incrementa 10 puntos más por estar en columna abierta
	}
	return score;
}
/*

Pawns() searches for pawns on a file.
It is used to detect isolated pawns.

*/
bool Pawns(const int s, const int file)
{// s es side, file es la columna
	for(int x = file + 8;x < A8; x += 8) // parte de file+8 que es el escaque que tiene en frente
	{// incrementa de a 8 (para pasar de una fila a la siguiente)
		if(board[x]==P && color[x]==s) // si en x hay un peón de s
			return true; // retorna verdadero
	}
	return false; // retorna falso si no hay peones en esa columna
}
/*

Pawns2() searches for pawns on a file beyond a square.
It is used to detect passed pawns.

*/
/* Retorna VERDADERO si en x hay un peón de s que tiene peones del
   bando contrario (en frente o en las columnas adyacentes) o de su
   mismo bando (en frente) que se le oponen en su camino a la promoción.
   Dicho de otra forma, retorna FALSO si en x hay un peón pasado de s. */
bool Pawns2(const int s, const int xs, const int start)
{
	int x = start + ForwardSquare[s]; // avanza x un escaque hacia adelante
	while(x>H1 && x<A8) // mientras x no se salga del tablero
	{
		if(board[x]==P) // si hay un peón en x
			return true; // retorna verdadero, ya que existe un peón que se opone (sea del bando s o de xs)
		if(col[x]>0 && board[x-1]==P && color[x-1]==xs) // si en la columna de la izquierda (asumiendo que no es la columna a) hay un peón del bando contrario
			return true; // retorna verdadero, ya que hay un peón en una columna adyacente del bando xs que podría evitar la promoción
		if(col[x]<7 && board[x+1]==P && color[x+1]==xs) // si en la columna de la derecha (asumiendo que no es la columna h) hay un peón del bando contrario
			return true; // retorna verdadero, ya que hay un peón en una columna adyacente del bando xs que podría evitar la promoción
		x += ForwardSquare[s]; // avanza x otro escaque
	}
	return false; // si se terminó el tablero y no fue encontrado ningún peón opositor, retorna falso
}