scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

# python3 $scriptDir/processRawRate.py $onlineFooDir/$testSet.csv $runningDir/PMC_stream.in
# python3 $scriptDir/jenks_rate.py $runningDir/PMC_stream.in $runningDir/pmc_stream_w8.out 5 $runningDir
# cp $bypassDir/$testSet/bypass.out $runningDir/bypass.out

bypassDir=/home/kanzhu/uop/0426_before_sensitive_cache/bypass/result/6_8_512_foo_0_0_no
cp $bypassDir/$testSet/bypass.out $runningDir/bypass.out
furbysTrace=/home/kanzhu/uop/0426_sensitive_cache/6_8_512_DynamicBypass_5_1_no
cp $furbysTrace/$testSet/pmc_stream_w8.out $runningDir/pmc_stream_w8.out