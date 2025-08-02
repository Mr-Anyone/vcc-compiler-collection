CC := /opt/llvm/bin/clang
CXX := /opt/llvm/bin/clang++
CFLAGS := -std=c17 -g
VCC := vcc
LDFLAGS =  -lSDL3

VCC_SRCS := $(wildcard *.vcc)
C_SRCS   := $(wildcard *.c)
VCC_OBJS := $(VCC_SRCS:.vcc=.vcco)
C_OBJS   := $(C_SRCS:.c=.o)
OBJS := $(VCC_OBJS) $(C_OBJS)

all: main

main: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.vcco : %.vcc
	$(VCC) $< -o $@ -O3

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
		
.PHONY: clean
clean:
	rm -f *.o *.vcco main
