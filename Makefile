MSG_BIN := msgrux
SRV_BIN := srvmain
HEADERS := hdr/*.h libs/*.h
SOURCES := src/*.c libs/*.c
CC := gcc #-Wall
CFLAGS := -g
OFLAGS := -c
LIBS := -lcurses
# $(pkg-config ncursesw --libs --cflags)

all: app srv

app: messenger.c $(SOURCES)
	$(CC) -o $(MSG_BIN) messenger.c $(SOURCES) $(CFLAGS) $(LIBS)
app: $(HEADERS)

srv: main_server.c $(SOURCES)
	$(CC) -o $(SRV_BIN) main_server.c $(SOURCES)  $(LIBS) $(CFLAGS)
srv: $(HEADERS)

clean:
	rm -rf $(MSG_BIN) $(SRV_BIN) *.o

rebuild: clean all