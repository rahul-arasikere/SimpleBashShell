#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_matrix.h"

ucontext_t main_thread;
ucontext_t *child_threads;

int *A;
int *B;
int *C;

int num_rows_a = 0, num_cols_a = 0, num_rows_b = 0, num_cols_b = 0;

void my_thr_create(void (*func)(int), int thr_id);
void multiply_thread(int);

void my_thr_create(void (*func)(int), int thr_id)
{
    if (getcontext(&child_threads[thr_id]) == -1)
    {
        fprintf(stderr, "matmult_t: thread id already in use!\n");
        exit(EXIT_FAILURE);
    }
    char *stack = (char *)malloc(16384 * sizeof(char));
    child_threads[thr_id].uc_stack.ss_sp = stack + 8192;
    child_threads[thr_id].uc_stack.ss_size = 8192;
    child_threads[thr_id].uc_link = &main_thread;
    makecontext(&child_threads[thr_id], (void (*)(void))func, 3, thr_id);
}

void multiply_thread(int thr_id)
{
    int row = (thr_id - 1) / num_cols_b;
    int column = (int)(thr_id - 1) % num_cols_b;
    int i = 0;
    for (i = 0; i < num_cols_a; i++)
    {
        C[row * num_cols_b + column] += (A[row * num_cols_a + i] * B[column + num_cols_b * i]);
    }
    if (thr_id == ((num_cols_a * num_rows_b) - 1))
    {
        if (swapcontext(&child_threads[thr_id], &child_threads[0]) == -1)
        {
            fprintf(stderr, "matmult_t: failed to start thread!\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (swapcontext(&child_threads[thr_id], &child_threads[thr_id + 1]) == -1)
        {
            fprintf(stderr, "matmult_t: failed to start thread!\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    getcontext(&main_thread);
    A = read_matrix(&num_rows_a, &num_cols_a);
    if (A == NULL)
    {
        fprintf(stderr, "matmult_p: enter a valid matrix!\n");
        exit(EXIT_FAILURE);
    }
    B = read_matrix(&num_rows_b, &num_cols_b);
    if (B == NULL)
    {
        fprintf(stderr, "matmult_p: enter a valid matrix!\n");
        exit(EXIT_FAILURE);
    }
    if (num_cols_a != num_rows_b)
    {
        fprintf(stderr, "matmult_p: row column mismatch!\n");
        exit(EXIT_FAILURE);
    }
    int matrix_size = num_cols_a * num_rows_b;
    C = (int *)malloc(matrix_size * sizeof(int));
    memset(C, 0, matrix_size * sizeof(int));
    child_threads = (ucontext_t *)malloc(matrix_size * sizeof(ucontext_t));
    int i;
    for (i = 0; i < matrix_size; i++)
    {
        my_thr_create(&multiply_thread, i);
    }
    if (swapcontext(&main_thread, &child_threads[0]) == -1)
    {
        fprintf(stderr, "matmult_t: failed to start thread!\n");
        exit(EXIT_FAILURE);
    }
    print_array(C, num_rows_a, num_cols_b);
    return EXIT_SUCCESS;
}