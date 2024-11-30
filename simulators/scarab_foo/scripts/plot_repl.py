import numpy as np
import matplotlib.pyplot as plt
import sys

folder_name = sys.argv[1] #dc_benchv15
tests = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"] #,  "drupal"
tests_name = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"]
        #tests

uop_cache_misses=[[],[],[]]
uop_cache_miss_rates=[[],[],[]]
icache_misses=[[],[],[]]
IPC_lists=[[],[],[]]

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
        for typ in range(3):
                if typ==0: # and i!=2 and i!=3: # and
                        given_file = open("../test/uop_tests/"+sys.argv[1]+"/"+tests[i]+"lrurepl/memory.stat.0.out", 'r')    
                        log_file = open("../test/uop_tests/"+sys.argv[1]+"/"+tests[i]+"lrurepl/log.txt", 'r')
                # elif (i==8 or i==11) and typ==1:
                #         given_file = open("../test/uop_tests/"+"dc_benchv15"+"/"+tests[i]+"1/memory.stat.0.out", 'r')
                #         log_file = open("../test/uop_tests/"+"dc_benchv15"+"/"+tests[i]+"1/log.txt", 'r')
                # elif i==2 and typ==1:
                #         given_file = open("../test/uop_tests/"+"dc_benchv17"+"/"+tests[i]+"1/memory.stat.0.out", 'r')    #+"idealrepl1/memory.stat.0.out", 'r')    
                #         log_file = open("../test/uop_tests/"+"dc_benchv17"+"/"+tests[i]+"1/memory.stat.0.out", 'r')    #+"idealrepl1/log.txt", 'r')
                # elif i==3 and typ==1:
                #         given_file = open("../test/uop_tests/"+"dc_benchv18"+"/"+tests[i]+"1/memory.stat.0.out", 'r')    #+"idealrepl1/memory.stat.0.out", 'r')    
                #         log_file = open("../test/uop_tests/"+"dc_benchv18"+"/"+tests[i]+"1/memory.stat.0.out", 'r')    #+"idealrepl1/log.txt", 'r')
                elif typ==2 and i==11: # and i!=2 and i!=3: # and
                       given_file = open("../test/uop_tests/"+"dc_benchv9/idealsecond"+tests[i]+"1/memory.stat.0.out", 'r')    
                       log_file = open("../test/uop_tests/"+"dc_benchv9/idealsecond"+tests[i]+"1/log.txt", 'r')
                elif typ==2: # and i!=2 and i!=3: # and
                        given_file = open("../test/uop_tests/"+"dc_benchv9/ideal"+tests[i]+"1/memory.stat.0.out", 'r')    
                        log_file = open("../test/uop_tests/"+"dc_benchv9/ideal"+tests[i]+"1/log.txt", 'r')
                else:
                        given_file = open("../test/uop_tests/"+sys.argv[1]+"/"+tests[i]+"/memory.stat.0.out", 'r')
                        log_file = open("../test/uop_tests/"+sys.argv[1]+"/"+tests[i]+"/log.txt", 'r')
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
                if typ==1:
                        for line in lines:
                                if "UOP_ACCUMULATE" in line:
                                        uop_0 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_PWS_INSERTED" in line:
                                        uop_1 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_LINES_INSERTED" in line:
                                        uop_2 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_LINES_USED" in line:
                                        uop_3 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_PW_INSERT_FAILED_CACHE_HIT" in line:
                                        uop_4 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_ICACHE_SWITCH_BR_NOT_TAKEN_RESTEERED" in line:
                                        uop_5 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_ICACHE_SWITCH_BR_NOT_TAKEN_CORRECT_PRED" in line:
                                        uop_6 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_ICACHE_SWITCH_BR_TAKEN_RESTEERED" in line:
                                        uop_7 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_ICACHE_SWITCH_BR_TAKEN_CORRECT_PRED" in line:
                                        uop_8 = " ".join(line.strip().split()).split(" ")[1]

                                
                uop_cache_misses[typ].append(float(tmp_miss)/float(inst_num)*1000)
                uop_cache_miss_rates[typ].append(float(uop_miss_rate))
                icache_misses[typ].append(float(icache_miss)/float(inst_num)*1000)
                IPC_lists[typ].append(float(ipc_num))

                given_file.close()
                log_file.close()
        uop_accumulate.append(float(uop_0)/float(inst_num))#*1000)
        uop_cache_pws_inserted.append(float(uop_1)/float(inst_num))#*1000)
        uop_cache_lines_inserted.append(float(uop_2)/float(inst_num))#*1000)
        uop_cache_lines_used.append(float(uop_3)/float(inst_num))#*1000)
        uop_cache_pw_insert_failed_cache_hit.append(float(uop_4)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_not_taken_resteered.append(float(uop_5)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_not_taken_correct_pred.append(float(uop_6)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_taken_resteered.append(float(uop_7)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_taken_correct_pred.append(float(uop_8)/float(inst_num))#*1000)
print(uop_cache_misses)    
print(uop_cache_miss_rates)  
print(icache_misses) 
print(IPC_lists)


 
# set height of bar

# set width of bar
barWidth = 0.25
fig = plt.subplots(figsize =(26, 8))

# Set position of bar on X axis
br1 = np.arange(len(IPC_lists[0]))
br2 = [x + barWidth for x in br1]
br3 = [x + barWidth for x in br2]
 
# Make the plot
plt.bar(br1, IPC_lists[0], color ='r', width = barWidth,
        edgecolor ='grey', label ='REPL_LRU')
plt.bar(br2, IPC_lists[1], color ='g', width = barWidth,
        edgecolor ='grey', label ='REPL_OPT') #'REPL_SHADOW_IDEAL')
plt.bar(br3, IPC_lists[2], color ='b', width = barWidth,
        edgecolor ='grey', label ='ORACLE_CACHE')

# Adding Xticks
plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('IPC', fontweight ='bold', fontsize = 15)
plt.xticks([r + barWidth for r in range(len(IPC_lists[0]))],
        tests_name, fontsize = 15)

for index, value in enumerate(IPC_lists[0]):
    plt.text(br1[index]-0.2, value+0.05,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[1]):
    plt.text(br2[index]-0.1, value+0.05,
             str(round(value,2)), fontsize = 15)
for index, value in enumerate(IPC_lists[2]):
    plt.text(br3[index]-0.1, value+0.15,
             str(round(value,2)), fontsize = 15)
 
plt.legend(fontsize = 15)
plt.savefig("dc_bench_replOPT_IPC_v"+sys.argv[1].split("_")[-2]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()



# set height of bar

# set width of bar
barWidth = 0.25
fig = plt.subplots(figsize =(26, 8))

# Set position of bar on X axis
br1 = np.arange(len(IPC_lists[0]))

IPC_speedup=[]
for i in range(len(IPC_lists[0])):
        IPC_speedup.append(100*((IPC_lists[1][i]/IPC_lists[0][i]) - 1))
 
# Make the plot
plt.bar(br1, IPC_speedup, color ='g', width = barWidth,
        edgecolor ='grey', label ='REPL_IPC Speedup')

# Adding Xticks
plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
plt.ylabel('IPC Speedup (%)', fontweight ='bold', fontsize = 15)
plt.xticks([r for r in range(len(IPC_lists[0]))],
        tests_name, fontsize = 15)

for index, value in enumerate(IPC_speedup):
    plt.text(br1[index]-0.2, value+0.05,
             str(round(value,2)), fontsize = 15)
 
plt.legend(fontsize = 15)
plt.savefig("dc_bench_replOPT_IPCspeedup_v"+sys.argv[1].split("_")[-2]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()


plot_lists=[[],[],[]]
plot_lists[0] = uop_cache_misses
plot_lists[1] = icache_misses
plot_lists[2] = uop_cache_miss_rates
plot_name=['uopcachemiss','icachemiss','uopmissrate']
plot_y=['UC_MPKI','IC_MPKI','UC Miss Rate']

 
# Set position of bar on X axis
br1 = np.arange(len(plot_lists[0][0]))
br2 = [x + barWidth for x in br1]
br3 = [x + barWidth for x in br2]
br4 = [x + barWidth for x in br1]

for plot_item in range(3):
        # set width of bar
        barWidth = 0.25
        fig,ax = plt.subplots(figsize =(26, 8))

        # Make the plot
        plt.bar(br1, plot_lists[plot_item][0], color ='r', width = barWidth,
                edgecolor ='grey', label ="REPL_LRU")
        plt.bar(br2, plot_lists[plot_item][1], color ='g', width = barWidth,
                edgecolor ='grey', label ="REPL_OPT") #"REPL_SHADOW_IDEAL")
        plt.bar(br3, plot_lists[plot_item][2], color ='b', width = barWidth,
                edgecolor ='grey', label ="ORACLE_CACHE")
        
        # Adding Xticks
        plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
        plt.ylabel(plot_y[plot_item], fontweight ='bold', fontsize = 15)
        plt.xticks([r +0.5*barWidth for r in range(len((plot_lists[0][0])))],
                tests_name, fontsize = 15)

        
        for index, value in enumerate(plot_lists[plot_item][0]):
                # print(value)
                added = str(round(value,2))
                if plot_item==2:
                        added = str(round(value,2)) + "%"
                plt.text(br1[index]-0.2, value+(0.2-(1-plot_item)*0.15)*float(plot_lists[plot_item][0][0])*0.7,
                added, fontsize = 15)
        for index, value in enumerate(plot_lists[plot_item][1]):
                # print(value)
                added = str(round(value,2))
                if plot_item==2:
                        added = str(round(value,2)) + "%"
                plt.text(br2[index]-0.1, value+0.05*float(plot_lists[plot_item][1][0]),
                added, fontsize = 15)
        for index, value in enumerate(plot_lists[plot_item][2]):
                # print(value)
                added = str(round(value,2))
                if plot_item==2:
                        added = str(round(value,2)) + "%"
                plt.text(br3[index], value+0.2*float(plot_lists[plot_item][2][0]),
                added, fontsize = 15)

        # twin object for two different y-axis on the sample plot
        ax2=ax.twinx()
        # make a plot with different y-axis using second axis object
        ax2.plot(br4, IPC_lists[0], color ='y', marker='o', markersize=12,linestyle='dashed',linewidth=0,
                 label ="IPC") #width = barWidth, edgecolor ='grey',
        ax2.set_ylabel("IPC",color="y", fontweight ='bold', fontsize = 15)
        for index, value in enumerate(IPC_lists[0]):
                # print(value)
                ax2.text(br4[index]-0.17, value+0.05*float(IPC_lists[0][0]),
                str('{:.2g}'.format(value)), fontsize = 15)
        
        ax.legend(fontsize=15)
        plt.savefig("dc_bench_replOPT_"+plot_name[plot_item]+"_v"+sys.argv[1].split("_")[-2]+".pdf", format="pdf", bbox_inches="tight")
        plt.clf()
#17.6*float(plot_lists[plot_item][0][0])

