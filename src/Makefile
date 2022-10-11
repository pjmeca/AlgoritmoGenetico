CC = gcc
CFLAGS = -O3 -Wall -std=c99 -g -fopenmp 

N_GEN = 2000
T_POB = 256

EXEC = sec
OUTFILE = ../output/out.txt
C_FILES = main.c imagen.c ga.c

sec: $(C_FILES)
	$(CC) $(CFLAGS) $(C_FILES) -o $(EXEC) -lm -DTIME -DDEBUG

test_sec:
	./$(EXEC) ../input/pitufo.ppm ../output/pitufo-out_$(N_GEN)_$(T_POB).ppm $(N_GEN) $(T_POB) >> $(OUTFILE)

clean:
	rm -f $(EXEC)
