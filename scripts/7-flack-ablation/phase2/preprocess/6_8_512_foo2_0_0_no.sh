scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

unfixedFoo="$UOP_SCRIPTS_ROOT/7-flack-ablation/phase1/result"
if [ ! -f "$runningDir/combine.txt" ];then
    cp $unfixedFoo/$testSet/combine.txt $runningDir/combine.txt
fi