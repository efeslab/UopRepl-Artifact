scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5
OPTFileBase="/home/kanzhu/uop/1121_belady_traces/phase1/result"

if [ ! -f "$runningDir/new_opt_stream.out" ];then
    cp $OPTFileBase/$testSet.csv $runningDir/new_opt_stream.out
fi