#!/bin/sh
/data/bin/all_cpu_online.sh
/data/bin/sysclock_governor.sh
echo 1 > sys/rtes/config/disable_cpus  
echo 1 > sys/rtes/config/guarantee
echo 1 > sys/rtes/config/migrate
echo $1 > sys/rtes/config/partition_policy
echo "Beginning tasks"
/data/bin/App_Reservation 3 5 3 5 0 0 0 0 &
/data/bin/App_Reservation 2 4 2 4 0 0 0 0 &
/data/bin/App_Reservation 2 5 2 5 0 0 0 0 &
/data/bin/App_Reservation 2 5 2 5 0 0 0 0 &
/data/bin/App_Reservation 1 5 1 5 0 0 0 0 &
echo "Finished launching tasks"