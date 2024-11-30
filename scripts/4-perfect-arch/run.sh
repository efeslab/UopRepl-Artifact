dir=$(pwd)

# Run simulation
cd $dir/perfectArch/result
bash run_single.sh 40000000

# get ppw number
cd $dir
bash $UOP_SCRIPTS_ROOT/3-power-model/call.sh $dir/perfectArch/result

cp $dir/perfectArch/result/power.csv $dir