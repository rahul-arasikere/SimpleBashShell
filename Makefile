# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall

all: myshell matformatter multiply matmult_p matmult_t

%: %.c read_matrix.c; gcc -o $@ $^ $(CFLAGS)

test: ;

clean:; rm -rf *.so *.o *.a myshell matmult_p matmult_t multiply matformatter