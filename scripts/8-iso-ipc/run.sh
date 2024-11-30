dir=$(pwd)

cd $dir/ipcIso
python3 genConfig.py

cd result
bash run_single.sh 90000000

cp -r $dir/phase2/analyze/plot $dir