# Makefile for UD CISC user-level thread library

CC = gcc
CFLAGS = -g

LIBOBJS = t_lib.o 

TSTOBJS = test00.o test03.o test05.o test06.o test08.o

# specify the executable 

EXECS = test00 test03 test05 test06 test08

# specify the source files

LIBSRCS = t_lib.c

TSTSRCS = test00.c test03.c test05.c test06.c test08.c

# ar creates the static thread library

t_lib.a: ${LIBOBJS} Makefile
	ar rcs t_lib.a ${LIBOBJS}

# here, we specify how each file should be compiled, what
# files they depend on, etc.

t_lib.o: t_lib.c t_lib.h Makefile
	${CC} ${CFLAGS} -c t_lib.c

test00.o: test00.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test00.c

test00: test00.o t_lib.a Makefile
	${CC} ${CFLAGS} test00.o t_lib.a -o test00

test03.o: test03.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test03.c

test03: test03.o t_lib.a Makefile
	${CC} ${CFLAGS} test03.o t_lib.a -o test03

test06.o: test06.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test06.c

test06: test06.o t_lib.a Makefile
	${CC} ${CFLAGS} test06.o t_lib.a -o test06

test05.o: test05.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test05.c

test05: test05.o t_lib.a Makefile
	${CC} ${CFLAGS} test05.o t_lib.a -o test05

test08.o: test08.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c test08.c

test08: test08.o t_lib.a Makefile
	${CC} ${CFLAGS} test08.o t_lib.a -o test08

clean:
	rm -f t_lib.a ${EXECS} ${LIBOBJS} ${TSTOBJS} 
