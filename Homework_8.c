#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "timing.h"
#include <mpi.h>

//randomally generate a matrix MxN with 1's and 0's
void generateMatrix(int M, int N, int matrix[M][N]){
    int i, j;
    srand(time(NULL));

    for(i=0; i<M; i++){
        for(j=0; j<N; j++){
            matrix[i][j] = rand() % 2;
        }
    }
}

//prints matrix
void printMatrix(int M, int N, int matrix[M][N]){
    int i, j;

    for(i=0; i<M; i++){
        for(j=0; j<N; j++){
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

//Game of life simulation
void main(int argc, char *argv[]){
    //checks command line args
    if(argc != 4){
        printf("Usage: mpirun -np <num of procs> ./%s <M> <N> <num of iters>\n", argv[0]);
        exit(1);
    }

    //converts command line args to ints
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int iterations = atoi(argv[3]);

    int matrix[M][N];//out matrix

    int rank, size, i, j;// process rank and total proccess
    int neighbors[8], neighbor_count, created; //neighbor cells, totla neighbors, heighbors created flag

    int cell, state, alive_count;//cell value, state and count of alive neighbor cells
    
    //initalizes MPI section and gets the current processes rank and total number of proccesses
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Status statuses[8];//recv statuses
    MPI_Request requests[8];//non blocking requests

    //makes sure that the numper of procs in equal to the number of elements in the matrix
    if(size != M*N){
        printf("number of procs must be equal to the M*N\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }

    //master proccess generates the matrix
    if(rank == 0){
        generateMatrix(M, N, matrix);
        printf("Start:\n");
        printf("------\n");
        printMatrix(M, N, matrix);
    }

    //scatter cells to proccesses
    MPI_Scatter(&matrix, 1, MPI_INT, &cell, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //gets neighbor cells 
    neighbor_count = 0;
    created = 0;
    for(i=0; i<M; i++){
        for(j=0; j<N; j++){
            if(rank == (i*N + j)){
                if(i-1 >=0){
                    if(j-1 >=0){
                        neighbors[neighbor_count] = (i-1)*N + (j-1);
                        neighbor_count++;
                    }

                    neighbors[neighbor_count] = (i-1)*N + j;
                    neighbor_count++;

                    if(j+1 < N){
                        neighbors[neighbor_count] = (i-1)*N + (j+1);
                        neighbor_count++;
                    }
                }

                if(j-1 >=0){
                    neighbors[neighbor_count] = i*N + (j-1);
                    neighbor_count++;
                }

                if(j+1 < N){
                    neighbors[neighbor_count] = i*N + (j+1);
                    neighbor_count++;
                }

                if(i+1 < M){
                    if(j-1 >=0){
                        neighbors[neighbor_count] = (i+1)*N + (j-1);
                        neighbor_count++;
                    }

                    neighbors[neighbor_count] = (i+1)*N + j;
                    neighbor_count++;

                    if(j+1 < N){
                        neighbors[neighbor_count] = (i+1)*N + (j+1);
                        neighbor_count++;
                    }
                }
                created = 1;
                break;
            }
        }

        if(created){
            break;
        }
    }

    timing_start();
    //starts game
    for(i=0; i<iterations; i++){
        for(j=0; j<neighbor_count; j++){
            MPI_Isend(&cell, 1, MPI_INT, neighbors[j], 0, MPI_COMM_WORLD, &requests[j]); //broadcasts cell state to neighbors
        }

        alive_count = 0;
        //gets neighbor states
        for(j=0; j<neighbor_count; j++){
            MPI_Recv(&state, 1, MPI_INT, neighbors[j], 0, MPI_COMM_WORLD, &statuses[j]);
            if(state == 1){
                alive_count++;
            }
        }

        //decides if cell lives or dies
        if(alive_count == 2 || alive_count == 3){
            cell = 1;
        }
        else{
            cell = 0;
        }

        MPI_Gather(&cell, 1, MPI_INT, &matrix, 1, MPI_INT, 0, MPI_COMM_WORLD); //gather all cels to make the resulting matrix into the master proccess
        
        //master proccess prints the resulting matrix after round i
        if(rank == 0){
            printf("Round %d:\n", i);
            printf("---------\n");
            printMatrix(M, N, matrix);
        }
    }
    timing_stop();

    if(rank == 0){
        print_timing();
    }

    MPI_Finalize();
}