sim_binary="/mnt/storage/shuwend/code_library/scarab_hlitz/src/scarab";
#"/mnt/storage/shuwend/code_library/scarab_hlitz/src/bin/Linux/opt/scarab";
cp ../../src/PARAMS.kaby_lake PARAMS.in
params="/mnt/storage/shuwend/code_library/scarab_hlitz/test/uop_tests/PARAMS.in"
benchmark_arr=('clang')
#('postgres')
#('cassandra'  'drupal'  'finagle-chirper'  'finagle-http'  'kafka'  'mediawiki'  'tomcat'  'verilator'  'wordpress'  'clang'  'mysql'  'python')
#('postgres')
#('cassandra'  'drupal'  'finagle-chirper'  'finagle-http'  'kafka'  'mediawiki'  'tomcat'  'verilator'  'wordpress'  'clang'  'mysql'  'python')
# ('clang')
#('cassandra'  'drupal'  'finagle-chirper'  'finagle-http'  'kafka'  'mediawiki'  'tomcat'  'verilator'  'wordpress'  'clang'  'mysql'  'python')
#'postgres'  
#"drupal" #"mediawiki" #"drupal" #"postgres"
file0="/mnt/storage/shuwend/code_library/trace_lists/traces/"
file1="/trace"
files="$file0${benchmark_arr[0]}$file1";
file2="/mnt/storage/shuwend/code_library/trace_lists/four-new-traces/"
# files="$file2$benchmark";


second="" #$2 #"" #"size4096OPT" #fourth"
bench_dir0="dc_bench"
bench_dir="$bench_dir0$1"
mkdir -p $bench_dir;
# echo $bench_dir

exe_count=$3
exe_count_name=$4
counter_bit=3


for index in {0..0} #12}
do

    # # 1st crew

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="trancacheaccess" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 16 --uop_cache_exe_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #   fi
    #     if ((COUNTER%1==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="lrurepl"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%1==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    
    # # 2nd crew

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="wholecacheaccess" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &    
    #   fi
    #     if ((COUNTER%1==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="wholecacheaccess" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

    #   fi
    #     if ((COUNTER%1==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;





    # # 3rd crew


    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="3bitcacheaccess" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &    
    #   fi
    #     if ((COUNTER%1==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd ../../scripts;
    # mkdir -p $bench_dir
    # time OMP_NUM_THREADS=4 python3 jenks.py $bench_dir $counter_bit
    # cd ../test/uop_tests/;

    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<1))  #13
    do
      COUNTER=$((COUNTER+1));	
      perfectop="3bitcacheaccess" #$exe_count_name #"cachestore"
      # echo $perfectop
      echo "Benchmark $((COUNTER-1))"
      files="$file0${benchmark_arr[COUNTER-1]}$file1" 
      echo $sim_binary
      # echo $second${benchmark_arr[COUNTER-1]}$perfectop
      mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
      cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
      cp $params PARAMS.in;
      if (( COUNTER > 0 )); then
          echo $files
          # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
          # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream_w8.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

      fi
        if ((COUNTER%1==0));  #13
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;





    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1)) 
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="record" #"srrip" #"record" #"istagefetch" # "writerecord" #"lrurepl"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 12 --uop_rrip_pw 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &

    #   fi
    #     if ((COUNTER%1==0)); 
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<1)) 
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="record" #"srripori" #"record" #"istagefetch" # "writerecord" #"lrurepl"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out"  --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 12 --uop_rrip_pw 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &

    #   fi
    #     if ((COUNTER%1==0)); 
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;


    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13)) 
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="opt" #"record" #"opfetch" # "readrecord" #"lrurepl"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out"  --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

    #   fi
    #     if ((COUNTER%13==0)); 
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;


    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="cacheaccess" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &    
    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd ../../scripts;
    # time OMP_NUM_THREADS=4 python3 jenks.py $bench_dir
    # cd ../test/uop_tests/;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="cacheaccess" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream_w.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="recordopt" #$exe_count_name #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # --uop_cache_repl_policy 16 --uop_cache_exe_count

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="srrip" #"cachestore"
    #   # echo $perfectop
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   # echo $second${benchmark_arr[COUNTER-1]}$perfectop
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 12 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # # --uop_cache_repl_policy 11


    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="drrip"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 13 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # # --uop_cache_repl_policy 16 --uop_cache_exe_count 3

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="shippp"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 14 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="cacheaccess"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 16 --uop_cache_exe_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="lrurepl"
    #   echo "Benchmark $((COUNTER-1))"
    #   files="$file0${benchmark_arr[COUNTER-1]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cd "$second${benchmark_arr[COUNTER-1]}$perfectop"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%13==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    




done

# --uop_cache_size 4096






