#!/bin/bash
opt_riscv=false
if [[ "$1" == "--riscv"  ]]; then
    opt_riscv=true
    shift
fi
BIN=./$@

function assert-x64 {
    want=$1
    input=$2

    $BIN "$input" > tmp.s || exit 1
    CC=cc
    $CC -c -o tests/extern.o tests/extern.c
    $CC -static -o tmp tmp.s tests/extern.o
    ./tmp
    got=$?

    if [[ "$got" != "$want" ]]; then
        echo "$input => want $want, got $got"
        exit 1
    else
        echo "$input => $got"
    fi
}

function assert-riscv {
    want=$1
    input=$2
    $BIN -march=riscv "$input" > tmp.s || exit 1
    CC=riscv64-linux-gnu-gcc-8
    $CC -c -o tests/extern-riscv.o tests/extern.c
    $CC -static -o tmp tmp.s tests/extern-riscv.o
    qemu-riscv64 ./tmp
    got=$?

    if [[ "$got" != "$want" ]]; then
        echo "$input => want $want, got $got"
        exit 1
    else
        echo "$input => $got"
    fi
}

function assert {
    want=$1
    input=$2
    if $opt_riscv; then
        assert-riscv "$want" "$input"
    else
        assert-x64 "$want" "$input"
    fi
}

assert 0 'return 0;'
assert 42 'return 42;'
assert 42 'return 50 - 10 + 2;'
assert 153 'return 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17;'
assert 42 'return 6*7;'
assert 2 'return 6/3;'
assert 42 'return (3+4) * (12/2);'
assert 42 'return (-3+-4) * (12/-2);'

assert 1 'return 1==1;'
assert 0 'return 1!=1;'
assert 0 'return 1<1;'
assert 1 'return 1<=1;'
assert 0 'return 1>1;'
assert 1 'return 1>=1;'

assert 0 'return 0==1;'
assert 1 'return 0!=1;'
assert 1 'return 0<1;'
assert 1 'return 0<=1;'
assert 0 'return 0>1;'
assert 0 'return 0>=1;'

assert 3 '1; 2; return 3;'
assert 3 '1; return 3; 2;'
assert 10 '1;2;3;4;5;6;7;8;9;return 10;11;12;13;14;'
assert 4 'a=4; return a;'
assert 4 'abc=4; return abc;'
assert 2 'K=5; t_t = 2; abc=4; a123=123;return t_t;'
assert 44 'a = b = 44; return a;'
assert 42 'if (1) return 42; return 0;'
assert 0 'if (0) return 42; return 0;'
assert 42 'if (1) return 42; else return 0;'
assert 0 'if (0) return 42; else return 0;'

assert 5 'a=0; for (;a<5;a=a+1) 0; return a;'
assert 5 'a=0; while (a<5) a=a+1; return a;'

assert 15 'a=0; b=0; while(a<5) {a=a+1; b=b+a;} return b;'

assert 3 'return ret3();'
assert 5 'return ret5();'

assert 3 'return identity(3);'
assert 3 'return add2(1, 2);'
echo "ok"
