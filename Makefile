CFLAGS=-std=c11 -g -fno-common -Wall -Wno-switch
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

bin/lucc: $(OBJS)
	mkdir -p $(@D)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
$(OBJS): src/lucc.h

test: test-x64 test-riscv

test-x64: bin/lucc
	tests/test.sh --x64 $<

test-riscv: bin/lucc
	tests/test.sh --riscv $<
clean:
	git clean -fdX

.PHONY: clean test
