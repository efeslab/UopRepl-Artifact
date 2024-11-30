rm -rf build
mkdir build
cd build
cmake ..
make
cd ..

benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
traceBaseLine="$UOP_SCRIPTS_ROOT/10-prepare-cross-traces/phase1/PW"
outFileBaseLine="$UOP_SCRIPTS_ROOT/10-prepare-cross-traces/phase2/PW"
mkdir -p $outFileBaseLine

traceIndexPool=('0' '1' '2' '3' '4' '5' '6' '7' '8' '9')
for traceIndex in "${traceIndexPool[@]}"; do
    mkdir -p $outFileBaseLine/$traceIndex
    for testSet in "${benchmark_arr[@]}"; do
        if [ -e "$traceBaseLine/$traceIndex/$testSet.csv" ]; then
            scriptName="./build/copyTrace"
            $scriptName $traceBaseLine/$traceIndex/$testSet.csv $outFileBaseLine/$traceIndex/$testSet.csv &
        fi
    done
    wait
    echo "finish $traceIndex"
done

wait