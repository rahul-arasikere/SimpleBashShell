#include "read_matrix.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_array(int *A, int n, int m)
{
    int i;
    for (i = 0; i < n; i++)
    {
        int j;
        for (j = 0; j < m; j++)
        {
            printf("%d ", A[i * m + j]);
        }
        printf("\n");
    }
}

int *read_matrix(int *r, int *c)
{
    unsigned long bufsize = MAX_BUF_SIZE;
    unsigned long long int _count = 0;
    char *pch;
    int column;
    int row = 0;
    int *matrix = (int *)malloc(bufsize * sizeof(int));
    char buffer[MAX_BUF_SIZE];
    while (fgets(buffer, MAX_BUF_SIZE, stdin) != NULL)
    {
        if (strcmp(buffer, "\n") == 0)
        {
            break;
        }
        buffer[strcspn(buffer, "\r\n")] = 0;
        // ^edge case handling...
        column = 0;
        pch = strtok(buffer, " ");
        do
        {
            int element = strtol(pch, NULL, 10);
            if (row == 0)
            {
                matrix[column] = element;
            }
            else
            {
                matrix[row * *c + column] = element;
            }
            if ((++_count) > bufsize)
            {
                bufsize *= 2;
                matrix = (int *)realloc(matrix, bufsize * sizeof(int));
            }
            column += 1;

        } while ((pch = strtok(NULL, " ")) != NULL);
        if (*c != column && *c != 0)
        {
            free(matrix);
            *c = -1;
            *r = -1;
        }
        *c = column;
        row++;
    }
    *r = row;
    return matrix;
}