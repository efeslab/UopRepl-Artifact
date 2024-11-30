rm -rf build
mkdir build
cd build
cmake ..
make
cd ..

benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat'  'postgres' 'clang' 'wordpress' 'mysql'  'python' )
traceBaseLine="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase1/PW"
outFileBaseLine="$UOP_SCRIPTS_ROOT/2-prepare-traces/phase2/PW"
mkdir -p $outFileBaseLine

# traceIndexPool=('5' '6' '7' '8' '9')

# for traceIndex in "${traceIndexPool[@]}"; do
#     mkdir -p $outFileBaseLine/$traceIndex
#     for testSet in "${benchmark_arr[@]}"; do
#         if [ -e "$traceBaseLine/$traceIndex/$testSet.csv" ]; then
#             scriptName="./build/copyTrace"
#             $scriptName $traceBaseLine/$traceIndex/$testSet.csv $outFileBaseLine/$traceIndex/$testSet.csv &
#         fi
#     done
#     wait
#     echo "finish $traceIndex"
# done

# wait

for testSet in "${benchmark_arr[@]}"; do
    if [ -e "$traceBaseLine/$testSet.csv" ]; then
        scriptName="./build/copyTrace"
        $scriptName $traceBaseLine/$testSet.csv $outFileBaseLine/$testSet.csv &
    fi
done

wait