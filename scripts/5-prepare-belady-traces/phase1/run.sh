benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat' 'postgres' 'clang' 'wordpress' 'mysql'  'python' )
traceBase="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase1/PW"
outputBase="$UOP_SCRIPTS_ROOT/5-prepare-belady-traces/phase1/result"
mkdir -p $outputBase

for testSet in "${benchmark_arr[@]}"; do
    outputFile=$outputBase/$testSet.csv
    rm $outputFile
    touch $outputFile
    python3 copyTrace.py $traceBase/$testSet.csv &> $outputFile &
    
    echo $testSet
done

wait