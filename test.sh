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

assert 0 '0'
assert 42 '42'
assert 42 '50 - 10 + 2'
assert 153 '1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17'
assert 42 '6*7'
assert 2 '6/3'
assert 42 '(3+4) * (12/2)'
assert 42 '(-3+-4) * (12/-2)'
echo "ok"
