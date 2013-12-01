ONE=1
PARTITION=L
TIME=1
/data/bin/all_cpu_online.sh
/data/bin/sysclock_governor.sh
echo $ONE > /sys/rtes/config/disable_cpus
echo $ONE > /sys/rtes/config/guarantee
echo $ONE > /sys/rtes/config/migrate
echo $ONE > /sys/rtes/config/energy
echo $PARTITION > /sys/rtes/config/partition_policy
echo "Beginning task 1"
/data/bin/App_Reservation 3 5 3 5 0 0 0 0 > /dev/null &
sleep $TIME
echo "Beginning task 2"
/data/bin/App_Reservation 2 4 2 4 0 0 0 0 > /dev/null &
sleep $TIME
echo "Beginning task 3"
/data/bin/App_Reservation 2 5 2 5 0 0 0 0 > /dev/null &
sleep $TIME
echo "Beginning task 4"
/data/bin/App_Reservation 2 5 2 5 0 0 0 0 > /dev/null &
sleep $TIME
echo "Beginning task 5"
/data/bin/App_Reservation 1 5 1 5 0 0 0 0 > /dev/null &
echo "Finished launching tasks"

