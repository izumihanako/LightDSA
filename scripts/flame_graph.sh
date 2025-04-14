
if [ "${1}" = "clear" ] || [ "${1}" = "clean" ]; then
    rm -f perf*.svg
    exit
fi

FlameFolder="/root/FlameGraph"

perf record --call-graph dwarf --kernel-callchains ${1}
perf script -i perf.data > perf.unfold
${FlameFolder}/stackcollapse-perf.pl perf.unfold > perf.folded

cnt=$(expr $(ls -l | grep "perf.*\.svg" | wc -l)) 
${FlameFolder}/flamegraph.pl perf.folded > perf${cnt}.svg

rm -f perf.data.old perf.data perf.unfold perf.folded  