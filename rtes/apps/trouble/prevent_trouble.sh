PID=0
C=2
T=4
Z=0
echo Preventing Trouble
echo Starting busy loop
./busy_loop &
pidof busy_loop > $PID
./set_reserve $PID $C $T

