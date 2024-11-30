scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

hotBound=0.8
coldBound=0.6

hitRatePath="$UOP_SCRIPTS_ROOT/5-prepare-belady-traces/phase3/result"
python3 $scriptDir/thermoGenerator.py $hitRatePath/$testSet/PMC_stream.in $runningDir/thermo.csv $hotBound $coldBound