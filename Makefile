# Derleyici ayarlari
CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS = -lm
TARGET  = load_balancer

# Windows icin .exe uzantisi
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
endif

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) results.csv

.PHONY: all run clean
