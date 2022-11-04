#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <time.h> // para crear un número aleatorio

#include "../include/imagen.h"
#include "../include/ga.h"

#include <mpi.h>
#include "derivados_mpi.c"

static double mseconds() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
}

int main(int argc, char **argv)
{
	int ancho, alto, max, total;
	int num_generaciones, tam_poblacion;
	
	ancho = alto = max = 0;
	num_generaciones = tam_poblacion = 0;

	int name, p;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &name);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	RGB *imagen_objetivo=NULL, *mejor_imagen;

	// Solo recibe los parámetros el proceso 0
	if (name == 0){
		// Check Input
		if(argc < 4) {
			printf("Ayuda:\n"); 
			printf("  ./programa entrada salida num_generaciones tam_poblacion\n");
			//return(-1);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		num_generaciones = atoi(argv[3]);
		tam_poblacion = atoi(argv[4]);
		
		if (tam_poblacion % 4 != 0) {
			printf("El tamaño de la población debe ser divisible por 4\n");
			//return(-1);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
		
		// Read Input Data
		imagen_objetivo = leer_ppm(argv[1], &ancho, &alto, &max);
	} 

	// El proceso 0 comunica al resto los parámetros
	MPI_Bcast(&ancho, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&alto, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&max, 1, MPI_INT, 0, MPI_COMM_WORLD);
	total = ancho*alto;
	MPI_Bcast(&num_generaciones, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&tam_poblacion, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	// Los demás procesos reservan la memoria para la imagen_objetivo
	if (name != 0)
		imagen_objetivo = malloc(total * 3*sizeof(char));

	// El proceso 0 comunica la imagen_objetivo
	MPI_Datatype rgb_type, individuo_type;
	crear_tipo_datos(total, &rgb_type, &individuo_type);
	MPI_Bcast(imagen_objetivo, total, rgb_type, 0, MPI_COMM_WORLD);

	// Allocate Memory for Output Data
	mejor_imagen = (RGB *) malloc(total*sizeof(RGB));

	// Call Genetic Algorithm
	srand (time(NULL));
	
	#ifdef TIME
		double ti = mseconds();
	#endif
	
	crear_imagen(imagen_objetivo, total, ancho, alto, max,
				num_generaciones, tam_poblacion, mejor_imagen, argv[2], rgb_type, individuo_type);	

	MPI_Barrier(MPI_COMM_WORLD);
	
	if (name == 0){
		#ifdef TIME
			double tf = mseconds();
			printf("Execution Time = %.6lf seconds\n", (tf - ti));
		#endif
	}
		
	// Smooth Output Image
	RGB *img_out = (RGB *) malloc(ancho*alto*sizeof(RGB));
	suavizar(ancho, alto, mejor_imagen, img_out, rgb_type);

	if (name == 0){

		#ifdef DEBUG
			// Print Result
			escribir_ppm(argv[2], ancho, alto, max, img_out); 
		#endif

	}

	MPI_Barrier(MPI_COMM_WORLD);

	free(img_out);
	free(mejor_imagen);
	free(imagen_objetivo);

	MPI_Finalize();
	
	return(0);
}
