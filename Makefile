# Not a very complex project, simple makefile is fine
CC		:= gcc

TARGET	:= hrpt2apt

SRCDIR	:= .
INCDIR	:= .
LIB		:=
CFILES	:= main.c resample.c args.c

# Default target
all: release

novolk: LIB :=
novolk: release

release: CFLAGS	:= -O3 -fpermissive
release: $(TARGET)

debug: CFLAGS	:= -Wall -Wextra -Wpedantic -O0 -g3
debug: $(TARGET)

$(TARGET): 
	$(CC) -o $(TARGET) $(CFILES) -I$(INCDIR) $(LIB)

clean:
	rm $(TARGET)

.PHONY: all clean $(TARGET) # completely void the use of a Makefile lol