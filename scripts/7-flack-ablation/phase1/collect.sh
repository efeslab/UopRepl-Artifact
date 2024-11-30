now=$(date +"%T")
echo "Current time : $now"
start_time=`date +%s`

dir=$(pwd)

# reuse previously generated traces
ln -s $UOP_SCRIPTS_ROOT/2-prepare-traces/phase2/PW $UOP_SCRIPTS_ROOT/7-flack-ablation/phase1/PW

# run VC FLACK
bash run.sh 8 64 &

wait

now=$(date +"%T")
echo "Finish time : $now"
end_time=`date +%s`
echo "TOTALLLL execution time was" `expr $end_time - $start_time` s.

