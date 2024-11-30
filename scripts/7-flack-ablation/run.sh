dir=$(pwd)

cd $dir/phase1
bash run.sh

cd $dir/phase2/result
bash run_single.sh 90000000

cp -r $dir/phase2/analyze/plot $dir