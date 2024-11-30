binaryRoot="./FOO/foo";
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
traceBaseLine="./PW"
traceLength=-1

waySize=$1
setSize=$2
traceIndex=$3


now=$(date +"%T")
echo "Current time : $now"
start_time=`date +%s`

dir=$(pwd)
# make foo
cd $dir/FOO
make

cd $dir

for testSet in "${benchmark_arr[@]}"; do
    curResultPath=$dir/result/$testSet
    tracePath=$traceBaseLine/$testSet.csv
    if [ -e "$tracePath" ]; then
        outPathBase=$curResultPath/setOut
        mkdir -p $outPathBase

        rm $curResultPath/combine.txt
        touch $curResultPath/combine.txt

        for i in `eval echo {0..$setSize}` # modify here! 
        do
            $binaryRoot $tracePath $waySize 4 $outPathBase/$i.txt $i $traceLength $setSize &
            activeCount=$(ps aux | grep 'foo' | awk '{print $2}' | wc -l)
            while ((activeCount>=500)); do
                sleep 35
                activeCount=$(ps aux | grep 'foo' | awk '{print $2}' | wc -l)
            done
        done
    fi
done

wait

for testSet in "${benchmark_arr[@]}"; do
    resultBase=$dir/result
    combineFile=$resultBase/$testSet/combine.txt
    if [ -e "$combineFile" ]; then
        rm $combineFile
        touch $combineFile
        setBase=$resultBase/$testSet/setOut
        for i in `eval echo {0..$setSize}` # modify here! 
        do
            cat $setBase/$i.txt >> $combineFile
        done
        python3 sortTrace.py $combineFile
    fi
done

now=$(date +"%T")
echo "${waySize}_${setSize} Finish time : $now"
end_time=`date +%s`
echo "execution time was" `expr $end_time - $start_time` s.








