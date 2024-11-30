scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

newFooDir="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase2/result"
if [ ! -f "$runningDir/combine.txt" ];then
    cp $newFooDir/$testSet/combine.txt $runningDir/combine.txt
fi