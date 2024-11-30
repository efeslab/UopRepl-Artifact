scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

pmcWeightPath="$UOP_SCRIPTS_ROOT/10-prepare-cross-traces/phase4/result"
cp $pmcWeightPath/{}/{}/pmc_stream_w8.out $runningDir/pmc_stream_w8.out