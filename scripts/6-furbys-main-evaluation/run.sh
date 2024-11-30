dir=$(pwd)

# Run simulation
cd $dir/allPolicy/result
bash run_single.sh 1000000

# get ppw number
cd $dir
bash $UOP_SCRIPTS_ROOT/3-power-model/call.sh $dir/allPolicy/result

cp $dir/allPolicy/result/power.csv $dir
cp -r $dir/allPolicy/analyze/plot $dir 