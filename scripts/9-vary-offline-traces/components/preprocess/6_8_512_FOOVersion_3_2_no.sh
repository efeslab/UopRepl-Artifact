scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

pmcWeightPath="$UOP_SCRIPTS_ROOT/9-vary-offline-traces/components/fooWeight/FOO"
cp $pmcWeightPath/$testSet/pmc_stream_w8.out $runningDir/pmc_stream_w8.out