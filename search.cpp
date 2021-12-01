#include <setjmp.h>

#include "globals.h"

void SetHashMove();
void DisplayPV(int i);

jmp_buf env; // env es por "environment", es donde se guarda el contexto de la llamada para luego ser usado con longjmp
bool stop_search; // bandera para determinar cuándo parar la búsqueda

int currentmax;

int move_start,move_dest;

/*

think() iterates until the maximum depth for the move is reached or until the allotted time
has run out.
After each iteration the principal variation is then displayed.

*/
void think()
{
	int x;

	stop_search = false;

	setjmp(env); // guarda el contexto (también es a donde se regresa luego de longjmp(env, 0);)
	if (stop_search) // si está levantada la bandera de dejar de buscar
	{
		while (ply)
			TakeBack(); // revierte todas las jugadas
		return;
	}
	if(fixed_time==0) // si no hay definido un tiempo fijo para cada jugada
	{
		if(game_list[hply-1].capture < 6 && game_list[hply-1].capture == board[game_list[hply-1].dest])
		// si la ply anterior fue peón x peón, caballo x caballo, alfil x alfil, torre x torre o dama x dama probablemente sea recaptura
		{
			max_time = max_time/2; // entonces fija el tiempo máximo en la mitad
		}
		else if (Attack(xside,kingloc[side])) // si el rey está en jaque
		{
			max_time = max_time/2; // entonces fija el tiempo máximo en la mitad
		}
	}
	start_time = GetTime(); // tiempo inicial es el instante actual
	stop_time = start_time + max_time; // tiempo de parada es el tiempo inicial + un tiempo máximo max_time

	ply = 0; // inicializa el contador de ply en 0
	nodes = 0; // inicializa cantidad de nodos en 0
	
	NewPosition(); // prepara el tablero para think()
	memset(history, 0, sizeof(history)); // limpia con todo ceros la matriz 64x64 history
	printf("ply      nodes  score  pv\n");

	for (int i = 1; i <= max_depth; ++i) // deepening iteration: hace la búsqueda con profundidad 1, profundidad 2, etc.
	{
		currentmax = i;
		//if(fixed_depth==0 && max_depth>1)
		if(fixed_time==1)
		{
			if(GetTime() >= start_time + max_time)
			{
				stop_search = true;
				return;
			}
		}
		else if(GetTime() >= start_time + max_time/4)
		{
			stop_search = true;
			return;
		}

		x = Search(-10000, 10000, i); // búsqueda en profundidad alfa-beta, x es el puntaje del resultado

		/* Despliega:
		   +Nro. de ply
		   +Puntaje
		   +Cantidad de centésimas de segundo que tardó
		   +Cantidad de nodos examinados
		*/
		printf("%d %d %d %d ", i, x, (GetTime() - start_time) / 10, nodes);

		if(LookUp(side))
		{
			DisplayPV(i); // muestra la variación principal
		}
		else
		{
			move_start = 0;
			move_dest = 0;
		}
		printf("\n");
		fflush(stdout);

		if (x > 9000 || x < -9000) // si el puntaje está por encima de 9000, es que encontró mate a nuestro favor
		{// si el puntaje está por debajo de -9000, es que encontró mate en nuestra contra
			break; // por tanto deja de buscar
		}
	}
}
/*

search is the main part of the search.
If the position is repeated we don't need to look any further.
If depth has run out, the capture search is done.
Every 4,000 positions approx the time is checked.
Moves are generated.
The moves are looped through in order of their score.
If a move is illegal (for example, if it attempts to move a pinned piece)
then it is skipped over.
If the move is check, we extend by one ply. This is done by not changing depth in the call to search.
If it has a score greater than zero, ply is one or the move number is less than 12
a normal search is done. This is done by subtracting 1 from depth. 
Otherwise we reduce by one ply. This is done by subtracting 2 from depth. 
The move is taken back.

If the score from search is greater than beta, a beta cutoff happens. No need to
search any moves at this depth.
Otherwise, if it is greater than alpha, alpha is changed. 

If there were no legal moves it is either checkmate or stalemate.

*/
/* alfa es el máximo puntaje encontrado hasta el momento para el que le toca mover,
   beta es el máximo puntaje encontrado hasta el momento para el bando opuesto, 
   depth es la profundidad.
*/
int Search(int alpha, int beta, int depth)
{
	if (ply && reps2()) // si ply>=1 y esa misma posición ocurrió antes
	{
		return 0; // retorna 0
	}

	if (depth < 1) // si la profundidad es 0, es el fin de la búsqueda en profundidad
		return CaptureSearch(alpha,beta); // en ese caso, retornaría la evaluación de la posición final, pero prefiere ir un paso más allá: analizar las capturas

	nodes++; // incrementa la cantidad de nodos analizados

	if ((nodes & 4095) == 0) // AND bit and bit entre nodes y 1...1 (doce unos)
	{// si este AND da 0 es que nodes termina en 12 ceros, o sea que nodes es múltiplo de 4096
		CheckUp(); // chequea si no se ha excedido el tiempo, y en dicho caso, detiene la búsqueda
	}

	if (ply > MAX_PLY-2) // si está por exceder el máximo de jugadas permitidas en una variante
		return Eval(); // retorna la evaluación de la posición actual

	move_data bestmove; // aquí va a guardarse la mejor jugada encontrada

	int bestscore = -10001; // inicializa bestscore con el peor posible

	int check = 0; // esta variable se usa como booleano: vale 1 si en la posición actual le están dando jaque a side, y 0 en caso contrario
	if (Attack(xside,kingloc[side])) // si el bando contrario está dando jaque
	{
		check = 1; // setea check en 1
	}

	Gen(); /*genera todos los movimientos
	para esa ply los cuales quedan guardados
	en move_list[i], donde i es tal que:
	first_move[ply] <= i < first_move[ply+1]*/

	if(LookUp(side)) // si la posición actual estaba en la tabla de hash (desde la perspectiva de side)
		SetHashMove(); // busca la jugada en move_list (de la ply en curso) cuyas casillas origen y destino coinciden con las de la tabla de hash, y a esa jugada le asigna un puntaje alto para que así sea elegida primero

	int c = 0; // contador de jugadas legales efectuadas
	int x; // puntaje obtenido de la llamada recursiva a Search()
	int d; // nueva profundidad a pasar en la llamada recursiva a Search()

	for (int i = first_move[ply]; i < first_move[ply + 1]; i++) // recorre cada uno de los movimientos de la ply actual
	{
		Sort(i); // recorre la move_list desde el índice i, y pone en el primer índice la jugada con el más alto score

		if (!MakeMove(move_list[i].start,move_list[i].dest)) // si no puede ser efectuado ese movimiento (por ejemplo, si se movió una pieza clavada)
		{
			continue; // pasa a analizar el siguiente
		}
		c++;

		if (Attack(xside,kingloc[side])) // SI LA JUGADA FUE JAQUE (después de MakeMove(), al que le toca mover es a side y el que recién movió es xside)
		{
			d = depth; // se extiende la profundidad en 1 ply, conocido como "check extension"; se implementa dejando constante la profundidad
		}
		else
		{
			if(move_list[i].score > CAPTURE_SCORE || c==1 || check==1) // SI LA JUGADA FUE UNA CAPTURA || ES LA PRIMERA JUGADA ANALIZADA PARA ESA PLY || MOVIÓ EL REY PARA DEJAR DE ESTAR EXPUESTO A UN JAQUE ("check evasion")
			{
				d = depth - 1; // búsqueda normal
			}
			else if(move_list[i].score > 0) // SI LA JUGADA TENÍA UN SCORE POSITIVO
			{
				d = depth - 2; // reducción de 1 ply
			}
			else // SI JUGADA NO CUMPLE NADA DE LO ANTERIOR
				d = depth - 3; // reducción de 2 ply
		}
		x = -Search(-beta, -alpha, d); // llamada recursiva (cambian los roles de alfa y beta con signo opuesto)

		TakeBack(); // a la vuelta de la llamada recursiva, tiene que que deshacer la última jugada

		if(x > bestscore) // si el puntaje obtenido del hijo es superior al mejor encontrado hasta el momento
		{
			bestscore = x; // actualiza mejor puntaje
			bestmove = move_list[i]; // guarda en bestmove la mejor jugada
		}
		if (x > alpha) // si el puntaje obtenido desde abajo es mejor que alfa => alfa = x
		{
			if (x >= beta) // beta cutoff (ocurrió una poda alfa-beta)
			{
				if(board[move_list[i].dest]==6) // si es un movimiento que no es captura y ocasionó una poda alfa-beta, se guardan el escaque inicial y final en history
				{
					history[move_list[i].start][move_list[i].dest] += depth; // se incrementa con la profundidad
					AddHash(side, move_list[i]);
				}
				return beta; // se retorna beta en ese caso, ya que se sabe que forzosamente ha de ser el puntaje de ese nodo
			}
			alpha = x; // actualiza alfa
		}
	}
	if (c == 0) // no hubo jugadas legales
	{
		if (Attack(xside,kingloc[side])) // si el bando contrario está dando jaque
		{
			return -10000 + ply; // es que fue jaque mate; cuanto mayor es la ply, menor será el puntaje en valor absoluto 
		}
		else
			return 0; // tablas por ahogado (retorna 0)
	}

	if (fifty >= 100)
		return 0; // tablas por regla de las 50 jugadas (retorna 0)
	AddHash(side, bestmove); // agrega al hash la mejor jugada (desde la perspectiva de side)
	return alpha; // retorna alfa
}
/*

CaptureSearch evaluates the position. If the position is more than a queen less than
alpha (the best score that side can do) it doesn't search.
It generates all captures and does a recapture search to see if material is won.
If so, the material gain is added to the score.

*/
int CaptureSearch(int alpha, int beta)
{
	nodes++; // incrementa la cantidad de nodos analizados

	int x = Eval(); // obtiene x: puntaje de la posición actual

	if (x > alpha) // si x es mayor que alfa => actualiza alfa (alfa es el puntaje que terminará retornando)
	{
		if(x >= beta) // si el puntaje es mayor que beta
		{	
			return beta; // retorna beta porque no hay posibilidad de nada mejor según algoritmo minimax
		}
		alpha = x;
	}
	else if(x + 900 < alpha) // si alfa es mayor que la evaluación de la posición más el valor material de una dama
		return alpha; // retorna alfa, sin necesidad de revisar las capturas (ya que lo mejor que podría pasar, es que capture una dama)

	int score = 0; // puntaje de la captura (cuánto "gana" capturando esa pieza)
	int bestmove = 0; // en bestmove se guarda el índice en la move_list correspondiente a la mejor captura
	int best = 0; // mejor puntaje hasta el momento (en la búsqueda de las mejores capturas)

	GenCaptures(); // genera todos los movimientos de captura

	for (int i = first_move[ply]; i < first_move[ply + 1]; i++) 
	{
		Sort(i); // los ordena de mejor a peor

		if(x + piece_value[board[move_list[i].dest]] < alpha) // si x + valor de la pieza capturada < alfa
		{
			continue; // continúa a la siguiente captura ya que no es suficientemente buena
		}

		score = ReCaptureSearch(move_list[i].start, move_list[i].dest); // evalúa cuán "conveniente" es esa captura
		
		if(score>best) // si el puntaje es superior al mejor obtenido hasta el momento
		{
			best = score; // actualiza best
			bestmove = i; // guarda el índice en bestmove
		}
	}

	if(best>0)
	{
		x += best; // suma a x el "plus" por la mejor captura
	}
	if (x > alpha) // si x "actualizado" (o sea, la evaluación de la posición actual + plus por mejor captura) es mayor que alfa
	{
		if (x >= beta) // no tiene sentido retornar x porque no va a poder valer ese nodo más que beta (por algoritmo minimax)
		{
			if(best>0)
				AddHash(side, move_list[bestmove]);
			return beta; // retorna beta
		}
		return x; // retorna x (en caso de x<beta)
	}

	return alpha; // retorna alfa
}
/*

ReCaptureSearch searches the outcome of capturing and recapturing on the same square.
It stops searching if the value of the capturing piece is more than that of the
captured piece and the next attacker. For example, a White queen could take a rook, but a
bishop could take the queen. Even if White could take the bishop, its not worth exchanging a
queen for rook and bishop.

*/
int ReCaptureSearch(int a, const int sq)
// a es casilla de origen (donde está la pieza captora inicial), sq es casilla de destino (donde está la pieza capturada inicial)
{
	int b; // casilla de donde proviene la pieza captora
	int c = 0; // cantidad de capturas "definitivas" (realmente tenidas en cuenta para la valoración)
	int t = 0; // contador del total de movimientos efectuados (para deshacer luego)
	
	int score[12]; 
	/* arreglo donde se van guardando los valores materiales de las piezas capturadas:
	   en índice par los del bando contrario (xside) y en índice impar los del bando 
	   que inició la secuencia de capturas.
	*/

	memset(score,0,sizeof(score)); // limpia score todo con ceros
	score[0] = piece_value[board[sq]]; // score[0] es el valor de la 1ª pieza capturada
	score[1] = piece_value[board[a]]; // score[1] es el valor de la 1ª pieza captora
	// esto quiere decir que en la captura i, score[i] tiene el valor de la pieza captora y score[i-1]

	int total_score = 0;

	while(c < 10)
	{
		if(!MakeRecapture(a,sq)) // si es ilegal la captura, sale del while
			break;
		t++; // incrementa el contador de movimientos totales
		nodes++; // incrementa la cantidad de nodos analizados
		c++; // incrementa el contador de capturas

		b = LowestAttacker(side,sq); // obtiene la casilla de la pieza de menor valor que podría capturar en sq

		if(b>-1) // si hay una pieza que puede capturar 
		{
			score[c + 1] = piece_value[board[b]]; // guarda en el siguiente índice par disponible el valor la pieza que recapturaría
			/* score[c] : valor de la pieza que acaba de capturar (por ejemplo, una dama)
			   socre[c - 1] : valor de la pieza recién capturada (por ejemplo, una torre)
			   score[c + 1] : valor de la pieza del bando contrario que recapturaría en sq (por ejemplo, un alfil) */
			if(score[c] > score[c - 1] + score[c + 1]) // en este caso la pieza usada para capturar (dama) vale más que la capturada (torre) más la que emplearía el rival para recapturar nuestra dama (alfil)
			{ // aún bajo el supuesto de que se pueda recapturar ese alfil, no es conveniente cambiar dama por torre y alfil, entonces se detiene la búsqueda
				c--; // también se decrementa c, ya que c se había incrementado pero esta captura no será realizada
				break;
			}
		}
		else // si no hay pieza que pueda capturar en sq
		{
			// sale del while (si ocurrió la primera vez, es que la captura fue "limpia" i.e. el bando contrario no tiene nada de material en compensación)
			break; // en dicho caso, total_score va a terminar valiendo el valor material de la pieza capturada
		}
		a = b; // la casilla origen es ahora aquella de donde proveniría la siguiente pieza captora
	}
	/* Por ejemplo, en el caso de la posición 2r2rk1/pp2bpp1/4pn1p/qb1nN3/3P3B/1BN5/PPQR1PPP/4R1K1 b - - 0 1 
	   la secuencia de capturas sería 1...Nxc3 2.bxc3 Rxc3 3.Qxc3 Qxc3 y las blancas ya no tienen pieza que 
	   ataque c3.
	   
	   Inicialmente, c=0 y score = { 300, 300, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
	   
	   -> Tras 1...Nxc3(c=0), c=1. Las blancas se preguntan cuál es la pieza de menor valor que ataca c3. 
	   Es el peón de b2. Se hace score[2] = piece_value[board[B2]] (==100). Como el caballo no es más valioso
	   que un caballo y un peón, continúa el while.
	   
	   -> Tras 2.bxc3(c=1), c=2. Las negras se preguntan cuál es la pieza de menor valor que ataca c3. 
	   Es la torre de c8. Se hace score[3] = piece_value[board[C8]] (==500). Como el peón no es más valioso 
	   que un caballo y una torre, continúa el while.
	   
	   -> Tras 2...Rxc3(c=2), c=3. Las blancas se preguntan cuál es la pieza de menor valor que ataca c3. 
	   Es la dama de c2. Se hace score[4] = piece_value[board[C2]] (==900). Como la torre no es más valiosa 
	   que un peón y una dama, continúa el while.
	   
	   -> Tras 3.Qxc3(c=3), c=4. Las negras se preguntan cuál es la pieza de menor valor que ataca c3, 
	   Es la dama de a5. Se hace score[5] = piece_value[board[A5]] (==900). Como la dama no es más valiosa 
	   que una torre y una dama, continúa el while.
	   
	   -> Tras 3...Qxc3(c=4), c=5. Las blancas se preguntan cuál es la pieza de menor valor que ataca c3, 
	   ¡No hay! Con lo cual b==-1. Sale del while.
	   
	   En este instante, c=5 y score = { 300, 300, 100, 500, 900, 900, 0, 0, 0, 0, 0, 0 }
	   
	   Cuando las negras hicieron 3.Qxc3, evaluaron que el cambio era conveniente:
	   Dama (pieza captora) por 
	   Torre (pieza capturada) y Dama (pieza que recapturarían eventualmente más adelante),
	   sin embargo en la iteración siguiente se dieron cuenta de esa dama no la pueden recapturar 
	   (porque no tienen ninguna pieza que amenace c3). Por tanto hay que deshacer las dos últimas 
	   capturas si ocurre que en la penúltima el valor de la pieza captora (dama, en el ejemplo) es 
	   superior al de la capturada (torre). Esto es lo que hace el while siguiente:
	   */
	while(c>1) // hay que chequear la captura c-1 (penúltima)
	{
		if(score[c-1] >= score[c-2]) // si el valor de la pieza captora es superior al de la capturada
			c -= 2; // decrementa dos
		else
			break;
	} // PREGUNTA: ¿es necesario que este if esté dentro de un while? ¿No basta sólo con chequear la c-1?

	for(int x=0; x < c; x++)
	{
		if(x%2 == 0)
			total_score += score[x]; // los puntajes de índice par se suman porque corresponden a ganancia de material para side
		else
			total_score -= score[x]; // los puntajes de índice impar se restan porque corresponden a pérdida de material para side
	}

	while(t) // deshace todos los movimientos efectuados
	{
		UnMakeRecapture();
		t--;
	}

	return total_score; // retorna el puntaje final tras las capturas (ganancia de material)
}
/*

reps2() searches backwards for an identical position.
A positions are identical if the key and lock are the same.
'fifty' represents the number of moves made since the last pawn move or capture.

*/
int reps2()
{
	for (int i = hply-4; i >= hply-fifty; i-=2)
	{
		if (game_list[i].hash == currentkey && game_list[i].lock == currentlock) // misma key y lock
		{
			return 1;
		}
	}
	return 0;
}
/*

Sort searches the move list for the move with the highest score.
It is moved to the top of the list so that it will be played next.

*/
void Sort(const int from)
// from es el índice a partir del cual buscar en move_list
{
	move_data m;

	int bs = move_list[from].score; // bs es abreviatura de "best score" (mejor puntaje encontrado hasta el momento)
	int bi = from; // bi es abreviatura de "best index" (índice del movimiento de mejor puntaje)
	for (int i = from + 1; i < first_move[ply + 1]; ++i) // recorre move_list desde from hasta first_move[ply+1] que es donde arrancan las jugadas de la siguiente ply
		if (move_list[i].score > bs) // si el score de la jugada actual es mayor al máximo encontrado
		{
			bs = move_list[i].score; // actualiza el máximo
			bi = i; // actualiza el índice donde se encuentra el máximo
		}
	// swap entre move_list[from] y move_list[bi]
	m = move_list[from];
	move_list[from] = move_list[bi];
	move_list[bi] = m;
}
/*

checkup checks to see if the time has run out.
If so, the search ends.

*/
void CheckUp()
{
	if( (GetTime() >= stop_time || (max_time<50 && ply>1)) && fixed_depth==0 && ply>1)
	{
		stop_search = true; // setea la bandera de detener búsqueda en verdadero
		longjmp(env, 0); // salta a set
	}
}
/*

SetHashMove searches the move list for the move from the Hash Table.
If it finds it, it sets the move a high score so that it will be played first.

*/
void SetHashMove()
{
	for(int x=first_move[ply];x < first_move[ply+1];x++)
	{
		if(move_list[x].start == hash_start && move_list[x].dest == hash_dest)
		{
			move_list[x].score = HASH_SCORE;
			return;
		}
	}
}
/*

DisplayPV displays the principal variation(PV). This is the best line of play by both sides.
Firstly it displays the best move at the root. 
It plays this move so that the current hash key and lock will be correct.
It looks up the Hash Table and finds the best move at the greater depth and
continues until no more best moves can be found.
Lastly, it takes back the moves, returning to the original position.

*/
void DisplayPV(int i)
{
	move_start = hash_start;
	move_dest = hash_dest;

	for(int x=0;x < i;x++)
	{
		if(LookUp(side)==false)
			break;
		printf(" ");
		Alg(hash_start,hash_dest);
		MakeMove(hash_start,hash_dest);
	}
	while (ply)
		TakeBack();
}
