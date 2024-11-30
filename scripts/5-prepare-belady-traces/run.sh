dir=$(pwd)

cd $dir/phase1
bash run.sh

cd $dir/phase2/result
bash run_single.sh 99000000

cd $dir/phase3
bash run.sh

cd $dir/phase3/analyze
bash run.sh