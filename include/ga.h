#ifndef _GA
#define _GA

#include "imagen.h"
#include <mpi.h>

#define TAM_IMAGEN 93*128

typedef struct {
	//RGB *imagen;
	RGB imagen[TAM_IMAGEN]; // RGB estático para poder mandarlo por MPI
	double fitness;
} Individuo;

void crear_imagen(const RGB *, int, int, int, int, int, int, RGB *, const char *, MPI_Datatype, MPI_Datatype);
void cruzar(Individuo *, Individuo *, Individuo *, Individuo *, int);
void fitness(const RGB *, Individuo *, int);
void mutar(Individuo *, int, int);

#endif
