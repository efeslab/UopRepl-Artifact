### Phase 1 collect traces

# 8_512 LRU insert traces -> FOO traces -> FLACK traces -> Furbys Input
dir=$(pwd)

# cd $dir/phase1/result
# bash run_single.sh 99000000

# cd $dir/phase2/
# bash collect.sh

cd $dir/phase3/result
bash run_single.sh 90000000

cd $dir/phase4/
bash launch.sh