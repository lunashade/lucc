CFLAGS=-std=c11 -g -fno-common -Wall -Wno-switch
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

bin/lucc: $(OBJS)
	mkdir -p $(@D)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
$(OBJS): src/lucc.h

test: bin/lucc
	./test.sh $<

test-riscv: bin/lucc
	./test.sh --riscv $<
clean:
	git clean -fdX

.PHONY: clean test
