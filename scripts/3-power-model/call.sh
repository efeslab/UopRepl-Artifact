targetPath=$1

CACTI_EXE=$UOP_SCRIPTS_ROOT/3-power-model/mcpat/cacti/cacti
MCPAT_EXE=$UOP_SCRIPTS_ROOT/3-power-model/mcpat/mcpat

startTime=$(date +%s)
echo "Start running CACTI and McPAT: start at $startTime"

benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat' 'clang' 'wordpress' 'mysql'  'python'  'postgres')


rm $targetPath/power.csv
touch $targetPath/power.csv
echo "app,repl,value" >> $targetPath/power.csv

# iterate on the subdir of the target path
# only directories
subDirArr=($(ls -d $targetPath/*/))
for subDir in ${subDirArr[@]}; do
    # get the directory name
    repl=$(basename $subDir)
    for testcase in "${benchmark_arr[@]}"; do
        (
            dir=$subDir/$testcase
            $UOP_SCRIPTS_ROOT/3-power-model/toxml $dir
            cd $dir
            $CACTI_EXE -infile cacti_infile.cfg > cacti_dram.out
            $MCPAT_EXE -dump_design -infile mcpat_infile.xml -print_level 5 > mcpat.out
            powerValue=$(awk '{
                for (i=1; i<=NF; i++) {
                    if ($i ~ /^[0-9]*\.[0-9]+$/) {
                        print $i
                        exit
                    }
                }
            }' mcpat.out)
            echo "$testcase,$repl,$powerValue" >> $targetPath/power.csv
        )&
    done
done

wait