run: Homework_8.out 

Homework_8.out: Homework_8.c timing.c
        mpicc Homework_8.c timing.c -o Homework_8.out

clean:
        rm *.out
