scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

bypassDir="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase4/analyze/result"
cp $bypassDir/$testSet/bypass.out $runningDir/bypass.out

pmcWeightPath="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase4/result"
cp $pmcWeightPath/$testSet/pmc_stream_w8.out $runningDir/pmc_stream_w8.out