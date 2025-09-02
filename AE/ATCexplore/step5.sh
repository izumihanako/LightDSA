#!/bin/zsh
set -e

usage() {
  echo "Usage: $0 -m <memmove> -n <noop>"
  echo "  -m   Number of memmove submissions (m)"
  echo "  -n   Number of noop submissions (n)"
  echo "Use batch to submit \"m\" memmove and \"n\" noop operations. Each memmove is 1 page long (4KB)"
  echo "The batch descriptor list spans ceil((m+n)/64) pages"
  exit 1
}

# 解析参数
while getopts "m:n:" opt; do
  case "$opt" in
    m) M="$OPTARG" ;;
    n) N="$OPTARG" ;;
    *) usage ;;
  esac
done

# 基本检查
[ -z "${M:-}" ] && usage
[ -z "${N:-}" ] && usage
SPAN=$(( (M + N + 63) / 64 ))

echo "Use batch to submit ${M} memmove and ${N} noop operations. Each memmove is 1 page long (4KB)"
echo "The batch descriptor list spans ${SPAN} pages"
sudo python3 perfDSA.py "../../build/expr/paper/chapter3_1_ATC/batch_noop_memmove ${M} ${N}"

# ---- Expected output (printed for easy comparison) ----
echo "---------- Expected Output ----------"
echo "Translation requests  : $((2+N+3*M+SPAN-1))   (2+n+3m+p-1, where p is ceil((m+n)/64), p=$SPAN in this case)"  
echo "Translation hits      : NOT concerned"
