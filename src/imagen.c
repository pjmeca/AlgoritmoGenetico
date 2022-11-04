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

	RGB *imagen = (RGB *) malloc(size * 3*sizeof(char));
	assert(imagen);
	
	for(i=0; i<size; i++) {
		n = fscanf(fd, "%d %d %d", &red, &green, &blue);
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
	for(i=0; i<size; i++) {
		fprintf(fd, "%d %d %d ", imagen[i].r, imagen[i].g, imagen[i].b);
		if((i+1) % 18 == 0) {
			fprintf(fd, "\n");
		}
	}
	fclose(fd);
}

// Aplicar tecnica "mean-filter" para suavizar la imagen resultante
void suavizar(int ancho, int alto, RGB *img, RGB *img_out, MPI_Datatype rgb_type)
{
	int name, p;
	MPI_Comm_rank(MPI_COMM_WORLD, &name);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	int inicio = name*alto*ancho/p;
	int final;
	if(name != p-1)
		final = (name+1)*alto*ancho/p;
	else
		final = alto*ancho;

	RGB *img_aux;
	if (name == 0)
		img_aux = img_out;
	else 
	 	img_aux = (RGB *) malloc(ancho*alto*sizeof(RGB));
	// Usar dos bucles for y muchos if con los 9 casos posibles (4 esquinas, 4 laterales o centro)
	for(int i=inicio; i<final; i+=ancho){
		int limite = (i+ancho) < final ? ancho : (final-i);
		for(int j=0; j<limite; j++){

			int p[9] = {i+j-ancho-1, i+j-ancho, i+j-ancho+1, i+j-1, i+j, i+j+1, i+j+ancho-1, i+j+ancho, i+j+ancho+1};

			// Esquina superior izquierda
			if(i+j == 0){
				//suavizar(imagen, imagen_out, i+j-ancho-1, i+j-ancho, i+j-ancho+1, i+j-1, i+j, i+j+1, i+j+ancho-1, i+j+ancho, i+j+ancho+1);
				img_aux[i+j].r = (img[p[4]].r+img[p[5]].r+img[p[7]].r+img[p[8]].r)/4;
				img_aux[i+j].g = (img[p[4]].g+img[p[5]].g+img[p[7]].g+img[p[8]].g)/4;
				img_aux[i+j].b = (img[p[4]].b+img[p[5]].b+img[p[7]].b+img[p[8]].b)/4;
			}

			// Esquina superior derecha
			else if (i+j == ancho-1){
				img_aux[i+j].r = (img[p[3]].r+img[p[4]].r+img[p[6]].r+img[p[7]].r)/4;
				img_aux[i+j].g = (img[p[3]].g+img[p[4]].g+img[p[6]].g+img[p[7]].g)/4;
				img_aux[i+j].b = (img[p[3]].b+img[p[4]].b+img[p[6]].b+img[p[7]].b)/4;
			}

			// Superior
			else if (i == 0){
				img_aux[i+j].r = (img[p[3]].r+img[p[4]].r+img[p[5]].r+img[p[6]].r+img[p[7]].r+img[p[8]].r)/6;
				img_aux[i+j].g = (img[p[3]].g+img[p[4]].g+img[p[5]].g+img[p[6]].g+img[p[7]].g+img[p[8]].g)/6;
				img_aux[i+j].b = (img[p[3]].b+img[p[4]].b+img[p[5]].b+img[p[6]].b+img[p[7]].b+img[p[8]].b)/6;
			}

			// Esquina inferior izquierda
			else if(j == 0 && i == (alto-1)*ancho){ 
				img_aux[i+j].r = (img[p[1]].r+img[p[2]].r+img[p[4]].r+img[p[5]].r)/4;
				img_aux[i+j].g = (img[p[1]].g+img[p[2]].g+img[p[4]].g+img[p[5]].g)/4;
				img_aux[i+j].b = (img[p[1]].b+img[p[2]].b+img[p[4]].b+img[p[5]].b)/4;
			}

			// Esquina inferior derecha
			else if(i+j == alto*ancho-1){
				img_aux[i+j].r = (img[p[0]].r+img[p[1]].r+img[p[3]].r+img[p[4]].r)/4;
				img_aux[i+j].g = (img[p[0]].g+img[p[1]].g+img[p[3]].g+img[p[4]].g)/4;
				img_aux[i+j].b = (img[p[0]].b+img[p[1]].b+img[p[3]].b+img[p[4]].b)/4;
			}

			// Lateral izquierdo
			else if(j == 0){
				img_aux[i+j].r = (img[p[1]].r+img[p[2]].r+img[p[4]].r+img[p[5]].r+img[p[7]].r+img[p[8]].r)/6;
				img_aux[i+j].g = (img[p[1]].g+img[p[2]].g+img[p[4]].g+img[p[5]].g+img[p[7]].g+img[p[8]].g)/6;
				img_aux[i+j].b = (img[p[1]].b+img[p[2]].b+img[p[4]].b+img[p[5]].b+img[p[7]].b+img[p[8]].b)/6;
			}

			// Lateral derecho
			else if(j == ancho-1){
				img_aux[i+j].r = (img[p[0]].r+img[p[1]].r+img[p[3]].r+img[p[4]].r+img[p[6]].r+img[p[7]].r)/6;
				img_aux[i+j].g = (img[p[0]].g+img[p[1]].g+img[p[3]].g+img[p[4]].g+img[p[6]].g+img[p[7]].g)/6;
				img_aux[i+j].b = (img[p[0]].b+img[p[1]].b+img[p[3]].b+img[p[4]].b+img[p[6]].b+img[p[7]].b)/6;
			}

			// Inferior
			else if(i == (alto-1)*ancho){
				img_aux[i+j].r = (img[p[0]].r+img[p[1]].r+img[p[2]].r+img[p[3]].r+img[p[4]].r+img[p[5]].r)/6;
				img_aux[i+j].g = (img[p[0]].g+img[p[1]].g+img[p[2]].g+img[p[3]].g+img[p[4]].g+img[p[5]].g)/6;
				img_aux[i+j].b = (img[p[0]].b+img[p[1]].b+img[p[2]].b+img[p[3]].b+img[p[4]].b+img[p[5]].b)/6;
			}

			// Centro
			else{
				//suavizar(imagen, imagen_out, i+j-ancho-1, i+j-ancho, i+j-ancho+1, i+j-1, i+j, i+j+1, i+j+ancho-1, i+j+ancho, i+j+ancho+1);
				img_aux[i+j].r = (img[p[0]].r + img[p[1]].r + img[p[2]].r +img[p[3]].r + img[p[4]].r+img[p[5]].r+img[p[6]].r+img[p[7]].r+img[p[8]].r)/9;
				img_aux[i+j].g = (img[p[0]].g + img[p[1]].g + img[p[2]].g +img[p[3]].g + img[p[4]].g+img[p[5]].g+img[p[6]].g+img[p[7]].g+img[p[8]].g)/9;
				img_aux[i+j].b = (img[p[0]].b + img[p[1]].b + img[p[2]].b +img[p[3]].b + img[p[4]].b+img[p[5]].b+img[p[6]].b+img[p[7]].b+img[p[8]].b)/9;
			}

		}
	}

	if (name == 0){
		for (int i=1; i<p; i++){
			if(i != p-1)
				MPI_Recv(&img_out[i*alto*ancho/p], ancho*alto/p, rgb_type, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			else
				MPI_Recv(&img_out[i*alto*ancho/p], ancho*alto - (p-1)*alto*ancho/p, rgb_type, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	} else {
		MPI_Send(&img_aux[inicio], final-inicio, rgb_type, 0, 0, MPI_COMM_WORLD);
		free(img_aux);
	}
}
