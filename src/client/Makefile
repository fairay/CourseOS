TARGET	= client 
OBJS 	= gpioclient.o
CFLAGS	= -Wall -O2 -I ../
LIBS	= -L/usr/local/lib -I/usr/local/include -lreadline

.PHONY: clean

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LIBS} 

clean:
	${RM} *.o core* ${TARGET}
