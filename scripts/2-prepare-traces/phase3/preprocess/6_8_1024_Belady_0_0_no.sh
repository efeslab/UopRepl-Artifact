scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5
OPTFileBase="$HOME/uop/scarab_belady/traces/result"

if [ ! -f "$runningDir/new_opt_stream.out" ];then
    cp $OPTFileBase/$testSet.csv $runningDir/new_opt_stream.out
fi