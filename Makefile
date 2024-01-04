CC = $(shell xcrun --sdk iphoneos --find clang)
LDID = $(shell which ldid)

MINOS = 12.0
SDK = $(shell xcrun --sdk iphoneos --show-sdk-path)
LIBS = $(SDK)/usr/lib

CFLAGS := -dynamiclib -I./src -I./include -isysroot $(SDK) 
CFLAGS += -miphoneos-version-min=$(MINOS) -Wall 
LDFLAGS := -L$(LIBS) -framework Foundation -framework IOKit -lc++ -lc

SRC = $(wildcard */*.m) $(wildcard */*.c) src/util.s

all: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o chimera4k.dylib $^
	$(LDID) -S chimera4k.dylib

clean:
	@rm -rf chimera4k.dylib