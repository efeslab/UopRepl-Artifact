scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5
bypassDir="$HOME/uop/0320_generate_FOO/analyze/result"

cp $bypassDir/$testSet/bypass.out $runningDir/bypass.out
pmcWeightPath="$HOME/uop/0425_sensitive_bits/result/6_8_1024_DynamicBypass_3_1_no"
cp $pmcWeightPath/$testSet/pmc_stream_w8.out $runningDir/pmc_stream_w8.out