#!/bin/bash
BIN=./$1

function assert {
    want=$1
    input=$2
    $BIN "$input" > tmp.s || exit 1
    cc -o tmp tmp.s
    ./tmp
    got=$?

    if [[ "$got" != "$want" ]]; then
        echo "$input => want $want, got $got"
        exit 1
    else
        echo "$input => $got"
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
echo "ok"
