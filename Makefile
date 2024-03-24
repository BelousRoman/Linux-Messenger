BIN := msgrux
HEADERS := hdr/*.h libs/*.h
SOURCES := src/*.c libs/*.c
CC := gcc #-Wall
CFLAGS := -g
OFLAGS := -c
LIBS := -lcurses
# $(pkg-config ncursesw --libs --cflags)

all: messenger.c $(SOURCES)
	$(CC) -o $(BIN) messenger.c $(SOURCES) $(CFLAGS) $(LIBS) 
all: $(HEADERS)

clean:
	rm -rf $(BIN) *.o

rebuild: clean all