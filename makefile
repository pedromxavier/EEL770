# COC473 - Álgebra Linear Computacional @ ECI/UFRJ
# Macros
cc = gcc
# args =
# Makefile

all: matrix1 matrix2 matrix3

matrix1: matrixlib.o main1.o
	$(cc) -o shellmatrix1 $(args) matrixlib.o main1.o

clean:
	rm *.o *.mod
# End Makefile
