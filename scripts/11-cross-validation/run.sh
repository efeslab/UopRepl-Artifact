dir=$(pwd)
instMinLimit=90000000

cd $dir/phase1
python3 genConfig.py --inst_count $instMinLimit --trace_path $UOP_DATACENTER_TRACES/isca-2022-traces
cd result
bash run_single.sh

cd $dir/phase2
python3 genConfig.py --inst_count $instMinLimit --trace_path $UOP_DATACENTER_TRACES/isca-2022-traces
cd result
bash run_single.sh