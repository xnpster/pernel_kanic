#!/bin/bash
jobs=()
trap 'kill $jobs' EXIT HUP TERM INT
/home/xnpst/ispras-toolchain/ispras-llvm/bin/clang -o udp_test udp_test.c
NETWORK_TEST=1 make -C .. qemu 2>/dev/null 1>out.txt & jobs+=($!)
sleep 20
python3 tests.py
if cat out.txt | grep -q "HELLO"; then echo "JOS UDP output. Result: OK"; else echo "JOS UDP output. Result: FAIL"; fi
rm out.txt udp_test

