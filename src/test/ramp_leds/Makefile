TARGET	= rampleds
OBJS 	= ${TARGET}.o
CFLAGS	= -Wall -O2 -I /home/techplex/ece331/km/gpio
LIBS	=

.PHONY: clean

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LIBS}

clean:
	${RM} *.o core* ${TARGET}