scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

pmcWeightPath="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase4/result"
cp $pmcWeightPath/$testSet/pmc_stream_w8.out $runningDir/pmc_stream_w8.out