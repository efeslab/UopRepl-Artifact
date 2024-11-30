csvBase="/home/kanzhu/uop/1120_profile_traces/phase3/UOP"
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
outputBase="/home/kanzhu/uop/1120_profile_traces/phase4/analyze/result"


for testSet in "${benchmark_arr[@]}"; do
    mkdir -p $outputBase/$testSet
    outputFile=$outputBase/$testSet/bypass.out
    rm $outputFile
    touch $outputFile
    python3 uopGr.py $csvBase/$testSet.csv $testSet 0.95 &>> $outputFile &
done

wait
