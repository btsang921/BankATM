CC = gcc
CFLAGS = -fno-stack-protector -Wall -g -Wextra -Iutil -Iatm -Ibank -Irouter -I.
LDFLAGS= -lcrypto -lssl


all: atm bank 


atm : atm-main.c atm.c net.c
	${CC} ${CFLAGS}  atm.c atm-main.c net.c -o atm ${LDFLAGS}
bank : bank-main.c bank.c net.c
	${CC} ${CFLAGS} bank.c bank-main.c net.c -o bank ${LDFLAGS}
clean:
	rm -f atm bank *.o
