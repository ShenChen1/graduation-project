TARGET=sim900a

CFLAGS=-g
all:$(TARGET)

sim900a:serial.c SIM900A.c
	gcc $(CFLAGS) serial.c SIM900A.c -o sim900a

clean:
	rm $(TARGET) -f
