#!/usr/bin/env bash
# test_cli.sh --- end-to-end tests of bin/aad_modify (exit codes and the -m CSV line)
# run from the repository root: ./tests/test_cli.sh
set -u
bin=./bin/aad_modify
tmp=$(mktemp -d)
fail=0
check() { # check <description> <expected exit> <command...>
    local desc=$1 want=$2; shift 2
    "$@" >"$tmp/out" 2>"$tmp/err"; local got=$?
    if [ "$got" -ne "$want" ]; then echo "FAIL $desc: exit $got, expected $want"; cat "$tmp/err"; fail=1; fi
}

check "help" 0 $bin -h
check "no arguments" 2 $bin
check "unknown engine" 2 $bin -e foo inputs/aad_a1.txt "$tmp/x.txt"
check "unknown input extension" 2 $bin README.md "$tmp/x.txt"
check "missing input" 1 $bin "$tmp/none.pdf" "$tmp/x.pdf"
check "bad -j" 2 $bin -e omp -j 2x inputs/aad_a1.txt "$tmp/x.txt"
check "bad -t" 2 $bin -t 1abc inputs/aad_a1.txt "$tmp/x.txt"
printf '%%PDF-1.4\nstartxref\n9\n%%%%EOF\n%060d\n' 0 > "$tmp/too_much.pdf"
check "|S| > 55 in PDF" 1 $bin "$tmp/too_much.pdf" "$tmp/x.pdf"
check "no solution in 0.2 s" 3 $bin -t 0.2 inputs/aad_a1.txt "$tmp/x.txt"

$bin -m -t 1 inputs/aad_a1.txt "$tmp/m.txt" >"$tmp/csv" 2>/dev/null
grep -Eq '^scalar,1,txt,1,[0-9.]+,[0-9]+,[0-9]+,[0-9]+$' "$tmp/csv" || { echo "FAIL -m CSV: $(cat "$tmp/csv")"; fail=1; }

rm -rf "$tmp"
[ $fail -eq 0 ] && echo "cli tests passed"
exit $fail
