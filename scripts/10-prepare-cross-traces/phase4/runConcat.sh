benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
traceBase="$UOP_SCRIPTS_ROOT/10-prepare-cross-traces/phase3/UOP"
dir=$(pwd)

for testSet in "${benchmark_arr[@]}"; do
    resultBase=$dir/result/50/$testSet
    mkdir -p $resultBase
    (
        python3 processRawRateConcat.py $traceBase/0/$testSet.csv \
        $traceBase/1/$testSet.csv \
        $traceBase/2/$testSet.csv \
        $resultBase/PMC_stream.in
        python3 jenks_rate.py $resultBase/PMC_stream.in $resultBase/pmc_stream_w8.out 3 $resultBase
    ) &
done

wait