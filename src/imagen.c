#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../include/imagen.h"

#define TAM_FILA_SELECCION_SUAVIZADO 3 // tamaño de la fila del cuadrado de suavizado

RGB *leer_ppm(const char *file, int *ancho, int *alto, int *max)
{
	int i, n;
	FILE *fd;
	
	char c, b[100];
	int red, green, blue;
	//unsigned char red, green, blue; añadido por nosotros
	
	fd = fopen(file, "r");
	
	n = fscanf(fd, "%[^\n] ", b);
	if(b[0] != 'P' || b[1] != '3') {
		printf("%s no es una imagen PPM\n", file);
		exit(0);
	}
	
	printf("Leyendo fichero PPM %s\n", file);
	n = fscanf(fd, "%c", &c);
	while(c == '#') {
		n = fscanf(fd, "%[^\n] ", b);
		printf("%s\n", b);
		n = fscanf(fd, "%c", &c);
	}
	
	ungetc(c, fd);
	n = fscanf(fd, "%d %d %d", ancho, alto, max);
	assert(n == 3);
	
	int size = (*ancho)*(*alto);

	//RGB *imagen = (RGB *) malloc(size*sizeof(char));
	RGB *imagen = (RGB *) malloc(size * 3*sizeof(char)); // no reservaba lo suficiente
	assert(imagen);
	
	//for(i=0; i<=size; i++) {
	for(i=0; i<size; i++) { // se salía del array
		n = fscanf(fd, "%d %d %d", &red, &green, &blue);
		//n = fscanf(fd, "%c %c %c", &red, &green, &blue); // añadido por nosotros
		assert(n == 3);
		
		imagen[i].r = red;
		imagen[i].g = green;
		imagen[i].b = blue;
	}
	
	fclose(fd);
	return imagen;
}

void escribir_ppm(const char *fichero, int ancho, int alto, int max, const RGB *imagen)
{
	int i;
	FILE *fd;
	
	fd = fopen(fichero, "w");
	
	fprintf(fd, "P3\n");
	fprintf(fd, "%d %d\n%d\n", ancho, alto, max);
	
	int size = alto*ancho;
	//for(i=0; i<=size; i++) {
	for(i=0; i<size; i++) { // añadido por nosotros
		fprintf(fd, "%d %d %d ", imagen[i].r, imagen[i].g, imagen[i].b);
		if((i+1) % 18 == 0) {
			fprintf(fd, "\n");
		}
	}
	fclose(fd);
}

void suavizar(int ancho, int alto, RGB *imagen)
{
	// Aplicar tecnica "mean-filter" para suavizar la imagen resultante
	int media_r,media_g,media_b;
    for(int i =0;i<=(alto*ancho);i++){
			// no se sale por la derecha 					   // no se sale por abajo
        if(i%ancho > (ancho - TAM_FILA_SELECCION_SUAVIZADO) || i >= (ancho*alto - 2*ancho))
			continue;

		media_r = media_g = media_b = 0;

		for(int tam_i=0; tam_i<TAM_FILA_SELECCION_SUAVIZADO; tam_i++) {
			for(int tam_j=0; tam_j<TAM_FILA_SELECCION_SUAVIZADO; tam_j++){
				media_r += imagen[i+tam_j+tam_i*ancho].r;
				media_g += imagen[i+tam_j+tam_i*ancho].g;
				media_b += imagen[i+tam_j+tam_i*ancho].b;
			}
		}

        imagen[i].r = media_r / (TAM_FILA_SELECCION_SUAVIZADO*TAM_FILA_SELECCION_SUAVIZADO);
        imagen[i].g = media_g / (TAM_FILA_SELECCION_SUAVIZADO*TAM_FILA_SELECCION_SUAVIZADO);
        imagen[i].b = media_b / (TAM_FILA_SELECCION_SUAVIZADO*TAM_FILA_SELECCION_SUAVIZADO);
    }
}
