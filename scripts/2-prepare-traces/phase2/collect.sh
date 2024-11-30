now=$(date +"%T")
echo "Current time : $now"
start_time=`date +%s`

dir=$(pwd)
cd $dir/traceCopy
bash run.sh

cd $dir
bash run.sh 8 64 &

wait

now=$(date +"%T")
echo "Finish time : $now"
end_time=`date +%s`
echo "TOTALLLL execution time was" `expr $end_time - $start_time` s.

