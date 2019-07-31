DIR_INC = ./inc
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_BIN = ./bin

SRC = $(wildcard ${DIR_SRC}/*.c)
OBJ = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC}))

TARGET = main.out

BIN_TARGET = ${DIR_BIN}/${TARGET}
all:$(BIN_TARGET)

CC = arm-linux-gcc
CFLAGS = -g -lpthread -ljpeg -Wall -I${DIR_INC}

${BIN_TARGET}:${OBJ}
	$(CC) $(CFLAGS) $(OBJ) -o $@

${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	find ${DIR_OBJ} -name *.o -exec rm -rf {} +
