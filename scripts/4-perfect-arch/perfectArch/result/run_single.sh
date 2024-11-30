sim_binary="$UOP_SIMULATOR_ROOT/scarab_ipc_debug/src/scarab";

params="../baseline/PARAMS.in"
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat' 'clang' 'wordpress' 'mysql'  'python'  'postgres')

TraceBaseLine="$UOP_DATACENTER_TRACES"
TraceEnding="trace"
# ConfigPath="../tmpConfig"
ConfigPath="../config"
PostConfigPath="../post-config"
ProcessPath="../process"
OracleFooTraceBase="$HOME/uop/1120_profile_traces/phase2/result"
OnlineFooTraceBase="$HOME/uop/1120_profile_traces/phase7/UOP"

python3 pythonDependencyCheck.py

INSTCOUNT=100000
concurrency=500
if [ "$#" -eq 1 ]; then
  INSTCOUNT=$1
fi

now=$(date +"%T")
echo "Current time : $now"
start_time=`date +%s`
echo "Runing simulation with $INSTCOUNT instructions"

dir=$(pwd)


for configPath in $ConfigPath/*.config; do
  configBaseName="$(basename ${configPath})" # abc.config
  configCoreName="${configBaseName%.*}" ## abc
  shellFile=$dir/../process/$configCoreName.sh
  preprocessFile=$dir/../preprocess/$configCoreName.sh
  postConfig=$dir/../post-config/$configCoreName.config
  dos2unix $postConfig
  dos2unix $configPath
  mkdir -p $configCoreName

  userParam=$(<$configPath)
  userParam="${userParam//$'\n'/ }"
  echo $userParam

  if [ -f "$postConfig" ]; then
  userPostParam=$(<$postConfig)
  userPostParam="${userPostParam//$'\n'/ }"
  echo $userPostParam
  fi


  for testSet in "${benchmark_arr[@]}"; do
    benchmarkFile=$TraceBaseLine/$testSet/$TraceEnding
    OutputFolder=$dir/$configCoreName/$testSet
    mkdir -p $OutputFolder
    cp $params $OutputFolder/PARAMS.in;
    
    touch $OutputFolder/bypass.out
(   
    # Preprocess
    if [ -f "$preprocessFile" ]; then
      dos2unix $preprocessFile
      bash $preprocessFile $dir $OutputFolder $OracleFooTraceBase $OnlineFooTraceBase $testSet
    fi
  
    #first pass
    cd $OutputFolder
    $sim_binary $userParam --cbp_trace_r0=$benchmarkFile --inst_limit $INSTCOUNT &> log.txt 
    cd $dir

    #intermediate processing
    if [ -f "$shellFile" ]; then
      dos2unix $shellFile
      $shellFile $dir $OutputFolder &> $OutputFolder/py.out
    fi

    #second pass
    cd $OutputFolder
    if [ -f "$postConfig" ]; then
      $sim_binary $userPostParam --cbp_trace_r0=$benchmarkFile --inst_limit $INSTCOUNT &> log2.txt 
    fi
    
    cd $dir
)&

    cd $dir
    activeCount=$(ps aux | grep 'scarab' | awk '{print $2}' | wc -l)
  while ((activeCount>=$concurrency)); do
    sleep 120
    activeCount=$(ps aux | grep 'scarab' | awk '{print $2}' | wc -l)
  done
  done  
done
wait;

cd ../analyze
./process.sh 


now=$(date +"%T")
echo "Finish time : $now"
end_time=`date +%s`
echo "execution time was" `expr $end_time - $start_time` s.



