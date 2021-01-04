#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>

int main(int argc, char **argv)
{
    if (argc < 9)
    {
        fprintf(stderr, "multiply: invalid arguments passed!\n");
        exit(EXIT_FAILURE);
    }
    int num_cols_a = atoi(argv[1]);
    int num_rows_a = atoi(argv[2]);
    int num_cols_b = atoi(argv[3]);
    int num_rows_b = atoi(argv[4]);
    int idx = atoi(argv[5]);
    int sharedMatID = atoi(argv[6]);
    int sharedMatAID = atoi(argv[7]);
    int sharedMatBID = atoi(argv[8]);
    int *A, *B, *C;
    C = (int *)shmat(sharedMatID, 0, 0);
    if (C == (void *)-1)
    {
        fprintf(stderr, "multiply: error attaching shared memory!\n");
        exit(EXIT_FAILURE);
    }
    A = (int *)shmat(sharedMatAID, 0, 0);
    if (A == (void *)-1)
    {
        fprintf(stderr, "multiply: error attaching shared memory!\n");
        exit(EXIT_FAILURE);
    }
    B = (int *)shmat(sharedMatBID, 0, 0);
    if (B == (void *)-1)
    {
        fprintf(stderr, "multiply: error attaching shared memory!\n");
        exit(EXIT_FAILURE);
    }
    int row = (idx) / num_cols_b;
    int column = (int)(idx) % num_cols_b;
    int i = 0;
    for (i = 0; i < num_cols_a; i++)
    {
        C[row * num_cols_b + column] += (A[row * num_cols_a + i] * B[column + num_cols_b * i]);
    }
    shmdt(A);
    shmdt(B);
    shmdt(C);
    exit(EXIT_SUCCESS);
}