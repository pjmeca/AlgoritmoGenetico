#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <omp.h>

#include "../include/imagen.h"
#include "../include/ga.h"

#define PRINT 1

#define NUM_PIXELS_MUTAR 0.01
#define NUM_ITERACIONES_CONVERGENCIA 200 

#define OMP_FITNESS "REDUCTION"
#define SCHEDULE_STATIC
#define CHUNK_SIZE 4

static int aleatorio(int max)
{
	return (rand() % (max + 1));
}

void init_imagen_aleatoria(RGB *imagen, int max, int total)
{
	#ifdef SCHEDULE_STATIC
		#pragma omp parallel for schedule(static, CHUNK_SIZE)
	#elif defined SCHEDULE_DYNAMIC
		#pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
	#elif defined SCHEDULE_GUIDED
		#pragma omp parallel for schedule(guided, CHUNK_SIZE)
	#endif
	for (int i = 0; i < total; i++)	// Paralelizable
	{
		imagen[i].r = aleatorio(max);
		imagen[i].g = aleatorio(max);
		imagen[i].b = aleatorio(max);
	}
}

RGB *imagen_aleatoria(int max, int total)
{
	RGB *imagen = (RGB *)malloc(total * sizeof(RGB));
	assert(imagen);

	init_imagen_aleatoria(imagen, max, total);
	return imagen;
}

static int comp_fitness(const void *a, const void *b)
{
	/* qsort pasa un puntero al elemento que está ordenando */
	return (*(Individuo **)a)->fitness - (*(Individuo **)b)->fitness;
}

void crear_imagen(const RGB *imagen_objetivo, int num_pixels, int ancho, int alto, int max, int num_generaciones, int tam_poblacion, RGB *imagen_resultado, const char *output_file)
{
	int i, mutation_start, contador_fitness = 0;
	double fitness_anterior, fitness_actual, diferencia_fitness;

	// A. Crear Poblacion Inicial (array de imagenes aleatorias)
	Individuo **poblacion = (Individuo **)malloc(tam_poblacion * sizeof(Individuo *));
	assert(poblacion);

	#ifdef SCHEDULE_STATIC
		#pragma omp parallel for schedule(static, CHUNK_SIZE)
	#elif defined SCHEDULE_DYNAMIC
		#pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
	#elif defined SCHEDULE_GUIDED
		#pragma omp parallel for schedule(guided, CHUNK_SIZE)
	#endif
	for (i = 0; i < tam_poblacion; i++) // Paralelizable
	{
		poblacion[i] = (Individuo *)malloc(sizeof(Individuo));
		poblacion[i]->imagen = imagen_aleatoria(max, num_pixels);
	}

	for (i = 0; i < tam_poblacion; i++) // Paralelizable
	{
		fitness(imagen_objetivo, poblacion[i], num_pixels);
	}

	// Ordenar individuos según la función de bondad (menor "fitness" --> más aptos)
	qsort(poblacion, tam_poblacion, sizeof(Individuo *), comp_fitness);

	// B. Evolucionar la Población (durante un número de generaciones)
	for (int g = 0; g < num_generaciones; g++)
	{
		fitness_anterior = poblacion[0]->fitness;

		for (i = 0; i < (tam_poblacion / 2) - 1; i += 2) // Paralelizable
		{
			cruzar(poblacion[i], poblacion[i + 1], poblacion[tam_poblacion / 2 + i], poblacion[tam_poblacion / 2 + i + 1], num_pixels);
		}

		// Mutar una parte de la individuos de la población (se decide que muten tam_poblacion/4)
		mutation_start = tam_poblacion / 4;
	
		for (i = mutation_start; i < tam_poblacion; i++) // Paralelizable
		{
			mutar(poblacion[i], max, num_pixels);
		}

		// Recalcular Fitness
		for (i = 0; i < tam_poblacion; i++) // Paralelizable
		{
			fitness(imagen_objetivo, poblacion[i], num_pixels);
		}
	
		// Ordenar individuos según la función de bondad (menor "fitness" --> más aptos)
		qsort(poblacion, tam_poblacion, sizeof(Individuo *), comp_fitness);

		// La mejor solución está en la primera posición del array
		fitness_actual = poblacion[0]->fitness;
		diferencia_fitness = abs(fitness_actual - fitness_anterior);

		// Guardar cada 300 iteraciones para observar el progreso
		if (PRINT && (g % 300 == 0))
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

	// Devuelve Imagen Resultante
	memmove(imagen_resultado, poblacion[0]->imagen, num_pixels * sizeof(RGB));

	// Release memory
	#ifdef SCHEDULE_STATIC
		#pragma omp parallel for schedule(static, CHUNK_SIZE)
	#elif defined SCHEDULE_DYNAMIC
		#pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
	#elif defined SCHEDULE_GUIDED
		#pragma omp parallel for schedule(guided, CHUNK_SIZE)
	#endif
	for (i = 0; i < tam_poblacion; i++) // Paralelizable
	{
		free(poblacion[i]->imagen);
		free(poblacion[i]);
	}

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
	for (int i = 0; i < num_pixels; i++) // Paralelizable
	{
		// Si estamos en la segunda mitad, los intercambiamos
		if (i >= punto_corte) // Hay que poner el mayor para poder paralelizarlo
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
	
	if(strcmp(OMP_FITNESS, "SECUENCIAL") == 0){
		for (int i = 0; i < num_pixels; i++) // Paralelizable
		{
			// Distancia euclídea
			fitness += sqrt(pow(objetivo[i].r - individuo->imagen[i].r, 2) + pow(objetivo[i].g - individuo->imagen[i].g, 2) + pow(objetivo[i].b - individuo->imagen[i].b, 2));
		}
	} else if(strcmp(OMP_FITNESS, "CRITICAL") == 0){
		#pragma omp parallel for shared(fitness, individuo, objetivo) // serían compartidas las variables: fitness, objetivo e individuo, y privada la variable i
		for (int i = 0; i < num_pixels; i++) // Paralelizable
		{
			#pragma omp critical
			fitness += sqrt(pow(objetivo[i].r - individuo->imagen[i].r, 2) + pow(objetivo[i].g - individuo->imagen[i].g, 2) + pow(objetivo[i].b - individuo->imagen[i].b, 2));
		}	
	} else if(strcmp(OMP_FITNESS, "ATOMIC") == 0){
		#pragma omp parallel for shared(fitness, individuo, objetivo) // serían compartidas las variables: fitness, objetivo e individuo, y privada la variable i
		for (int i = 0; i < num_pixels; i++) // Paralelizable
		{
			#pragma omp atomic
			fitness += sqrt(pow(objetivo[i].r - individuo->imagen[i].r, 2) + pow(objetivo[i].g - individuo->imagen[i].g, 2) + pow(objetivo[i].b - individuo->imagen[i].b, 2));
		}
	} else if(strcmp(OMP_FITNESS, "REDUCTION") == 0){
		#pragma omp parallel for reduction(+: fitness)
		for (int i = 0; i < num_pixels; i++) // Paralelizable
		{
			fitness += sqrt(pow(objetivo[i].r - individuo->imagen[i].r, 2) + pow(objetivo[i].g - individuo->imagen[i].g, 2) + pow(objetivo[i].b - individuo->imagen[i].b, 2));
		}
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
	for (int i = 0; i < num_pixels_mutar; i++) // Paralelizable
	{
		int pos = aleatorio(num_pixels - 1);
		actual->imagen[pos].r = aleatorio(max); 
		actual->imagen[pos].g = aleatorio(max);
		actual->imagen[pos].b = aleatorio(max);
	}
}
