benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat' 'postgres' 'clang' 'wordpress' 'mysql'  'python' )
dir=$(pwd)

scriptDir="$UOP_SCRIPTS_ROOT/9-vary-offline-traces/components/result"

outputBase=$dir/FOO
sourceBase="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase2/result"

for testSet in "${benchmark_arr[@]}"; do
(
    outputDir=$outputBase/$testSet
    mkdir -p $outputDir
    sourceDir=$sourceBase/$testSet
    python3 $scriptDir/processRawRateFOO.py $sourceDir/combine.txt $outputDir/PMC_stream.in
    python3 $scriptDir/jenks_rate.py $outputDir/PMC_stream.in $outputDir/pmc_stream_w8.out 3 $outputDir
)&
done

wait