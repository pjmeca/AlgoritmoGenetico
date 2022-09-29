#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../include/imagen.h"


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
/*
void suavizar(RGB* imagen_in, RGB* imagen_out, int a, int b, int c, int d, int e, int f, int g, int h, int i){
	
	RGB pixel1, pixel2, pixel3, pixel4, pixel5, pixel6, pixel7, pixel8, pixel9;
	
	pixel1_r = a<0 ? 1 : imagen_in[a];
	pixel1_g = a<0 ? 1 : imagen_in[a];
	pixel1_b = a<0 ? 1 : imagen_in[a];
	pixel2 = b<0 ? 1 : imagen_in[b];
	pixel3 = c<0 ? 1 : imagen_in[c];
	pixel4 = d<0 ? 1 : imagen_in[d];
	pixel5 = imagen_in[e];
	pixel6 = f<0 ? 1 : imagen_in[f];
	pixel7 = g<0 ? 1 : imagen_in[g];
	pixel8 = h<0 ? 1 : imagen_in[h];
	pixel9 = i<0 ? 1 : imagen_in[i];

	imagen_out[e].r = (pixel1.r + pixel2.r + pixel3.r +pixel4.r + pixel5.r+pixel6.r+pixel7.r+pixel8.r+pixel9.r)/9;
	imagen_out[e].g = (pixel1.g + pixel2.g + pixel3.g +pixel4.g + pixel5.g+pixel6.g+pixel7.g+pixel8.g+pixel9.g)/9;
	imagen_out[e].b = (pixel1.b + pixel2.b + pixel3.b +pixel4.b + pixel5.b+pixel6.b+pixel7.b+pixel8.b+pixel9.b)/9;
}*/

void suavizar(int ancho, int alto, RGB *img, RGB *img_out)
{
	// Aplicar tecnica "mean-filter" para suavizar la imagen resultante

	// Usar dos bucles for y muchos if con los 9 casos posibles (4 esquinas, 4 laterales o centro)
	for(int i=0; i<alto*ancho; i+=ancho){
		for(int j=0; j<ancho; j++){

			int p[9] = {i+j-ancho-1, i+j-ancho, i+j-ancho+1, i+j-1, i+j, i+j+1, i+j+ancho-1, i+j+ancho, i+j+ancho+1};

			// Esquina superior izquierda
			if(i+j == 0){
				//suavizar(imagen, imagen_out, i+j-ancho-1, i+j-ancho, i+j-ancho+1, i+j-1, i+j, i+j+1, i+j+ancho-1, i+j+ancho, i+j+ancho+1);
				img_out[i+j].r = (img[p[4]].r+img[p[5]].r+img[p[7]].r+img[p[8]].r)/4;
				img_out[i+j].g = (img[p[4]].g+img[p[5]].g+img[p[7]].g+img[p[8]].g)/4;
				img_out[i+j].b = (img[p[4]].b+img[p[5]].b+img[p[7]].b+img[p[8]].b)/4;
			}

			// Esquina superior derecha
			else if (i+j == ancho-1){
				img_out[i+j].r = (img[p[3]].r+img[p[4]].r+img[p[6]].r+img[p[7]].r)/4;
				img_out[i+j].g = (img[p[3]].g+img[p[4]].g+img[p[6]].g+img[p[7]].g)/4;
				img_out[i+j].b = (img[p[3]].b+img[p[4]].b+img[p[6]].b+img[p[7]].b)/4;
			}

			// Superior
			else if (i == 0){
				img_out[i+j].r = (img[p[3]].r+img[p[4]].r+img[p[5]].r+img[p[6]].r+img[p[7]].r+img[p[8]].r)/6;
				img_out[i+j].g = (img[p[3]].g+img[p[4]].g+img[p[5]].g+img[p[6]].g+img[p[7]].g+img[p[8]].g)/6;
				img_out[i+j].b = (img[p[3]].b+img[p[4]].b+img[p[5]].b+img[p[6]].b+img[p[7]].b+img[p[8]].b)/6;
			}

			// Esquina inferior izquierda
			else if(j == 0 && i == (alto-1)*ancho){ 
				img_out[i+j].r = (img[p[1]].r+img[p[2]].r+img[p[4]].r+img[p[5]].r)/4;
				img_out[i+j].g = (img[p[1]].g+img[p[2]].g+img[p[4]].g+img[p[5]].g)/4;
				img_out[i+j].b = (img[p[1]].b+img[p[2]].b+img[p[4]].b+img[p[5]].b)/4;
			}

			// Esquina inferior derecha
			else if(i+j == alto*ancho-1){
				img_out[i+j].r = (img[p[0]].r+img[p[1]].r+img[p[3]].r+img[p[4]].r)/4;
				img_out[i+j].g = (img[p[0]].g+img[p[1]].g+img[p[3]].g+img[p[4]].g)/4;
				img_out[i+j].b = (img[p[0]].b+img[p[1]].b+img[p[3]].b+img[p[4]].b)/4;
			}

			// Lateral izquierdo
			else if(j == 0){
				img_out[i+j].r = (img[p[1]].r+img[p[2]].r+img[p[4]].r+img[p[5]].r+img[p[7]].r+img[p[8]].r)/6;
				img_out[i+j].g = (img[p[1]].g+img[p[2]].g+img[p[4]].g+img[p[5]].g+img[p[7]].g+img[p[8]].g)/6;
				img_out[i+j].b = (img[p[1]].b+img[p[2]].b+img[p[4]].b+img[p[5]].b+img[p[7]].b+img[p[8]].b)/6;
			}

			// Lateral derecho
			else if(j == ancho-1){
				img_out[i+j].r = (img[p[0]].r+img[p[1]].r+img[p[3]].r+img[p[4]].r+img[p[6]].r+img[p[7]].r)/6;
				img_out[i+j].g = (img[p[0]].g+img[p[1]].g+img[p[3]].g+img[p[4]].g+img[p[6]].g+img[p[7]].g)/6;
				img_out[i+j].b = (img[p[0]].b+img[p[1]].b+img[p[3]].b+img[p[4]].b+img[p[6]].b+img[p[7]].b)/6;
			}

			// Inferior
			else if(i == (alto-1)*ancho){
				img_out[i+j].r = (img[p[0]].r+img[p[1]].r+img[p[2]].r+img[p[3]].r+img[p[4]].r+img[p[5]].r)/6;
				img_out[i+j].g = (img[p[0]].g+img[p[1]].g+img[p[2]].g+img[p[3]].g+img[p[4]].g+img[p[5]].g)/6;
				img_out[i+j].b = (img[p[0]].b+img[p[1]].b+img[p[2]].b+img[p[3]].b+img[p[4]].b+img[p[5]].b)/6;
			}

			// Centro
			else{
				//suavizar(imagen, imagen_out, i+j-ancho-1, i+j-ancho, i+j-ancho+1, i+j-1, i+j, i+j+1, i+j+ancho-1, i+j+ancho, i+j+ancho+1);
				img_out[i+j].r = (img[p[0]].r + img[p[1]].r + img[p[2]].r +img[p[3]].r + img[p[4]].r+img[p[5]].r+img[p[6]].r+img[p[7]].r+img[p[8]].r)/9;
				img_out[i+j].g = (img[p[0]].g + img[p[1]].g + img[p[2]].g +img[p[3]].g + img[p[4]].g+img[p[5]].g+img[p[6]].g+img[p[7]].g+img[p[8]].g)/9;
				img_out[i+j].b = (img[p[0]].b + img[p[1]].b + img[p[2]].b +img[p[3]].b + img[p[4]].b+img[p[5]].b+img[p[6]].b+img[p[7]].b+img[p[8]].b)/9;
			}

		}
	}
}
