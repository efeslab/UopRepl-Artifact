dir=$(pwd)

cd $dir/components/fooWeight
bash run.sh

cd ../result
bash run_single.sh 90000000

cp -r $dir/components/analyze/plot $dir -r