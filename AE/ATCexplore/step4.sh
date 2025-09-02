#!/bin/zsh
set -e

usage() {
  echo "Usage: $0 -k <times> -p <pages>"
  echo "  -k   Number of memmove submissions (k)"
  echo "  -p   Size of each memmove, in pages of 4KB (p)"
  echo "Use 1 descriptor to perform memmoves with fixed src and incrementally increasing dest"
  echo "Perform memmove \"k\" times; each memmove is \"p\" 4K pages long."
  exit 1
}

# 解析参数
while getopts "k:p:" opt; do
  case "$opt" in
    k) K="$OPTARG" ;;
    p) P="$OPTARG" ;;
    *) usage ;;
  esac
done

# 基本检查
[ -z "${K:-}" ] && usage
[ -z "${P:-}" ] && usage

# 计算 b = 4096 * p（字节）
B=$(( P * 4096 ))

echo "Use 1 descriptor to perform memmoves with fixed src and incrementally increasing dest"
echo "Perform memmove ${K} times; each memmove is ${P} 4K pages (i.e., ${B} bytes) long."
sudo python3 perfDSA.py "../../build/expr/paper/chapter3_1_ATC/diff_desc_s_src_d_dst 0 ${B} ${B} ${K} 1"

# ---- Expected output (printed for easy comparison) ----
echo "---------- Expected Output ----------"
if [ "$P" -eq 1 ]; then
  echo "Translation requests  : $((3*K))   (3k)"
  echo "Translation hits      : $((2*K-2))   (2k-2)"
else
  echo "Translation requests  : $((K+2*K*P))   (k+2kp)"
  echo "Translation hits      : $((K-1))   (k-1)"
fi
