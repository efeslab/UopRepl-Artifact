# benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
benchmark_arr=('python')
traceBase="/mnt/storage/xianshengzhao/uop/0427_getTrace/phase3/UOP"
dir=$(pwd)

for testSet in "${benchmark_arr[@]}"; do
    resultBase=$dir/result/50/$testSet
    mkdir -p $resultBase
    (
        python3 processRawRateConcat.py $traceBase/0/$testSet.csv \
        $traceBase/1/$testSet.csv \
        $traceBase/2/$testSet.csv \
        $resultBase/PMC_stream.in
        python3 jenks_rate.py $resultBase/PMC_stream.in $resultBase/pmc_stream_w8.out 5 $resultBase
    ) &
done

wait