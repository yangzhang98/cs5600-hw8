#!/bin/bash
# Usage: ./run.sh <question number 1-10>
# Runs the experiments for one question of OSTEP Ch.32 homework.
# Any run that hangs for more than 2 seconds is counted as a deadlock.

make -s 2>/dev/null || make
TRIALS=10

# run a program TRIALS times, count how many finish vs. hang (deadlock)
count() {
    ok=0; hang=0
    for i in $(seq 1 $TRIALS); do
        if timeout 2 "$@" > /dev/null; then ok=$((ok+1)); else hang=$((hang+1)); fi
    done
    printf "%-45s finished: %2d   deadlocked: %2d   (out of %d)\n" "$*" $ok $hang $TRIALS
}

# run a timed program once and print one line
timed() {
    printf "%-55s " "$*"
    timeout 60 "$@" | tr '\n' ' '
    echo
}

echo "===== Question $1 ====="
case $1 in
1)
    for i in 1 2 3 4 5; do echo "--- run $i"; ./vector-deadlock -n 2 -l 1 -v; done ;;
2)
    for l in 1 10 100 1000 10000; do count ./vector-deadlock -n 2 -l $l -d; done ;;
3)
    for n in 1 2 3 4 8 16; do count ./vector-deadlock -n $n -l 10000 -d; done
    echo "--- same, but without -d / with -p:"
    count ./vector-deadlock -n 8 -l 10000
    count ./vector-deadlock -n 8 -l 10000 -d -p ;;
4)
    echo "(code question: read vector-global-order.c)"
    count ./vector-global-order -n 8 -l 100000 -d ;;
5)
    for l in 100000 200000 400000; do timed ./vector-global-order -t -n 2 -l $l -d; done
    for n in 2 4 8; do timed ./vector-global-order -t -n $n -l 100000 -d; done ;;
6)
    for n in 2 4 8; do
        timed ./vector-global-order -t -n $n -l 100000 -d
        timed ./vector-global-order -t -n $n -l 100000 -d -p
    done ;;
7)
    for n in 2 4 8; do
        timed ./vector-global-order -t -n $n -l 100000 -d
        timed ./vector-try-wait     -t -n $n -l 100000 -d
        timed ./vector-try-wait     -t -n $n -l 100000 -d -p
    done ;;
8)
    for n in 2 4 8; do
        timed ./vector-global-order        -t -n $n -l 100000 -d
        timed ./vector-avoid-hold-and-wait -t -n $n -l 100000 -d
        timed ./vector-global-order        -t -n $n -l 100000 -d -p
        timed ./vector-avoid-hold-and-wait -t -n $n -l 100000 -d -p
    done ;;
9)
    echo "(code question: read vector-nolock.c)"
    count ./vector-nolock -n 8 -l 100000 -d ;;
10)
    for p in "" "-p"; do
        for n in 2 4 8; do
            for prog in vector-global-order vector-try-wait vector-avoid-hold-and-wait vector-nolock; do
                timed ./$prog -t -n $n -l 100000 -d $p
            done
        done
    done ;;
*)
    echo "usage: ./run.sh <1-10>" ;;
esac
