traceIndex=$1
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
traceBase="$UOP_SCRIPTS_ROOT/10-prepare-cross-traces/phase3/UOP/$traceIndex"
dir=$(pwd)

for testSet in "${benchmark_arr[@]}"; do
    resultBase=$dir/result/$traceIndex/$testSet
    if [ -e "$traceBase/$testSet.csv" ]; then
        mkdir -p $resultBase
        (
            python3 processRawRate.py $traceBase/$testSet.csv $resultBase/PMC_stream.in
            python3 jenks_rate.py $resultBase/PMC_stream.in $resultBase/pmc_stream_w8.out 3 $resultBase
        ) &
    fi
done

wait