scriptDir=$1
runningDir=$2
OMP_NUM_THREADS=4 python3 $scriptDir/jenks.py $runningDir/pmc_stream.out $runningDir/pmc_stream_w8.out 5 $runningDir
if [ -s $runningDir/pmc_stream_switch.out ] ; then
OMP_NUM_THREADS=4 python3 $scriptDir/jenks.py $runningDir/pmc_stream_switch.out $runningDir/pmc_stream_switch_w8.out 5 $runningDir
fi