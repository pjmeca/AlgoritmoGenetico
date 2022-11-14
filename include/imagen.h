#ifndef _IMAGEN
#define _IMAGEN

#include <mpi.h>

typedef struct {
	unsigned char r, g, b;
} RGB;

RGB *leer_ppm(const char *, int *, int *, int *);
void escribir_ppm(const char *, int, int, int, const RGB *);
void suavizar(int, int, RGB *, RGB *, MPI_Datatype);

#endif
