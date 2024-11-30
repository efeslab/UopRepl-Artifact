sim_binary="/mnt/storage/shuwend/code_library/scarab_hlitz/src/scarab";
#"/mnt/storage/shuwend/code_library/scarab_hlitz/src/bin/Linux/opt/scarab";
cp ../../src/PARAMS.kaby_lake PARAMS.in
params="/mnt/storage/shuwend/code_library/scarab_hlitz/test/uop_tests/PARAMS.in"
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper'  'finagle-http'  'kafka'  'mediawiki'  'tomcat'  'verilator'  'wordpress'  'clang'  'mysql'  'postgres'  'python')
#"drupal" #"mediawiki" #"drupal" #"postgres"
file0="/mnt/storage/shuwend/code_library/trace_lists/traces/"
file1="/trace"
files="$file0${benchmark_arr[0]}$file1";
file2="/mnt/storage/shuwend/code_library/trace_lists/four-new-traces/"
# files="$file2$benchmark";


second="" #fourth"
bench_dir0="dc_bench"
bench_dir="$bench_dir0$1"
mkdir -p $bench_dir;


for index in {0..12}
    do
    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<2)) 
    do
      COUNTER=$((COUNTER+1));	
      echo "Benchmark $index"
      files="$file0${benchmark_arr[index]}$file1" #"$file0${benchmark_arr[index]}";
      echo $sim_binary
      mkdir -p "$second${benchmark_arr[index]}$COUNTER"; 
      cd "$second${benchmark_arr[index]}$COUNTER"; 
      cp $params PARAMS.in;
      if (( COUNTER > 0 )); then
          echo $files
          $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 11 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

          # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

      fi
        if ((COUNTER%2==0)); 
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;



    x='

    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<2)) 
    do
      COUNTER=$((COUNTER+1));	
      perfectop="idealrepl" #"ideal"
      echo "Benchmark $index"
      files="$file0${benchmark_arr[index]}$file1" #"$file0${benchmark_arr[index]}$perfectop";
      echo $sim_binary
      mkdir -p "$second${benchmark_arr[index]}$perfectop$COUNTER"; 
      cd "$second${benchmark_arr[index]}$perfectop$COUNTER"; 
      cp $params PARAMS.in;
      if (( COUNTER > 0 )); then
        echo $files
        $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 1000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

        # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 4 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
      fi
        if ((COUNTER%2==0)); 
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;

    '

done








