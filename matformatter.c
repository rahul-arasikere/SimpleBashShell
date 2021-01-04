#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "read_matrix.h"

int main()
{
    int rows = 0, columns = 0;
    int *A = read_matrix(&rows, &columns);
    if (A == NULL)
    {
        fprintf(stderr, "matformatter: invalid matrix inputted!\n");
        exit(EXIT_FAILURE);
    }
    int i, j;

    for (j = 0; j < columns; ++j)
    {
        for (i = 0; i < rows; ++i)
        {
            printf("%d ", A[j + i * columns]);
        }
        printf("\n");
    }
    free(A);
}