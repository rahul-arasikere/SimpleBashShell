#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "read_matrix.h"

int main(int argc, char *argv[])
{
    int num_rows_A = 0, num_columns_A = 0, num_rows_B = 0, num_columns_B = 0;
    int *A = read_matrix(&num_rows_A, &num_columns_A);
    if (A == NULL)
    {
        fprintf(stderr, "matmult_p: enter a valid matrix!\n");
        exit(EXIT_FAILURE);
    }
    int *B = read_matrix(&num_rows_B, &num_columns_B);
    if (B == NULL)
    {
        free(A);
        fprintf(stderr, "matmult_p: enter a valid matrix!\n");
        exit(EXIT_FAILURE);
    }
    if (num_columns_A != num_rows_B)
    {
        free(A);
        free(B);
        fprintf(stderr, "matmult_p: row column mismatch!\n");
        exit(EXIT_FAILURE);
    }
    int sharedMatID = shmget(IPC_PRIVATE, num_rows_A * num_columns_B * sizeof(int), 0600);
    if (sharedMatID < 0)
    {
        fprintf(stderr, "matmult_p: failed to create shared memory space!\n");
        exit(EXIT_FAILURE);
    }
    int *output = shmat(sharedMatID, (void *)0, 0);
    if (output == (void *)-1)
    {
        fprintf(stderr, "matmult_p: failed to create shared memory space!\n");
        exit(EXIT_FAILURE);
    }
    int i;
    for (i = 0; i < num_rows_A * num_columns_B; i++)
    {
        output[i] = 0;
    }

    int sharedMatAID, sharedMatBID;
    int *sharedMatA;
    int *sharedMatB;
    sharedMatAID = shmget(IPC_PRIVATE, num_columns_A * num_rows_A * sizeof(int), 0600);
    sharedMatBID = shmget(IPC_PRIVATE, num_columns_B * num_rows_B * sizeof(int), 0600);
    if (sharedMatAID < 0 || sharedMatBID < 0)
    {
        fprintf(stderr, "matmult_p: failed to create shared memory space!\n");
        exit(EXIT_FAILURE);
    }
    sharedMatA = (int *)shmat(sharedMatAID, 0, 0);
    if (sharedMatA == (void *)-1)
    {
        fprintf(stderr, "matmult_p: failed to create shared memory space!\n");
        exit(EXIT_FAILURE);
    }
    sharedMatB = (int *)shmat(sharedMatBID, 0, 0);
    if (sharedMatB == (void *)-1)
    {
        fprintf(stderr, "matmult_p: failed to create shared memory space!\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < num_columns_A * num_rows_A; i++)
    {
        sharedMatA[i] = A[i];
    }
    for (i = 0; i < num_columns_B * num_rows_B; i++)
    {
        sharedMatB[i] = B[i];
    }
    int r;
    int c;
    i = 0;
    for (r = 0; r < num_rows_A; r++)
    {
        for (c = 0; c < num_columns_B; c++)
        {
            int forkId = fork();
            if (forkId == 0)
            {
                char numColumnsAARG[15];
                char numRowsAARG[15];
                char numRowsBARG[15];
                char numColumnsBARG[15];
                char idx[15];
                char out[15];
                char aid[15];
                char bid[15];
                sprintf(numRowsAARG, "%d", num_rows_A);
                sprintf(numColumnsAARG, "%d", num_columns_A);
                sprintf(numColumnsBARG, "%d", num_rows_B);
                sprintf(numRowsBARG, "%d", num_columns_B);
                sprintf(idx, "%d", i);
                sprintf(out, "%d", sharedMatID);
                sprintf(aid, "%d", sharedMatAID);
                sprintf(bid, "%d", sharedMatBID);
                char *args_to_multiply[] = {"./multiply", numColumnsAARG, numRowsAARG, numColumnsBARG, numRowsBARG, idx, out, aid, bid, NULL};
                execvp(args_to_multiply[0], args_to_multiply);
                fprintf(stderr, "matmult_p: child error!\n");
                exit(errno);
            }
            i++;
        }
    }
    bool _errors = false;
    for (int i = 0; i < num_columns_A * num_rows_B; i++)
    {
        int stat;
        wait(&stat);
        if (!WIFEXITED(stat) || (WIFEXITED(stat) && WEXITSTATUS(stat) != EXIT_SUCCESS))
            _errors = true;
    }
    if (_errors)
    {
        printf("matmult_p: error in one of the children!\n");
        exit(EXIT_FAILURE);
    }
    print_array(output, num_rows_A, num_columns_B);
    shmctl(sharedMatID, IPC_RMID, 0);
    shmctl(sharedMatAID, IPC_RMID, 0);
    shmctl(sharedMatBID, IPC_RMID, 0);
    shmdt((void *)output);
    shmdt((void *)sharedMatA);
    shmdt((void *)sharedMatB);
    free(A);
    free(B);
    return EXIT_SUCCESS;
}