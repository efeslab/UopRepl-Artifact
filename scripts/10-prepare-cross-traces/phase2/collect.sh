now=$(date +"%T")
echo "Current time : $now"
start_time=`date +%s`

dir=$(pwd)
cd $dir/traceCopy
bash run.sh

# make foo
cd $dir/FOO
make
cd $dir

bash run.sh 8 64 0 &
bash run.sh 8 64 1 &
bash run.sh 8 64 2 &
bash run.sh 8 64 3 &
bash run.sh 8 64 4 &
bash run.sh 8 64 5 &
bash run.sh 8 64 6 &
bash run.sh 8 64 7 &
bash run.sh 8 64 8 &
bash run.sh 8 64 9 &

wait

now=$(date +"%T")
echo "Finish time : $now"
end_time=`date +%s`
echo "TOTALLLL execution time was" `expr $end_time - $start_time` s.

