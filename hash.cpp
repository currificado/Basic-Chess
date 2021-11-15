#include "globals.h"

/* Cada posición en el tablero va a tener asociado un identificador (la clave o key). 
   Para que dos posiciones sean idénticas, no basta con que las piezas estén ubicadas 
   en el mismo lugar en el tablero sino que además debe tocarle mover al mismo bando. 
   Este identificador podría ser el mismo para dos posiciones diferentes. Por eso se usa 
   un segundo identificador (el cerrojo o lock). La probabilidad de que dos posiciones 
   diferentes coincidan en clave y cerrojo es despreciable.
*/
U64 currentkey,currentlock; // currentkey es la clave de la posición actual, currentlock es el cerrojo de la posición actual
U64 collisions;

/* Éstas dos son tablas estáticas que se usarán para el cálculo de la clave y cerrojo. 
   A cada escaque ocupado por un pieza (o vacío) se le asigna al azar un número entre 0 y HASHSIZE-1 
   (y esto para cada bando). Por ejemplo, hash[White][K][G1] es el número que corresponde a que el rey 
   blanco esté colocado en la casilla G1.
*/
U64 hash[2][6][64];
U64 lock[2][6][64];

const U64 MAXHASH =  5000000; // cantidad de elementos en las tablas de hash
const U64 HASHSIZE = 5000000; // rango usado para los números

/* Las tablas de hash (una para las blancas y otra para las negras) van a ser arreglos de memoria dinámica 
   (en el heap del proceso) de tamaño MAXHASH. Va a estar indizada por clave. Cada nodo de este arreglo va 
   a ser del tipo hashp (que representa una posición junto con la jugada que debe moverse en caso de dicha 
   posición).
*/

int hash_start,hash_dest; // variables globales donde se guarda la jugada extraída de la tabla de hash

/*

A hash table entry includes a lock and start and dest squares.

*/
struct hashp // entrada de la tabla de hash
{
	U64 hashlock; // cerrojo (la clave es el índice el nodo)
	int start; // casilla origen
	int dest; // casilla destino
	int num; // ¿este campo para qué es? no se usa nunca...
};

hashp *hashpos[2]; // arreglo de punteros: uno a la tabla de hash de blancas y el otro a la tabla de hash de negras
unsigned int hashpositions[2]; // contador de la cantidad de posiciones en la tabla de hash (hay uno para cada bando)
/*

RandomizeHash is called when the engine is started.
The whitehash, blackhash, whitelock and blacklock tables
are filled with random numbers.

*/
void RandomizeHash()
{// inicializa las tablas hash y lock y pide memoria para las dos tablas de hash consideradas
	int p,x;
	for(p=0;p<6;p++)
	{
		for(x=0;x<64;x++)
		{
			hash[0][p][x] = Random(HASHSIZE);
			hash[1][p][x] = Random(HASHSIZE);
			lock[0][p][x] = Random(HASHSIZE);
			lock[1][p][x] = Random(HASHSIZE);
		}
		hashpos[0] = new hashp[MAXHASH]; // pide memoria para la tabla de hash de blancas
	}
	hashpos[1] = new hashp[MAXHASH]; // pide memoria para la tabla de hash de negras
}
/*

Random() generates a random number up to the size of x.

*/
int Random(const int x)
{
	return rand() % x;
}
/*

Free() Frees memory that was allocated to the hashpos pointers 

with new.

*/
void Free()
{
	delete hashpos[0];
	delete hashpos[1];
}
/*

FreeAllHash() empties the Hash Tables.

*/
void FreeAllHash()
{
	hashpositions[0]=0;
	hashpositions[1]=0;
}
/*
Adds an entry into the HashTable.
If that index is already being used, it simply overwrites it.

*/
void AddHash(const int s, const move_ m)
// s es el bando que mueve, m es la jugada
{
	hashp* ptr = &hashpos[s][currentkey]; // obtiene un puntero al nodo (de la tabla de hash de side) ubicado en el índice de la clave
	ptr->hashlock = currentlock; // guarda el cerrojo actual
	ptr->start=m.start; // guarda la casilla origen
	ptr->dest=m.dest; // guarda la casilla destino
}
/*

AddKey updates the current key and lock.
The key is a single number representing a position.
Different positions may map to the same key.
The lock is very similar to the key (its a second key), which 
is a different number
because it was seeded with different random numbers.
While the odds of several positions having the same key are 
very high, the odds of
two positions having the same key and same lock are very very 
low.

*/
void AddKey(const int s,const int p,const int x)
{
  currentkey ^= hash[s][p][x]; // hace el xor de currentkey con hash[s][p][x] (de esa manera se alteran sólo los bits de ese escaque)
  currentlock ^= lock[s][p][x]; // xor de currentlock con lock[s][p][x]
} /* OBSERVACIÓN: cuando se realiza un movimiento, hay que efectuar 2 AddKey's: 
     + el primero para "deshacer" el impacto que tenía en las claves que la pieza estuviera en la casilla de origen (recordar que ((a xor b) xor b) == a)
     + el segundo para "reflejar" en las claves el hecho de que ahora la pieza está en la casilla destino
*/
/*

GetLock gets the current lock from a position.

*/
U64 GetLock()
{
	U64 loc=0; // parte de loc todo ceros
	for(int x=0;x<64;x++) // recorre una por una las casillas del tablero
	{
	if(board[x]!=6) // si la casilla no está vacía
		loc ^= lock[color[x]][board[x]][x]; // hace el xor de loc con el valor que corresponde según la tabla estática de lock
	}
	return loc; // finalmente retorna loc
}
/*

GetKey gets the current key from a position.

*/
U64 GetKey()
{
	U64 key=0; // parte de key todo ceros
	for(int x=0;x<64;x++) // recorre una por una las casillas del tablero
	{
	if(board[x]!=6) // si la casilla no está vacía
		key ^= hash[color[x]][board[x]][x]; // hace el xor de key con el valor que corresponde según la tabla estática de hash
	}
	return key; // finalmente retorna key
}
/*

Looks up the current position to see if it is in the HashTable.
If so, it fetches the move stored there.

*/
bool LookUp(const int s)
{
	if(hashpos[s][currentkey].hashlock != currentlock)
	{
		/* Esto puede ocurrir en dos situaciones:
		1) La entrada currentkey de la tabla de hash de s nunca fue utilizada.
		2) La entrada currentkey de la tabla de hash de s está ocupada, pero el cerrojo de dicha entrada no coincide con el cerrojo de la posición actual (hubo colisión).
		*/
		return false; // en cualquiera de los dos casos, se devuelve falso (no fue encontrada la posición actual en la tabla de hash)
	}
	// en caso contrario, se setean hash_start y hash_dest y se devuelve verdadero
	hash_start = hashpos[s][currentkey].start;
	hash_dest = hashpos[s][currentkey].dest;
	return true;
}

