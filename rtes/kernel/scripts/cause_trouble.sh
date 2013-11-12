C=10
CM=0
T=10
TM=0
PR=120
echo $C
echo $CM
echo $T
echo $TM
echo $PR
/data/bin/infinite_loop &
PID=$(ps | grep '[i]nfinite_loop' | awk '{print $1}')
/data/bin/set_reserve $PID $C $CM $T $TM
