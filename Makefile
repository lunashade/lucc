CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

$(OBJS): src/lucc.h

src/lucc.h:

bin/lucc: $(OBJS)
	mkdir -p $(@D)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

test: bin/lucc
	./test.sh $<

clean:
	git clean -fdX

.PHONY: clean test
