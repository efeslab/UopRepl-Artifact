### Phase 1 collect traces

dir=$(pwd)
instLimit=99000000
instMinLimit=90000000

cd $dir/phase1
python3 genConfig.py --inst_count $instLimit --trace_path $UOP_DATACENTER_TRACES/isca-2022-traces

cd result
bash run_single.sh 

cd $dir/phase2/
bash collect.sh

cd $dir/phase3
python3 genConfig.py --inst_count $instMinLimit --trace_path $UOP_DATACENTER_TRACES/isca-2022-traces --working_dir $UOP_SCRIPTS_ROOT/10-prepare-cross-traces
cd result
bash run_single.sh

cd $dir/phase4
bash launch.sh