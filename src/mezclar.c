void mezclar(Individuo **poblacion, int izq, int med, int der)
{	
	int i, j, k;
	
	Individuo **pob = (Individuo **) malloc((der - izq)*sizeof(Individuo *));
	assert(pob);
	
	for(i = 0; i < (der - izq); i++) {
		pob[i] = (Individuo *) malloc(sizeof(Individuo));
	}
	
	k = 0;
	i = izq;
	j = med;
	// Comparar las dos mitades ordenadas e ir poniendo el valor de menor fitness
	while( (i < med) && (j < der) ) {
		if (poblacion[i]->fitness < poblacion[j]->fitness) {
			// copiar poblacion[i++] en pob[k++]
			pob[k]->fitness = poblacion[i]->fitness;
			pob[k]->imagen = poblacion[i]->imagen;
			k++; i++;
		}
		else {
			// copiar poblacion[j++] en pob[k++]
			pob[k]->fitness = poblacion[j]->fitness;
			pob[k]->imagen = poblacion[j]->imagen;
			k++; j++;
		}
	}
	
	// Copiar el resto
	for(; i < med; i++) {
		// copiar poblacion[i] en pob[k++]
		pob[k]->fitness = poblacion[i]->fitness;
		pob[k]->imagen = poblacion[i]->imagen;
		k++;
	}
	for(; j < der; j++) {
		// copiar poblacion[j] en pob[k++]
		pob[k]->fitness = poblacion[j]->fitness;
		pob[k]->imagen = poblacion[j]->imagen;
		k++;
	}
	
	// Devolverlo al array poblacion
	
	for(i = 0; i < (der - izq); i++) {
		// copiar pob[i] en poblacion[i + izq]
		// ...
		// liberar individuo 'i'
		poblacion[i+izq]->fitness = pob[i]->fitness;
		poblacion[i+izq]->imagen = pob[i]->imagen;
		free(pob[i]);
	}
	free(pob);
}

void mergeSort(Individuo **poblacion, int izq, int der)
{
	int med = (izq + der)/2;
	if ((der - izq) < 2) return;

	#pragma omp parallel
	{
		#pragma omp single
		{
			#pragma omp task shared(poblacion) firstprivate(izq, med)
			mergeSort(poblacion, izq, med);
			#pragma omp task shared(poblacion) firstprivate(med, der)
			mergeSort(poblacion, med, der);

			#pragma omp taskwait
			mezclar(poblacion, izq, med, der);
		} // el resto de hilos empiezan a ejecutar las tareas aquí
	}
	
}