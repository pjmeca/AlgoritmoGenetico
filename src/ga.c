#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../include/imagen.h"
#include "../include/ga.h"
#include <unistd.h>

#define PRINT 1

#define NUM_PIXELS_MUTAR 0.01
#define NUM_ITERACIONES_CONVERGENCIA 200

#define NGM_PORCENTAJE 0.1

static int aleatorio(int max)
{
	return (rand() % (max + 1));
}

void init_imagen_aleatoria(RGB *imagen, int max, int total)
{
	for (int i = 0; i < total; i++)
	{
		imagen[i].r = aleatorio(max);
		imagen[i].g = aleatorio(max);
		imagen[i].b = aleatorio(max);
	}
}

// Para comparar punteros a individuos
static int comp_fitness(const void *a, const void *b)
{
	/* qsort pasa un puntero al elemento que está ordenando */
	return (*(Individuo **)a)->fitness - (*(Individuo **)b)->fitness;
}

// Para comparar individuos
static int comp_fitness2(const void *a, const void *b)
{
	/* qsort pasa un puntero al elemento que está ordenando */
	return (*(Individuo *)a).fitness - (*(Individuo *)b).fitness;
}

void crear_imagen(const RGB *imagen_objetivo, int num_pixels, int ancho, int alto, int max, int num_generaciones,
	int tam_poblacion, RGB *imagen_resultado, const char *output_file, MPI_Datatype rgb_type, MPI_Datatype individuo_type)
{
	// Intercambios MPI
	int NGM = num_generaciones*NGM_PORCENTAJE;
	int NEM = tam_poblacion>10 ? 10 : 1;

	int name, p;
	MPI_Comm_rank(MPI_COMM_WORLD, &name);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	int tam_poblacion_proceso = tam_poblacion/p;

	// A. Crear Poblacion Inicial (array de imagenes aleatorias)
	Individuo** poblacion = (Individuo **)malloc(tam_poblacion_proceso * sizeof(Individuo *));
	assert(poblacion);

	int i, mutation_start, contador_fitness = 0;
	double fitness_anterior, fitness_actual, diferencia_fitness;

	for (i = 0; i < tam_poblacion_proceso; i++)
	{
		poblacion[i] = (Individuo *)malloc(sizeof(Individuo));
		init_imagen_aleatoria(poblacion[i]->imagen, max, num_pixels);
	}

	for (i = 0; i < tam_poblacion_proceso; i++)
	{
		fitness(imagen_objetivo, poblacion[i], num_pixels);
	}

	// Ordenar individuos según la función de bondad (menor "fitness" --> más aptos)
	qsort(poblacion, tam_poblacion_proceso, sizeof(Individuo *), comp_fitness);

	// B. Evolucionar la Población (durante un número de generaciones)
	for (int g = 0; g < num_generaciones; g++)
	{
		fitness_anterior = poblacion[0]->fitness;

		// Promocionar a los descendientes de los individuos más aptos
		for (i = 0; i < (tam_poblacion_proceso / 2) - 1; i += 2)
		{
			cruzar(poblacion[i], poblacion[i + 1], poblacion[tam_poblacion_proceso / 2 + i], poblacion[tam_poblacion_proceso / 2 + i + 1], num_pixels);
		}

		// Mutar una parte de la individuos de la población (se decide que muten tam_poblacion/4)
		mutation_start = tam_poblacion_proceso / 4;

		for (i = mutation_start; i < tam_poblacion_proceso; i++)
		{
			mutar(poblacion[i], max, num_pixels);
		}

		// Recalcular Fitness
		for (i = 0; i < tam_poblacion_proceso; i++)
		{
			fitness(imagen_objetivo, poblacion[i], num_pixels);
		}

		// Envíos intermedios a P0
		if(g%(NGM-1) == 0){

			// Buffer de envío de cada proceso
			Individuo *sendbuf = malloc(NEM*sizeof(Individuo));
			Individuo *sendbuf2; 
			if(name == 0)
				sendbuf2 = malloc(NEM*p*sizeof(Individuo));
			else 
				sendbuf2 = malloc(NEM*sizeof(Individuo));

			// Cada proceso elige NEM posiciones aleatorias y las almacena en un array
			int* posiciones = malloc(NEM*sizeof(int));
			for(int i=0; i<NEM; i++){
				bool contenido = true;
				int num = aleatorio(tam_poblacion_proceso-1);
				while(contenido){
					contenido = false;
					for(int j = 0; j<i && !contenido; j++){
						if (posiciones[j]==num){
							num = aleatorio(tam_poblacion_proceso-1);
							contenido = true;
						}
					}
				}
				posiciones[i] = num;

				// Cada proceso mueve los individuos de esas posiciones al buffer de envío
				memmove(&sendbuf[i], poblacion[num], sizeof(Individuo));
			}

			// Cada proceso envía al proceso 0 sus individuos
			MPI_Gather(sendbuf, NEM, individuo_type,
					   sendbuf2, NEM, individuo_type,
					   0, MPI_COMM_WORLD);

			// El proceso 0 ordena por el fitness
			if (name == 0)
				qsort(sendbuf2, NEM*p, sizeof(Individuo), comp_fitness2);

			// Y envía los mejores a todos los procesos
			MPI_Bcast(sendbuf2, NEM, individuo_type, 0, MPI_COMM_WORLD);

			// Cada proceso vuelve a colocar los individuos en las posiciones seleccionadas
			for (int i=0; i<NEM; i++){
				memmove(poblacion[posiciones[i]], &sendbuf2[i], sizeof(Individuo));
			}

			// Antes de liberar, tenemos que esperar a que todos los procesos copien
			MPI_Barrier(MPI_COMM_WORLD);

			// Liberar la memoria
			free(sendbuf);
			free(sendbuf2);
			free(posiciones);
		}

		// Ordenar individuos según la función de bondad (menor "fitness" --> más aptos)
		qsort(poblacion, tam_poblacion_proceso, sizeof(Individuo *), comp_fitness);

		// La mejor solución está en la primera posición del array
		fitness_actual = poblacion[0]->fitness;
		diferencia_fitness = abs(fitness_actual - fitness_anterior);

		// Guardar cada 300 iteraciones para observar el progreso
		if (PRINT /*&& (g % 300 == 0)*/)
		{
			printf("Generacion %d - ", g);
			printf("Fitness = %e - ", fitness_actual);
			printf("Diferencia con Fitness Anterior = %.2e%c\n", diferencia_fitness, 37);

			/*
			// Generar imagenes intermedias
			if ((g % 300) == 0) {
				char *output_file2 = malloc(1000);
				sprintf(output_file2,"image_%d.ppm",g);
				escribir_ppm(output_file2, ancho, alto, max, poblacion[0]->imagen);
				printf("Imagen intermedia generada: %s\n", output_file2);
				free(output_file2);
			}*/
		}

		// Criterio de convergencia
		if (diferencia_fitness < 0.000001)
		{
			contador_fitness++;

			if (contador_fitness == NUM_ITERACIONES_CONVERGENCIA)
			{
				printf("Parada en la generación %d - Por alcanzar %d generaciones con el mismo valor fitness.\n", g, NUM_ITERACIONES_CONVERGENCIA);
				break;
			}
		}
		else
			contador_fitness = 0;
	}

	// Recibimos el mejor de cada proceso
	Individuo *mejores = NULL;
	if(name == 0) {
		mejores = malloc(sizeof(Individuo)*p);
		assert(mejores);
	}
	MPI_Gather(poblacion[0], 1, individuo_type,
		mejores, 1, individuo_type, 0, MPI_COMM_WORLD);

	Individuo *resultado = malloc(sizeof(Individuo));

	if(name == 0){
		// Los ordenamos
		qsort(mejores, p, sizeof(Individuo), comp_fitness2);

		memmove(resultado, mejores[0].imagen, num_pixels * sizeof(RGB));
	}

	// Enviamos la mejor imagen al resto de procesos (importante para suavizar)
	MPI_Bcast(resultado, 1, individuo_type, 0, MPI_COMM_WORLD);
		
	// Devuelve Imagen Resultante
	memmove(imagen_resultado, resultado, num_pixels * sizeof(RGB));	

	MPI_Barrier(MPI_COMM_WORLD);

	if (name == 0)
		free(mejores);
	free(resultado);

	// Release memory
	for (int i = 0; i < tam_poblacion_proceso; i++)
		free(poblacion[i]);
	free(poblacion);
}

void cruzar(Individuo *padre1, Individuo *padre2, Individuo *hijo1, Individuo *hijo2, int num_pixels)
{
	// Elegir un "punto" de corte aleatorio a partir del cual se realiza el intercambio de los genes.
	// * Cruzar los genes de cada padre con su hijo
	// * Intercambiar los genes de cada hijo con los del otro padre

	// Elegimos el punto
	int punto_corte = aleatorio(num_pixels - 1);

	// Curzamos los genes
	Individuo *p1 = padre1;
	Individuo *p2 = padre2;
	for (int i = 0; i < num_pixels; i++)
	{
		// Si estamos en la segunda mitad, los intercambiamos
		if (i == punto_corte)
		{
			p1 = padre2;
			p2 = padre1;
		}

		// Hijo 1
		hijo1->imagen[i].r = p1->imagen[i].r;
		hijo1->imagen[i].g = p1->imagen[i].g;
		hijo1->imagen[i].b = p1->imagen[i].b;

		// Hijo 2
		hijo2->imagen[i].r = p2->imagen[i].r;
		hijo2->imagen[i].g = p2->imagen[i].g;
		hijo2->imagen[i].b = p2->imagen[i].b;
	}
}

void fitness(const RGB *objetivo, Individuo *individuo, int num_pixels)
{
	// Determina la calidad del individuo (similitud con el objetivo)
	// calculando la suma de la distancia existente entre los pixeles
	double fitness = 0.0;
	for (int i = 0; i < num_pixels; i++)
	{
		// Distancia euclídea
		fitness += sqrt(pow(objetivo[i].r - individuo->imagen[i].r, 2) + pow(objetivo[i].g - individuo->imagen[i].g, 2) + pow(objetivo[i].b - individuo->imagen[i].b, 2));
	}

	individuo->fitness = fitness;
}

void mutar(Individuo *actual, int max, int num_pixels)
{
	// Cambia el valor de algunos puntos de la imagen de forma aleatoria.

	// Decidir cuantos pixels mutar. Si el valor es demasiado pequeño,
	// la convergencia es muy pequeña, y si es demasiado alto diverge.

	// Píxeles a mutar
	int num_pixels_mutar = aleatorio(num_pixels * NUM_PIXELS_MUTAR); // 0...NUM_PIXELS_MUTAR%

	// Cambiar el valor de los puntos
	for (int i = 0; i < num_pixels_mutar; i++)
	{
		int pos = aleatorio(num_pixels - 1);
		actual->imagen[pos].r = aleatorio(max); 
		actual->imagen[pos].g = aleatorio(max);
		actual->imagen[pos].b = aleatorio(max);
	}
}