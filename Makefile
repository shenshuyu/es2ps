SUB_TARGET=libmpeg_2ps.a

CROSS=

AR=$(CROSS)ar
CPP=$(CROSS)g++

SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)

$(SUB_TARGET):$(OBJ)
	$(AR) rc -o $(SUB_TARGET) $(OBJ)

$(OBJ): $(SRC)

%.o:%.c
	$(CPP) $(CFLAGS) -c $<


clean:
	@rm *.o *.a
	@rm $(SUB_TARGET)

