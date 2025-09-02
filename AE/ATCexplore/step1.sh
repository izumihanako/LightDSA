#!/bin/zsh
set -e

usage() {
  echo "Usage: $0 -k <k_times> -d <num_desc>"
  echo "  -k   Number of no-op submissions (k)"
  echo "  -d   Number of different descriptors (d)"
  echo "Use \"d\" descriptor with completion record located on different pages to perform no-op \"k\" times"
  exit 1
}

# 参数解析
while getopts "k:d:" opt; do
  case $opt in
    k) K=$OPTARG ;;
    d) D=$OPTARG ;;
    *) usage ;;
  esac
done

# 检查参数是否提供
if [ -z "$K" ] || [ -z "$D" ]; then
  usage
fi

echo "Use ${D} descriptor with completion record located on different pages to perform no-op ${K} times" 
sudo python3 perfDSA.py "../../build/expr/paper/chapter3_1_ATC/diff_desc_noop $K $D"

# ---- Expected output (printed for easy comparison) ----
echo "---------- Expected Output ----------"
echo "Translation requests  : ${K}   (k)"
if [ "$D" -eq 1 ]; then
  echo "Translation hits      : $((K - 1))   (k-1)"
else
  echo "Translation hits      : 0"
fi