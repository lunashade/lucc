#!/bin/bash
opt_riscv=false
if [[ "$1" == "--riscv"  ]]; then
    opt_riscv=true
    shift
fi
if [[ "$1" == "--x64"  ]]; then
    opt_riscv=false
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

assert 0 'int main(){return 0;}'
assert 42 'int main(){return 42;}'
assert 42 'int main(){return 50 - 10 + 2;}'
assert 153 'int main(){return 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17;}'
assert 42 'int main(){return 6*7;}'
assert 2 'int main(){return 6/3;}'
assert 42 'int main(){return (3+4) * (12/2);}'
assert 42 'int main(){return (-3+-4) * (12/-2);}'

assert 1 'int main(){return 1==1;}'
assert 0 'int main(){return 1!=1;}'
assert 0 'int main(){return 1<1;}'
assert 1 'int main(){return 1<=1;}'
assert 0 'int main(){return 1>1;}'
assert 1 'int main(){return 1>=1;}'

assert 0 'int main(){return 0==1;}'
assert 1 'int main(){return 0!=1;}'
assert 1 'int main(){return 0<1;}'
assert 1 'int main(){return 0<=1;}'
assert 0 'int main(){return 0>1;}'
assert 0 'int main(){return 0>=1;}'

assert 3 'int main(){1; 2; return 3;}'
assert 3 'int main(){1; return 3; 2;}'
assert 10 'int main(){1;2;3;4;5;6;7;8;9;return 10;11;12;13;14;}'
assert 4 'int main(){int a=4; return a;}'
assert 4 'int main(){int abc=4; return abc;}'
assert 2 'int main(){int K=5; int t_t = 2; int abc=4; int a123=123;return t_t;}'
assert 44 'int main(){int a,b; a = b = 44; return a;}'
assert 42 'int main(){if (1) return 42; return 0;}'
assert 0 'int main(){if (0) return 42; return 0;}'
assert 42 'int main(){if (1) return 42; else return 0;}'
assert 0 'int main(){if (0) return 42; else return 0;}'

assert 5 'int main(){int a=0; for (;a<5;a=a+1) 0; return a;}'
assert 5 'int main(){int a=0; while (a<5) a=a+1; return a;}'

assert 15 'int main(){int a=0; int b=0; while(a<5) {a=a+1; b=b+a;} return b;}'

assert 3 'int main(){return ret3();}'
assert 5 'int main(){return ret5();}'

assert 3 'int main(){return identity(3);}'
assert 3 'int main(){return add2(1, 2);}'

assert 42 'int main() {return ret42();} int ret42() {return 42;}'

assert 3 'int main() { int x=3; return *&x;}'
assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z;}'
assert 5 'int main() { int x=3; int y=5; return *(&x+1);}'
assert 5 'int main() { int x=3; int *y=&x; *y=5; return x;}'
assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y;}'
assert 7 'int main() { int x=3; int y=5; *(&y-1)=7; return x;}'

assert 8 'int main() { return sizeof 12; }'
assert 8 'int main() { int x=3; int *y = &x; return sizeof(&y); }'

assert 1 'int main() { int x=3; int *y=&x; int *z=y+1; return z-y; }'
assert 8 'int main() {return isalnum(97);}'

assert 6 'int main() {return add3(1,2,3);} int add3(int a, int b, int c) {return a+b+c;}'
assert 55 'int main() {return fibo(9);} int fibo(int n) {if (n<=1) return 1; return fibo(n-2) + fibo(n-1);}'

assert 3 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x);}'
assert 4 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1);}'
assert 5 'int main() {int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2);}'

assert 3 'int main() {int x[2][3]; **x=3; *(*(x+2)+1) = 5; return **x;}'
assert 5 'int main() {int x[2][3]; **x=3; *(*(x+2)+1) = 5; return *(*(x+2)+1);}'

assert 3 'int main() {int x[2]; x[0] = 3; x[1]=4; x[2]=5; return x[0];}'
assert 4 'int main() {int x[2]; x[0] = 3; x[1]=4; x[2]=5; return x[1];}'
assert 5 'int main() {int x[2]; x[0] = 3; x[1]=4; x[2]=5; return x[2];}'
assert 5 'int main() {int x[2]; x[0] = 3; x[1]=4; 2[x]=5; return *(x+2);}'

assert 0 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[0][0];}'
assert 1 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[0][1];}'
assert 2 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[0][2];}'
assert 3 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[1][0];}'
assert 4 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[1][1];}'
assert 5 'int main() {int x[2][3]; x[0][0]=0;x[0][1]=1;x[0][2]=2;x[1][0]=3;x[1][1]=4;x[1][2]=5; return x[1][2];}'

echo "ok"
