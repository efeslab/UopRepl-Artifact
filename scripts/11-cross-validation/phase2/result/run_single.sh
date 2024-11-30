sim_binary="$UOP_SIMULATOR_ROOT/scarab_ipc_debug/src/scarab";

params="../baseline/PARAMS.in"
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper' 'kafka'  'mediawiki'  'tomcat' 'clang' 'wordpress' 'mysql'  'postgres'  'python' )

TraceBaseLine="$UOP_DATACENTER_TRACES"
TraceEnding="trace"
ConfigPath="../config"
PostConfigPath="../post-config"
ProcessPath="../process"
OracleFooTraceBase="$HOME/uop/0320_generate_FOO/result"
OnlineFooTraceBase="$HOME/uop/_foo_"

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
  appName=$(echo $configBaseName | cut -d'_' -f1)
  traceIndex=$(echo $configBaseName | cut -d'_' -f2 | cut -d'.' -f1)

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

    OutputFolder=$dir/$configCoreName
    mkdir -p $OutputFolder
    cp $params $OutputFolder/PARAMS.in;
    
    touch $OutputFolder/bypass.out
(   
    # Preprocess
    if [ -f "$preprocessFile" ]; then
      dos2unix $preprocessFile
      bash $preprocessFile $dir $OutputFolder $OracleFooTraceBase $OnlineFooTraceBase $appName
    fi
  
    #first pass
    cd $OutputFolder
    $sim_binary $userParam &> log.txt 
    cd $dir
)&

    cd $dir
    activeCount=$(ps aux | grep 'scarab' | awk '{print $2}' | wc -l)
  while ((activeCount>=$concurrency)); do
    sleep 120
    activeCount=$(ps aux | grep 'scarab' | awk '{print $2}' | wc -l)
  done
  done  
wait;

now=$(date +"%T")
echo "Finish time : $now"
end_time=`date +%s`
echo "execution time was" `expr $end_time - $start_time` s.