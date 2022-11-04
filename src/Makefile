CC = mpicc
CFLAGS = -O3 -Wall -std=c99 -g -fno-omit-frame-pointer

N_GEN = 100
T_POB = 100
N_PROC = 6

EXEC = sec
OUTFILE = ../output/out.txt
C_FILES = main.c imagen.c ga.c

sec: $(C_FILES)
	$(CC) $(CFLAGS) $(C_FILES) -o $(EXEC) -lm -DTIME -DDEBUG

test_sec:
	./$(EXEC) ../input/pitufo.ppm ../output/pitufo-out_$(N_GEN)_$(T_POB).ppm $(N_GEN) $(T_POB) >> $(OUTFILE)

test_mpi: sec
	mpirun -np $(N_PROC) ./$(EXEC) ../input/pitufo.ppm ../output/pitufo-out_$(N_GEN)_$(T_POB).ppm $(N_GEN) $(T_POB)

debug_mpi: sec
	valgrind --tool=memcheck --leak-check=yes --keep-debuginfo=yes mpirun -np $(N_PROC) ./$(EXEC) ../input/pitufo.ppm ../output/pitufo-out_$(N_GEN)_$(T_POB).ppm $(N_GEN) $(T_POB) >> $(OUTFILE)

clean:
	rm -f $(EXEC)
