#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../include/imagen.h"
#include "../include/ga.h"
#include <unistd.h>

#define PRINT 0

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
	int name, p;
	MPI_Comm_rank(MPI_COMM_WORLD, &name);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	// Intercambios MPI
	int NGM = num_generaciones*NGM_PORCENTAJE;
	int NEM = tam_poblacion>10 ? 10 : 1;

	// Buffers de envío de cada proceso
	int posicion=0; //
	int size_ind = sizeof(double)+TAM_IMAGEN*3; //
	char *sendbuf = malloc(size_ind*NEM);
	char *sendbufaux = malloc(size_ind*NEM*p); //
	Individuo *sendbuf2; 
	if(name == 0){
		sendbuf2 = malloc(NEM*p*sizeof(Individuo));
	}else 
		sendbuf2 = malloc(NEM*sizeof(Individuo));

	int tam_poblacion_proceso = tam_poblacion/p;
	if (name == p-1)
		tam_poblacion_proceso += tam_poblacion%p;

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
			
			posicion = 0;
			
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

				// RGB				
				MPI_Pack(poblacion[num]->imagen, TAM_IMAGEN, rgb_type,//
            		sendbuf, size_ind*NEM, &posicion, MPI_COMM_WORLD);//

				// Fitness
				MPI_Pack(&(poblacion[num]->fitness), 1, MPI_DOUBLE,//
            		sendbuf, size_ind*NEM, &posicion, MPI_COMM_WORLD);//
			}
			
			MPI_Gather(sendbuf, size_ind*NEM, MPI_PACKED,
					   sendbufaux, size_ind*NEM, MPI_PACKED,
					   0, MPI_COMM_WORLD); // 

			if(name == 0){
				posicion = 0;

				for(int i=0; i<p; i++){
					for(int j=0; j<NEM; j++) {
						Individuo *ind = &(sendbuf2[i*NEM+j]);
						RGB *imagen = ind->imagen;
						MPI_Unpack(sendbufaux, size_ind*NEM*p, &posicion, imagen, TAM_IMAGEN, rgb_type, MPI_COMM_WORLD);
						double *fitness = &(ind->fitness);
						MPI_Unpack(sendbufaux, size_ind*NEM*p, &posicion, fitness, 1, MPI_DOUBLE, MPI_COMM_WORLD);
					}
				}
			}

			// El proceso 0 ordena por el fitness
			if (name == 0)
				qsort(sendbuf2, NEM*p, sizeof(Individuo), comp_fitness2);

			// Y envía los mejores a todos los procesos
			if(name == 0){
				posicion = 0;
				for(int i=0; i<NEM; i++){
					MPI_Pack(sendbuf2[i].imagen, TAM_IMAGEN, rgb_type,
						sendbuf, size_ind*NEM, &posicion, MPI_COMM_WORLD);
					MPI_Pack(&(sendbuf2[i].fitness), 1, MPI_DOUBLE,
						sendbuf, size_ind*NEM, &posicion, MPI_COMM_WORLD);
				}
			}

			MPI_Bcast(sendbuf, size_ind*NEM, MPI_PACKED, 0, MPI_COMM_WORLD);

			// Los procesos lo reciben y desempaquetan
			//printf("A [%d, %d] Primer pixel R=%u G=%u B=%u F=%f \n", name, g, sendbuf2[0].imagen[100].r, sendbuf2[0].imagen[100].g, sendbuf2[0].imagen[100].b, sendbuf2[0].fitness);
			// Cada proceso vuelve a colocar los individuos en las posiciones seleccionadas
			posicion = 0;
			Individuo *ind = malloc(sizeof(Individuo));
			for (int i=0; i<NEM; i++){
				MPI_Unpack(sendbuf, size_ind*NEM, &posicion, ind->imagen, TAM_IMAGEN, rgb_type, MPI_COMM_WORLD);
				MPI_Unpack(sendbuf, size_ind*NEM, &posicion, &(ind->fitness), 1, MPI_DOUBLE, MPI_COMM_WORLD);

				memmove(poblacion[posiciones[i]], ind, sizeof(Individuo));
			}
			free(ind);

			// Antes de liberar, tenemos que esperar a que todos los procesos copien
			MPI_Barrier(MPI_COMM_WORLD);
			//printf("D [%d, %d] Primer pixel R=%u G=%u B=%u F=%f \n", name, g, poblacion[posiciones[0]]->imagen[100].r, poblacion[posiciones[0]]->imagen[100].g, poblacion[posiciones[0]]->imagen[100].b, poblacion[posiciones[0]]->fitness);
			// Liberar la memoria
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

	// Los procesos empaquetan su mejor individuo
	posicion = 0;
	MPI_Pack(poblacion[0]->imagen, TAM_IMAGEN, rgb_type,
		sendbuf, size_ind, &posicion, MPI_COMM_WORLD);
	MPI_Pack(&(poblacion[0]->fitness), 1, MPI_DOUBLE,
		sendbuf, size_ind, &posicion, MPI_COMM_WORLD);

	MPI_Gather(sendbuf, size_ind, MPI_PACKED,
		sendbufaux, size_ind*p, MPI_PACKED, 0, MPI_COMM_WORLD);

	RGB *resultado = malloc(num_pixels * sizeof(RGB));

	if(name == 0){
		// Desempaquetamos y movemos a los mejores
		posicion = 0;
		for(int i=0; i<p; i++){
			MPI_Unpack(sendbufaux, size_ind*p, &posicion, mejores[i].imagen, TAM_IMAGEN, rgb_type, MPI_COMM_WORLD);
			MPI_Unpack(sendbufaux, size_ind*p, &posicion, &(mejores[i].fitness), 1, MPI_DOUBLE, MPI_COMM_WORLD);
		}

		// Los ordenamos
		qsort(mejores, p, sizeof(Individuo), comp_fitness2);

		memmove(resultado, mejores[0].imagen, num_pixels * sizeof(RGB));
	}

	// Enviamos la mejor imagen al resto de procesos (importante para suavizar)
	MPI_Bcast(resultado, 1, rgb_type, 0, MPI_COMM_WORLD);
		
	// Devuelve Imagen Resultante
	memmove(imagen_resultado, resultado, num_pixels * sizeof(RGB));	

	MPI_Barrier(MPI_COMM_WORLD);

	// Release memory
	if (name == 0)
		free(mejores);
	free(resultado);
	free(sendbuf);
	free(sendbuf2);
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