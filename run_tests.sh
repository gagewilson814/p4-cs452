#!/usr/bin/env bash
set -uuo pipefail

SRC_MAIN="./app/main.c"
SRC_QUEUE="./src/lab.c"
OUT_BIN="./fifo_test"

CFLAGS="-std=c11 -O2 -Wall -Wextra -pthread"

PRODUCERS=(1 2 4 8)
CONSUMERS=(1 2 4 8)
QUEUE_SIZES=(1 2 4 8)
ITEMS="1000"
DELAYS=("" "-d")

echo "Compiling ${SRC_MAIN} + ${SRC_QUEUE} → ${OUT_BIN}"
if ! gcc $CFLAGS "$SRC_MAIN" "$SRC_QUEUE" -o "$OUT_BIN"; then
  echo "✖ Compilation failed! See errors above."
  exit 1
fi
echo "✔ Compilation succeeded."

total=0
failures=0

for p in "${PRODUCERS[@]}"; do
  for c in "${CONSUMERS[@]}"; do
    for q in "${QUEUE_SIZES[@]}"; do
      for d in "${DELAYS[@]}"; do
        ((total++))
        args="-p $p -c $c -i $ITEMS -s $q $d"
        printf "\n>>> Test #%d: ./${OUT_BIN} %s\n" "$total" "$args"

        if OUTPUT=$(./"$OUT_BIN" $args 2>&1); then
          if grep -q '^ERROR:' <<<"$OUTPUT"; then
            echo "✖ FAIL: detected “ERROR:” in output"
            echo "$OUTPUT"
            ((failures++))
          else
            echo "✔ PASS"
          fi
        else
          echo "✖ FAIL: non-zero exit"
          echo "$OUTPUT"
          ((failures++))
        fi
      done
    done
  done
done

echo
echo "======================================"
echo " Total tests run : $total"
echo "      Failures   : $failures"
echo "======================================"
exit $failures
