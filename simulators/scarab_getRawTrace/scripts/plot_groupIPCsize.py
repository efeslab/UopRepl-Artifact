import numpy as np
import matplotlib.pyplot as plt
import sys

folder_name = sys.argv[1] #dc_benchv15
tests = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"] #   #,  "drupal"
tests_name = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"] # , "postgres" 
tests_name_speedup = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python", "Avg", "Avg w/o verilator"]  # , "postgres" 
        #tests

size_file_name = ["dc_benchv28_OPT_0814edits_v6", "dc_benchv29_OPT_0816size4k_v7", 
"dc_benchv30_OPT_0817size8k_v8", "dc_benchv42_OPT_0822_size_v14",
"dc_benchv41_OPT_0826_520k_v15"]
larger_partile_file= [16384, 32768, 65536, 131072, 262144, 524288, 1048576]
uop_cache_misses=[[],[],[],[],[],[],[],[],[],[],[],[]]
uop_cache_miss_rates=[[],[],[],[],[],[],[],[],[],[],[],[]]
icache_misses=[[],[],[],[],[],[],[],[],[],[],[],[]]
IPC_lists=[[],[],[],[],[],[],[],[],[],[],[],[]]

uop_accumulate=[]
uop_cache_pws_inserted=[]
uop_cache_lines_inserted=[]
uop_cache_lines_used=[]
uop_cache_pw_insert_failed_cache_hit=[]
uop_cache_icache_switch_br_not_taken_resteered=[]
uop_cache_icache_switch_br_not_taken_correct_pred=[]
uop_cache_icache_switch_br_taken_resteered=[]
uop_cache_icache_switch_br_taken_correct_pred=[]


# type 0 LRU; type 1 OPT
for i in range(len(tests)):
        for typ in range(11):#10):
                if typ<=2: # and i!=2 and i!=3: # and
                        given_file = open("../test/uop_tests/"+size_file_name[typ]+"/"+tests[i]+"lrurepl/memory.stat.0.out", 'r')    
                        log_file = open("../test/uop_tests/"+size_file_name[typ]+"/"+tests[i]+"lrurepl/log.txt", 'r')
                elif typ==10:
                        given_file = open("../test/uop_tests/"+size_file_name[4]+"/"+tests[i]+"/memory.stat.0.out", 'r')    
                        log_file = open("../test/uop_tests/"+size_file_name[4]+"/"+tests[i]+"/log.txt", 'r')
                        # print("../test/uop_tests/"+size_file_name[4]+"/"+tests[i]+"/memory.stat.0.out")
                else:
                        given_file = open("../test/uop_tests/"+size_file_name[3]+"/"+str(larger_partile_file[typ-3])+tests[i]+"lrurepl/memory.stat.0.out", 'r')
                        log_file = open("../test/uop_tests/"+size_file_name[3]+"/"+str(larger_partile_file[typ-3])+tests[i]+"lrurepl/log.txt", 'r')
                lines = given_file.readlines()
                log_lines = log_file.readlines()

                for line in lines:
                        if " Instructions:" in line:
                                inst_num = " ".join(line.strip().split()).split(" ")[-3]
                                ipc_num = " ".join(line.strip().split()).split(" ")[-1]
                                # print(inst_num)
                        if "UOP_CACHE_MISS" in line:
                                tmp_miss = " ".join(line.strip().split()).split(" ")[1]
                                uop_miss_rate = " ".join(line.strip().split()).split(" ")[2][:-1]
                                # print(tmp_miss)
                        if "ICACHE_MISS " in line:
                                icache_miss = " ".join(line.strip().split()).split(" ")[1]
                                # print(icache_miss)

                                
                uop_cache_misses[typ].append(float(tmp_miss)/float(inst_num)*1000)
                uop_cache_miss_rates[typ].append(float(uop_miss_rate))
                icache_misses[typ].append(float(icache_miss)/float(inst_num)*1000)
                IPC_lists[typ].append(float(ipc_num))

                given_file.close()
                log_file.close()
# print(uop_cache_misses)    
# print(uop_cache_miss_rates)  
# print(icache_misses) 
print(IPC_lists)


 
# set height of bar

# set width of bar
barWidth = 0.07
fig = plt.subplots(figsize =(45, 8))

# Set position of bar on X axis
br1 = np.arange(len(IPC_lists[0]))
br2 = [x + barWidth for x in br1]
br3 = [x + barWidth for x in br2]
br4 = [x + barWidth for x in br3]
br5 = [x + barWidth for x in br4]
br6 = [x + barWidth for x in br5]
br7 = [x + barWidth for x in br6]
br8 = [x + barWidth for x in br7]
br9 = [x + barWidth for x in br8]
br10 = [x + barWidth for x in br9]
br11 = [x + barWidth for x in br10]
 
# Make the plot
plt.bar(br1, IPC_lists[0], color ='#FFCCCC', width = barWidth,
        edgecolor ='grey', label ='2K')
plt.bar(br2, IPC_lists[1], color ='#FF6666', width = barWidth,
        edgecolor ='grey', label ='4K')
plt.bar(br3, IPC_lists[2], color ='#FF0000', width = barWidth,
        edgecolor ='grey', label ='8K')
plt.bar(br4, IPC_lists[3], color ='#CCFFCC', width = barWidth,
        edgecolor ='grey', label ='16K')
plt.bar(br5, IPC_lists[4], color ='#66FF66', width = barWidth,
        edgecolor ='grey', label ='32K')
plt.bar(br6, IPC_lists[5], color ='#00FF00', width = barWidth,
        edgecolor ='grey', label ='64K')
plt.bar(br7, IPC_lists[6], color ='#FF9966', width = barWidth,
        edgecolor ='grey', label ='128K')
plt.bar(br8, IPC_lists[7], color ='#FF6666', width = barWidth,
        edgecolor ='grey', label ='256K')
plt.bar(br9, IPC_lists[8], color ='#FF0066', width = barWidth,
        edgecolor ='grey', label ='512K')
plt.bar(br10, IPC_lists[9], color ='#CC6699', width = barWidth,
        edgecolor ='grey', label ='1M')
plt.bar(br11, IPC_lists[10], color ='k', width = barWidth,
        edgecolor ='grey', label ='512K OPT')

# Adding Xticks
plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('IPC', fontweight ='bold', fontsize = 15)
plt.xticks([r + 5*barWidth for r in range(len(IPC_lists[0]))],
        tests_name, fontsize = 15)
plt.yticks(fontsize = 15)
plt.grid(axis='y', color='0.95')
for index, value in enumerate(IPC_lists[0]):
    plt.text(br1[index]-0.15, value+0.05,
            str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[1]):
    plt.text(br2[index]-0.1, value+0.08,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[2]):
    plt.text(br3[index]-0.05, value+0.11,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[3]):
    plt.text(br4[index]-0.15, value+0.05,
            str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[4]):
    plt.text(br5[index]-0.1, value+0.08,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[5]):
    plt.text(br6[index]-0.05, value+0.11,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[6]):
    plt.text(br7[index]-0.05, value+0.21,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[7]):
    plt.text(br8[index]-0.05, value+0.31,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[8]):
    plt.text(br9[index]-0.05, value+0.41,
             str(round(value,2)), fontsize = 15, color ='#FF0066')
for index, value in enumerate(IPC_lists[9]):
    plt.text(br10[index]-0.05, value+0.51,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[10]):
    plt.text(br11[index], value+0.41,
             str(round(value,2)), fontsize = 15, color ='b')
 
plt.legend(fontsize = 15)
plt.savefig("dc_bench_IPC_v"+sys.argv[1]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()



# set height of bar

# set width of bar
barWidth = 0.07
fig = plt.subplots(figsize =(45, 8))

# Set position of bar on X axis
br1 = np.arange(len(IPC_lists[0])+2)
br2 = [x + barWidth for x in br1]
br3 = [x + barWidth for x in br2]
br4 = [x + barWidth for x in br3]
br5 = [x + barWidth for x in br4]
br6 = [x + barWidth for x in br5]
br7 = [x + barWidth for x in br6]
br8 = [x + barWidth for x in br7]
br9 = [x + barWidth for x in br8]
br10 = [x + barWidth for x in br9]

IPC_speedup=[[],[],[],[],[],[],[],[],[],[]]
IPC_avg=[0,0,0,0,0,0,0,0,0,0]
IPC_avgwoveri=[0,0,0,0,0,0,0,0,0,0]
for j in range(10): #9):
  for i in range(len(IPC_lists[0])):
        IPC_speedup[j].append(100*((IPC_lists[j+1][i]/IPC_lists[0][i]) - 1))
        IPC_avg[j]+=100*((IPC_lists[j+1][i]/IPC_lists[0][i]) - 1)
        if i!=7:
          IPC_avgwoveri[j]+=100*((IPC_lists[j+1][i]/IPC_lists[0][i]) - 1)
  IPC_speedup[j].append(IPC_avg[j]/13) #13)
  IPC_speedup[j].append(IPC_avgwoveri[j]/12) #12)

# for i in range(len(IPC_lists[0])):
#       IPC_speedup[3].append(100*((IPC_lists[1][i]/IPC_lists[0][i]) - 1))
#       IPC_avg[3]+=(100*((IPC_lists[1][i]/IPC_lists[0][i]) - 1))
#       if i!=7:
#         IPC_avgwoveri[3]+=(100*((IPC_lists[1][i]/IPC_lists[0][i]) - 1))
# IPC_speedup[3].append(IPC_avg[3]/13)
# IPC_speedup[3].append(IPC_avgwoveri[3]/12)
 
# for i in range(len(IPC_lists[0])):
#       IPC_speedup[4].append(100*((IPC_lists[2][i]/IPC_lists[0][i]) - 1))
#       IPC_avg[4]+=(100*((IPC_lists[2][i]/IPC_lists[0][i]) - 1))
#       if i!=7:
#         IPC_avgwoveri[4]+=(100*((IPC_lists[2][i]/IPC_lists[0][i]) - 1))
# IPC_speedup[4].append(IPC_avg[4]/13)
# IPC_speedup[4].append(IPC_avgwoveri[4]/12)

# Make the plot
plt.bar(br1, IPC_speedup[0], color ='#CCE5FF', width = barWidth,
        edgecolor ='grey', label ='4K')
plt.bar(br2, IPC_speedup[1], color ='#66B2FF', width = barWidth,
        edgecolor ='grey', label ='8K')
plt.bar(br3, IPC_speedup[2], color ='#0080FF', width = barWidth,
        edgecolor ='grey', label ='16K')
plt.bar(br4, IPC_speedup[3], color ='#009999', width = barWidth,
        edgecolor ='grey', label ='32K')
plt.bar(br5, IPC_speedup[4], color ='#003333', width = barWidth,
        edgecolor ='grey', label ='64K')
plt.bar(br6, IPC_speedup[5], color ='#00FF00', width = barWidth,
        edgecolor ='grey', label ='128K')
plt.bar(br7, IPC_speedup[6], color ='#FF9966', width = barWidth,
        edgecolor ='grey', label ='256K')
plt.bar(br8, IPC_speedup[7], color ='#FF6666', width = barWidth,
        edgecolor ='grey', label ='512K')
plt.bar(br9, IPC_speedup[8], color ='#FF0066', width = barWidth,
        edgecolor ='grey', label ='1M')
plt.bar(br10, IPC_speedup[9], color ='k', width = barWidth,
        edgecolor ='grey', label ='512K OPT')

# Adding Xticks
plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('IPC Speedup (%)', fontweight ='bold', fontsize = 15)
plt.xticks([r + 5*barWidth for r in range(len(IPC_speedup[0]))],
        tests_name_speedup , fontsize = 15)
plt.yticks(fontsize = 15)
plt.grid(axis='y', color='0.95')
plt.ylim([0, 200])
for index, value in enumerate(IPC_speedup[0]):
    plt.text(br1[index]-0.2, min(200,value+0.05),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[1]):
    plt.text(br2[index]-0.1, min(200,value+1.07),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[2]):
    plt.text(br3[index]-0.05, min(200,value+2.09),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[3]):
    plt.text(br4[index]-0.01, min(200,value+3.05),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[4]):
    plt.text(br5[index]-0.01, min(200,value+4.05),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[5]):
    plt.text(br6[index]-0.1, min(200,value+5.07),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[6]):
    plt.text(br7[index]-0.05, min(200,value+6.09),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[7]):
    plt.text(br8[index]-0.01, min(200,value+13.05),
             str(round(value,2)), fontsize = 15, color ='#FF6666')
for index, value in enumerate(IPC_speedup[8]):
    plt.text(br9[index]-0.01, min(200,value+18.1),
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_speedup[9]):
    plt.text(br10[index]+0.15, min(200,value+13),
             str(round(value,2)), fontsize = 15, color ='b')
 
plt.legend(fontsize = 15)
plt.savefig("dc_bench_IPCspeedup_cut_v"+sys.argv[1]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()

