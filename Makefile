CC		:= gcc
CFLAGS	:= -Wall -Wextra -g -Isrc

BIN		:= bin
SRC		:= src

LIBRARIES	:= -lpcap

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= sirin_sniffer.exe
else
EXECUTABLE	:= sirin_sniffer
endif

SOURCEDIRS	:= $(shell find $(SRC) -type d)


SOURCES		:= $(wildcard $(patsubst %,%/*.c, $(SOURCEDIRS)))
OBJECTS		:= $(SOURCES:.c=.o)


all: $(EXECUTABLE)


.PHONY: clean
clean:
	-$(RM) $(EXECUTABLE)
	-$(RM) $(OBJECTS)


run: all
	sudo ./$(BIN)/$(EXECUTABLE)


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBRARIES)