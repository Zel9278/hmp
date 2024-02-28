CC := gcc
CFLAGS := -std=c99 -Iincludes -D_GNU_SOURCE -D_DEFAULT_SOURCE
ifeq ($(DEBUG),YES)
	CFLAGS += -g
endif

LIBS := -lm -lpthread
OBJS := src/main.o src/midiplayer.o src/soundfont.o

ifeq ($(WIN32),YES)
	CC := i686-w64-mingw32-gcc
	SUFFIX := -i686.exe
endif

ifeq ($(WIN64),YES)
	CC := x86_64-w64-mingw32-gcc
	SUFFIX := -x86_64.exe
endif


.PHONY: clean all format

all: ./hmp$(SUFFIX)

./hmp$(SUFFIX): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

./src/%.o: ./src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

format:
	clang-format -i `find . -name "*.h" -or -name "*.c"`

clean:
	rm -f hmp hmp*.exe `find . -name "*.o"`