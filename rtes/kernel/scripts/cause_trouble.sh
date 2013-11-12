C=10
CM=0
T=10
TM=0
PR=120
/data/bin/inifinite_loop &
/data/bin/set_reserve $(ps | grep 'infinite_loop' | awk '{print $1}') $C $CM $T $TM $PR
