############################################################# 
# Makefile for shared library.
#############################################################
#set your own environment option

CC = /opt/x-tools/arm-L1S902-linux-gnueabi/bin/arm-L1S902-linux-gnueabi-gcc
CC_FLAG = -fPIC -D_NOMNG -D_FILELINE

#set your inc and lib
INC = 
LIB = -lpthread

#make target lib and relevant obj 
LIB_OBJ = libh18.so
BIN_OBJ = h18_test
OBJ = hal_log.o hal_uart.o hal_gps.o

#all target
all:$(LIB_OBJ) $(BIN_OBJ)

$(LIB_OBJ):$(OBJ)
	$(CC) -shared -o $@ $(OBJ) $(LIB)

$(BIN_OBJ):$(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)

.SUFFIXES: .c .o .cpp
.c.o:
	$(CC) $(CC_FLAG) $(INC) -c $*.c -o $*.o

.PRONY:clean
clean:
	@echo "Removing linked and compiled files......;"
	rm -f $(OBJ) $(PRG)
