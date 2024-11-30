sim_binary="/mnt/storage/shuwend/code_library/scarab_hlitz/src/scarab";
#"/mnt/storage/shuwend/code_library/scarab_hlitz/src/bin/Linux/opt/scarab";
cp ../../src/PARAMS.kaby_lake PARAMS.in
params="/mnt/storage/shuwend/code_library/scarab_hlitz/test/uop_tests/PARAMS.in"
benchmark_arr=('cassandra'  'drupal'  'finagle-chirper'  'finagle-http'  'kafka'  'mediawiki'  'tomcat'  'verilator'  'wordpress'  'clang'  'mysql'  'python')
#"drupal"
#('cassandra'  'drupal'  'finagle-chirper'  'finagle-http'  'kafka'  'mediawiki'  'tomcat'  'verilator'  'wordpress'  'clang'  'mysql'  'postgres'  'python')
#"drupal" 
#"cassandra  drupal  finagle-chirper  finagle-http  kafka  mediawiki  tomcat  verilator  wordpress  clang  mysql  postgres  python"
#"drupal" #"mediawiki" #"drupal" #"postgres"
file0="/mnt/storage/shuwend/code_library/trace_lists/traces/"
file1="/trace"
files="$file0${benchmark_arr[0]}$file1";
file2="/mnt/storage/shuwend/code_library/trace_lists/four-new-traces/"
# files="$file2$benchmark";
# echo $files


second="" #fourth"
bench_dir0="dc_bench"
bench_dir="$bench_dir0$1"
mkdir -p $bench_dir;
counter_bit=3
feature_bit=$2
bug_fixed=$3


  # REPL_OPT,     /* Based on the Belady's algorithm and Hawkeye*/ /* 11 */ 
  # REPL_SRRIP,     /* Based on RRIP paper*/ /* 12 */ 
  # REPL_DRRIP,     /* Based on RRIP paper*/ /* 13 */ 
  # REPL_SHIP_PP,     /* Based on SHiP++ paper*/ /* 14 */ 
  # REPL_MOCKINGJAY,  /* Based on mockingjay paper */ /* 15 */ 
  # REPL_NEW,  /* 16 */ 
  # REPL_FETCH_ADDR, /* 17 */ 

# --footprint "execution_count_stream.out" 
for index in {0..0} #12}
do

    # # 1st crew

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<12))  #13
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
    #       # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 16 --uop_cache_exe_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 19 --uop_cache_usage_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # # $sim_binary --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
 
    #   fi
    #     if ((COUNTER%12==0));  #13
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
    # while ((COUNTER<12))  #13
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
    #       $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%12==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # 2nd crew

    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<12))  #13
    do
      COUNTER=$((COUNTER+1));	
      perfectop="wholecacheaccess" #$exe_count_name #"cachestore"
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
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
          # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 19 --uop_cache_usage_count 2 --uop_cache_enable_record 1 --footprint "cacheaccess_stream.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
          $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 1 --footprint "pmc_stream.out" --footprint_switch "pmc_stream_switch.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
          # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 1 --footprint "pmc_stream.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &

      fi
        if ((COUNTER%12==0));  #13
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;

    # cd ../../scripts;
    # mkdir -p $bench_dir
    # time OMP_NUM_THREADS=4 python3 jenks_concat.py $bench_dir $counter_bit whole
    # cd ../test/uop_tests/;

    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<12))  #13
    do
      COUNTER=$((COUNTER+1));	
      perfectop="wholecacheaccess" #$exe_count_name #"cachestore"
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
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 19 --uop_cache_usage_count 2 --uop_cache_enable_record 0 --footprint "cacheaccess_stream.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 0 --footprint "pmc_stream.out" --footprint_switch "pmc_stream_switch.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 0 --footprint "pmc_stream.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

      fi
        if ((COUNTER%12==0));  #13
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;


    # 3rd crew


    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<12))  #13
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
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
          # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 19 --uop_cache_usage_count 2 --uop_cache_enable_record 1 --footprint "cacheaccess_stream.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
          $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 1 --footprint "pmc_stream.out" --footprint_switch "pmc_stream_switch.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
          # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 1 --footprint "pmc_stream.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &

      fi
        if ((COUNTER%12==0));  #13
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;

    cd ../../scripts;
    mkdir -p $bench_dir
    # time OMP_NUM_THREADS=4 python3 jenks_concat_completePW.py $bench_dir $counter_bit 3bit
    time OMP_NUM_THREADS=4 python3 jenks.py $bench_dir $counter_bit 
    cd ../test/uop_tests/;

    cd $bench_dir;
    COUNTER=0;
    while ((COUNTER<12))  #13
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
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
          # # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream_w8.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 19 --uop_cache_usage_count 2 --uop_cache_enable_record 0 --footprint "cacheaccess_stream_w8.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 0 --footprint "pmc_stream_w8.out" --footprint_switch "pmc_stream_switch_w8.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
          # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 20 --uop_cache_usage_count 2 --uop_cache_enable_record 0 --footprint "pmc_stream_w8.out"  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

      fi
        if ((COUNTER%12==0));  #13
        then
      echo "waiting $COUNTER";
      wait;
        fi
      cd ..;
    done  
    cd ..;
    wait;













    # # OPT crew

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<12))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="opt" #""srrip" #"opt" #"srrip" #"record" #"istagefetch" # "writerecord" #"lrurepl"
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
    #       # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       # # $sim_binary --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --uop_opt_cache_access 1 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 12 --uop_rrip_pw 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --uop_opt_cache_access 1 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
    #       # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1 --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --uop_opt_cache_access 1 --footprint "new_opt_stream.out" --uop_cache_enable_record 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &

    #   fi
    #     if ((COUNTER%12==0));  #13
    #     then
    #   echo "waiting $COUNTER";
    #   wait;
    #     fi
    #   cd ..;
    # done  
    # cd ..;
    # wait;

    # # --uop_cache_repl_policy 17 --uop_cache_enable_record 1
    # # --uop_cache_repl_policy 16 --uop_cache_exe_count 2 

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<12))  #13
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="opt" #""srrip" #"opt" #"srrip" #"record" #"istagefetch" # "writerecord" #"lrurepl"
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
    #       # # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       $sim_binary --uop_cache_feature $feature_bit --uop_cache_fix_bug $bug_fixed --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --uop_opt_cache_access 1 --footprint "new_opt_stream.out"  --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &
    #       # $sim_binary --perfect_bp 1 --perfect_btb 1 --perfect_ibp 1 --perfect_crs 1 --perfect_cbr_btb 1 --perfect_nt_btb 1 --perfect_mlc 1 --perfect_l1 1 --perfect_icache 1 --perfect_dcache 1  --fetch_across_cache_lines 0 --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 17 --uop_opt_cache_access 1 --footprint "new_opt_stream.out"  --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

    #   fi
    #     if ((COUNTER%12==0));  #13
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
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 16 --uop_cache_exe_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
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
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
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

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
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
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

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








    #  cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
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
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --footprint "cacheaccess_stream.out" --uop_cache_enable_record 1 --uop_cache_exe_count 1   --uop_access_count_bit $counter_bit --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log1.txt &
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
    # mkdir -p $bench_dir
    # time OMP_NUM_THREADS=4 python3 jenks.py $bench_dir $counter_bit
    # cd ../test/uop_tests/;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<13))  #13
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
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 18 --uop_cache_exe_count 1 --footprint "cacheaccess_stream_w8.out"  --uop_access_count_bit $counter_bit --uop_cache_enable_record 0  --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log2.txt &

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




   

    # # --uop_cache_repl_policy 17 --uop_cache_enable_record 0
    # # --uop_cache_repl_policy 16 --uop_cache_exe_count 3

    # # cd $bench_dir;
    # # COUNTER=0;
    # # while ((COUNTER<2)) 
    # # do
    # #   COUNTER=$((COUNTER+1));	
    # #   perfectop="cacheaccessfetchaddr" #"lrurepl"
    # #   echo "Benchmark $index"
    # #   files="$file0${benchmark_arr[index]}$file1" 
    # #   echo $sim_binary
    # #   mkdir -p "$second${benchmark_arr[index]}$perfectop$COUNTER"; 
    # #   cd "$second${benchmark_arr[index]}$perfectop$COUNTER"; 
    # #   cp $params PARAMS.in;
    # #   if (( COUNTER > 0 )); then
    # #       echo $files
    # #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    # #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 16 --uop_cache_exe_count 1 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    # #   fi
    # #     if ((COUNTER%2==0)); 
    # #     then
    # #   echo "waiting $COUNTER";
    # #   wait;
    # #     fi
    # #   cd ..;
    # # done  
    # # cd ..;
    # # wait;

    # cd $bench_dir;
    # COUNTER=0;
    # while ((COUNTER<2)) 
    # do
    #   COUNTER=$((COUNTER+1));	
    #   perfectop="lrurepl" #"opt" #"lrurepl" #"lrurepl"
    #   echo "Benchmark $index"
    #   files="$file0${benchmark_arr[index]}$file1" 
    #   echo $sim_binary
    #   mkdir -p "$second${benchmark_arr[index]}$perfectop$COUNTER"; 
    #   cd "$second${benchmark_arr[index]}$perfectop$COUNTER"; 
    #   cp $params PARAMS.in;
    #   if (( COUNTER > 0 )); then
    #       echo $files
    #       # $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 99000000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &
    #       $sim_binary --frontend pt --oracle_perfect_uop_cache 0 --uop_cache_repl_policy 0 --cbp_trace_r0=$files --fetch_off_path_ops=false --inst_limit 100000 --btb_entries 8192 --btb_assoc 4 --enable_crs 1 --crs_entries 32 --enable_ibp 1 --crs_realistic 1 --use_pat_hist 1 --bp_mech tagescl --btb_mech 0 --fdip_enable 0 &> log.txt &

    #   fi
    #     if ((COUNTER%2==0)); 
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








