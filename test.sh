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
echo "ok"
