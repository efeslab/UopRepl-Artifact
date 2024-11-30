scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

if [ ! -f "$runningDir/combine.txt" ];then
    cp $fooDir/$testSet/combine.txt $runningDir/combine.txt
fi