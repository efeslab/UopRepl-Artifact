scriptDir=$1
runningDir=$2
fooDir=$3
onlineFooDir=$4
testSet=$5

hotBound=0.8
coldBound=0.6

hitRatePath="$HOME/uop/0420_out_tests/result/6_8_1024_BeladyVersion_5_1_no"
# hitRatePath="/mnt/storage/xianshengzhao/uop/0420_out_tests/result/6_8_1024_DynamicBypass_5_1_no"
python3 $scriptDir/thermoGenerator.py $hitRatePath/$testSet/PMC_stream.in $runningDir/thermo.csv $hotBound $coldBound