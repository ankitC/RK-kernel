C=$1
CM=$2
T=$3
TM=$4
PR=$5
/data/bin/set_reserve $(ps | grep '[i]nfinite_loop' | awk '{print $1}') $C $CM $T $TM $PR
