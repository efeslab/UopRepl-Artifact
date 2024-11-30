import numpy as np
import matplotlib.pyplot as plt
import sys

folder_name = sys.argv[1] #dc_benchv9
tests = ["cassandra",  "drupal",  "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"]

uop_cache_misses=[[],[]]
icache_misses=[[],[]]

uop_accumulate=[]
uop_cache_pws_inserted=[]
uop_cache_lines_inserted=[]
uop_cache_lines_used=[]
uop_cache_pw_insert_failed_cache_hit=[]
uop_cache_icache_switch_br_not_taken_resteered=[]
uop_cache_icache_switch_br_not_taken_correct_pred=[]
uop_cache_icache_switch_br_taken_resteered=[]
uop_cache_icache_switch_br_taken_correct_pred=[]
uop_cache_single_ratio=[]



for i in range(len(tests)):
        for typ in range(2):
                given_file = open("../test/uop_tests/"+sys.argv[1]+"/"+tests[i]+"lrurepl/memory.stat.0.out", 'r')
                # if typ==1 and i!=11:
                #         given_file = open("../test/uop_tests/"+sys.argv[1]+"/ideal"+tests[i]+"1/memory.stat.0.out", 'r')   
                # elif typ==1 and i==11:
                #         given_file = open("../test/uop_tests/"+sys.argv[1]+"/idealsecond"+tests[i]+"1/memory.stat.0.out", 'r')  
                lines = given_file.readlines()

                for line in lines:
                        if " Instructions:" in line:
                                inst_num = " ".join(line.strip().split()).split(" ")[-3]
                                # print(inst_num)
                        if "UOP_CACHE_MISS" in line:
                                tmp_miss = " ".join(line.strip().split()).split(" ")[1]
                                # print(tmp_miss)
                        if "ICACHE_MISS " in line:
                                icache_miss = " ".join(line.strip().split()).split(" ")[1]
                                # print(icache_miss)
                if typ==0:
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
                                if "UOP_CACHE_ALL_LINES" in line:
                                        uop_9 = " ".join(line.strip().split()).split(" ")[1]
                                if "UOP_CACHE_SINGLE_LINE" in line:
                                        uop_10 = " ".join(line.strip().split()).split(" ")[1]

                                
                uop_cache_misses[typ].append(float(tmp_miss)/float(inst_num)*1000)
                icache_misses[typ].append(float(icache_miss)/float(inst_num)*1000)

                given_file.close()
        uop_accumulate.append(float(uop_0)/float(inst_num))#*1000)
        uop_cache_pws_inserted.append(float(uop_1)/float(inst_num))#*1000)
        uop_cache_lines_inserted.append(float(uop_2)/float(inst_num))#*1000)
        uop_cache_lines_used.append(float(uop_3)/float(inst_num))#*1000)
        uop_cache_pw_insert_failed_cache_hit.append(float(uop_4)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_not_taken_resteered.append(float(uop_5)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_not_taken_correct_pred.append(float(uop_6)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_taken_resteered.append(float(uop_7)/float(inst_num))#*1000)
        uop_cache_icache_switch_br_taken_correct_pred.append(float(uop_8)/float(inst_num))#*1000)
        uop_cache_single_ratio.append(float(uop_10)/float(uop_9)*100)

print(uop_cache_misses)     
print(icache_misses) 
print(uop_cache_single_ratio)
# print(uop_accumulate)
# print(uop_cache_pws_inserted)
# print(uop_cache_lines_inserted)
# print(uop_cache_lines_used)
# print(uop_cache_pw_insert_failed_cache_hit)
# print(uop_cache_icache_switch_br_not_taken_resteered)
# print(uop_cache_icache_switch_br_not_taken_correct_pred)
# print(uop_cache_icache_switch_br_taken_resteered)
# print(uop_cache_icache_switch_br_taken_correct_pred)


plot_name=['uop_cache_single_ratio']
plot_y='percentage(%100)'

for plot_item in range(1):
        # set width of bar
        barWidth = 0.15
        fig,ax = plt.subplots(figsize =(30, 10))

        # Set position of bar on X axis
        br1 = np.arange(len(uop_cache_single_ratio))


        # Make the plot
        plt.bar(br1, uop_cache_single_ratio, color ='r', width = barWidth,
                edgecolor ='grey', label = plot_name[0])

        # Adding Xticks
        plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
        plt.ylabel(plot_y, fontweight ='bold', fontsize = 15)
        plt.xticks([r for r in range(len((uop_cache_single_ratio)))],
                tests, fontsize = 15)
        plt.ylim([80, 100])

        for index, value in enumerate(uop_cache_single_ratio):
                # print(value)
                plt.text(br1[index]-0.2, value+0.005*float(uop_cache_single_ratio[0]),
                str(round(value,3)), fontsize = 15)


        ax.legend(fontsize = 15)
        plt.savefig("dc_bench_uopcache_singleratio.pdf", format="pdf", bbox_inches="tight")
        plt.clf()

 
# set height of bar
BASELINE =        [ 0.63, 1.51, 1.17, 0.52, 0.77, 1.24, 1.10, 0.20, 1.43, 0.92, 1.14, 0.97, 1.31] 
ORACLE = [ 0.86, 2.46, 2.00, 0.64, 1.27, 1.92, 1.77, 1.55, 2.19, 2.72, 2.39, 2.09, 2.07] 
# BASELINE =  [0.15, 1.34, 1.35, 0.11, 0.27, 1.08, 0.94, 0.20, 1.33] 
# ORACLE = [0.15, 2.35, 1.82, 0.11, 0.28, 1.84, 1.69, 1.49, 2.08] 

# # set width of bar
# barWidth = 0.25
# fig = plt.subplots(figsize =(26, 8))

# # Set position of bar on X axis
# br1 = np.arange(len(BASELINE))
# br2 = [x + barWidth for x in br1]
 
# # Make the plot
# plt.bar(br1, BASELINE, color ='r', width = barWidth,
#         edgecolor ='grey', label ='BASELINE')
# plt.bar(br2, ORACLE, color ='g', width = barWidth,
#         edgecolor ='grey', label ='ORACLE')
 
# # Adding Xticks
# plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
# plt.ylabel('IPC', fontweight ='bold', fontsize = 15)
# plt.xticks([r + barWidth for r in range(len(BASELINE))],
#         ['cassandra', 'drupal', 'finagle-chirper', 'finagle-http', 'kafka', 'mediawiki', 'tomcat', 'verilator', 'wordpress'
#         , 'clang', 'mysql', 'postgres', 'python'], fontsize = 15)

# for index, value in enumerate(BASELINE):
#     plt.text(br1[index]-0.2, value+0.05,
#              str(value), fontsize = 15)
# for index, value in enumerate(ORACLE):
#     plt.text(br2[index]-0.1, value+0.05,
#              str(value), fontsize = 15)
 
# plt.legend()
# plt.savefig("dc_bench_IPC.pdf", format="pdf", bbox_inches="tight")
# plt.clf()

# plot_lists=[[],[]]
# plot_lists[0] = uop_cache_misses
# plot_lists[1] = icache_misses
# plot_name=['uopcachemiss','icachemiss']
# plot_y=['UC_MPKI','IC_MPKI']

 
# # Set position of bar on X axis
# br1 = np.arange(len(plot_lists[0][0]))
# br2 = [x + barWidth for x in br1]
# br3 = [x + 0.2*barWidth for x in br2]

# for plot_item in range(2):
#         # set width of bar
#         barWidth = 0.25
#         fig,ax = plt.subplots(figsize =(26, 8))

#         # Make the plot
#         plt.bar(br1, plot_lists[plot_item][0], color ='r', width = barWidth,
#                 edgecolor ='grey', label ="BASELINE")
#         plt.bar(br2, plot_lists[plot_item][1], color ='g', width = barWidth,
#                 edgecolor ='grey', label ="ORACLE")
        
#         # Adding Xticks
#         plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
#         plt.ylabel(plot_y[plot_item], fontweight ='bold', fontsize = 15)
#         plt.xticks([r + barWidth for r in range(len((plot_lists[0][0])))],
#                 tests, fontsize = 15)

#         for index, value in enumerate(plot_lists[plot_item][0]):
#                 # print(value)
#                 plt.text(br1[index]-0.2, value+0.05*float(plot_lists[plot_item][0][0]),
#                 str(round(value,2)), fontsize = 15)
#         for index, value in enumerate(plot_lists[plot_item][1]):
#                 # print(value)
#                 plt.text(br2[index]-0.1, value+0.05*float(plot_lists[plot_item][1][0]),
#                 str(round(value,2)), fontsize = 15)

#         # twin object for two different y-axis on the sample plot
#         ax2=ax.twinx()
#         # make a plot with different y-axis using second axis object
#         ax2.plot(br3, BASELINE, color ='y', marker='o', markersize=12,linestyle='dashed',linewidth=2,
#                  label ="IPC") #width = barWidth, edgecolor ='grey',
#         ax2.set_ylabel("IPC",color="y", fontweight ='bold', fontsize = 15)
#         for index, value in enumerate(BASELINE):
#                 # print(value)
#                 ax2.text(br3[index]-0.1, value+0.05*float(BASELINE[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)
        
#         ax.legend()
#         plt.savefig("dc_bench_"+plot_name[plot_item]+".pdf", format="pdf", bbox_inches="tight")
#         plt.clf()
# #17.6*float(plot_lists[plot_item][0][0])



# plot_name=['uop_accumulate','uop_cache_pws_inserted','uop_cache_lines_inserted','uop_cache_lines_used','uop_cache_pw_insert_failed_cache_hit']
# plot_y='num/instr'

# for plot_item in range(1):
#         # set width of bar
#         barWidth = 0.15
#         fig,ax = plt.subplots(figsize =(30, 10))

#         # Set position of bar on X axis
#         br1 = np.arange(len(uop_accumulate))
#         br2 = [x + barWidth for x in br1]
#         br3 = [x + barWidth for x in br2]
#         br4 = [x + barWidth for x in br3]
#         br5 = [x + barWidth for x in br4]

#         # Make the plot
#         plt.bar(br1, uop_accumulate, color ='r', width = barWidth,
#                 edgecolor ='grey', label = plot_name[0])
#         plt.bar(br2, uop_cache_pws_inserted, color ='g', width = barWidth,
#                 edgecolor ='grey', label = plot_name[1])
#         plt.bar(br3, uop_cache_lines_inserted, color ='b', width = barWidth,
#                 edgecolor ='grey', label = plot_name[2])
#         plt.bar(br4, uop_cache_lines_used, color ='m', width = barWidth,
#                 edgecolor ='grey', label = plot_name[3])
#         plt.bar(br5, uop_cache_pw_insert_failed_cache_hit, color ='c', width = barWidth,
#                 edgecolor ='grey', label = plot_name[4])

#         # Adding Xticks
#         plt.xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
#         plt.ylabel(plot_y, fontweight ='bold', fontsize = 15)
#         plt.xticks([r + 2*barWidth for r in range(len((plot_lists[0][0])))],
#                 tests, fontsize = 15)

#         for index, value in enumerate(uop_accumulate):
#                 # print(value)
#                 plt.text(br1[index]-0.2, value+0.05*float(uop_accumulate[0]),
#                 str(round(value,3)), fontsize = 15)
#         for index, value in enumerate(uop_cache_pws_inserted):
#                 # print(value)
#                 plt.text(br2[index]-0.1, value+0.05*float(uop_cache_pws_inserted[0]),
#                 str(round(value,3)), fontsize = 15)
#         for index, value in enumerate(uop_cache_lines_inserted):
#                 # print(value)
#                 plt.text(br3[index], value+0.2*float(uop_cache_lines_inserted[0]),
#                 str(round(value,3)), fontsize = 15)
#         for index, value in enumerate(uop_cache_lines_used):
#                 # print(value)
#                 plt.text(br4[index]-0.2, value+0.5*float(uop_cache_lines_used[0]),
#                 str(round(value,3)), fontsize = 15)
#         for index, value in enumerate(uop_cache_pw_insert_failed_cache_hit):
#                 # print(value)
#                 plt.text(br5[index]-0.1, value+0.05*float(uop_cache_pw_insert_failed_cache_hit[0]),
#                 str(round(value,3)), fontsize = 15)

#         # twin object for two different y-axis on the sample plot
#         ax2=ax.twinx()
#         # make a plot with different y-axis using second axis object
#         ax2.plot(br4, BASELINE, color ='y', marker='o', markersize=12,linestyle='dashed',linewidth=2,
#                  label ="IPC") #width = barWidth, edgecolor ='grey',
#         ax2.set_ylabel("IPC",color="y", fontweight ='bold', fontsize = 15)
#         for index, value in enumerate(BASELINE):
#                 # print(value)
#                 ax2.text(br4[index]-0.1, value+0.05*float(BASELINE[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)

#         ax.legend(fontsize = 15)
#         plt.savefig("dc_bench_uopcache_nums.pdf", format="pdf", bbox_inches="tight")
#         plt.clf()



# plot_name=['uop_cache_icache_switch_br_not_taken_resteered','uop_cache_icache_switch_br_not_taken_correct_pred','uop_cache_icache_switch_br_taken_resteered','uop_cache_icache_switch_br_taken_correct_pred']
# plot_y='num/instr'

# for plot_item in range(1):
#         # set width of bar
#         barWidth = 0.15
#         fig,ax = plt.subplots(figsize =(30, 8))

#         # Set position of bar on X axis
#         br1 = np.arange(len(uop_cache_icache_switch_br_not_taken_resteered))
#         br2 = [x + barWidth for x in br1]
#         br3 = [x + barWidth for x in br2]
#         # br4 = [x + barWidth for x in br3]

#         # Make the plot
#         ax.bar(br1, uop_cache_icache_switch_br_not_taken_resteered, color ='r', width = barWidth,
#                 edgecolor ='grey', label = plot_name[0])
#         # plt.bar(br2, uop_cache_icache_switch_br_not_taken_correct_pred, color ='g', width = barWidth,
#         #         edgecolor ='grey', label = plot_name[1])
#         ax.bar(br2, uop_cache_icache_switch_br_taken_resteered, color ='b', width = barWidth,
#                 edgecolor ='grey', label = plot_name[2])
#         ax.bar(br3, uop_cache_icache_switch_br_taken_correct_pred, color ='m', width = barWidth,
#                 edgecolor ='grey', label = plot_name[3])

#         # Adding Xticks
#         ax.set_xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
#         ax.set_ylabel(plot_y, fontweight ='bold', fontsize = 15)
#         plt.xticks([r + 2*barWidth for r in range(len((plot_lists[0][0])))],
#                 tests, fontsize = 15)

#         for index, value in enumerate(uop_cache_icache_switch_br_not_taken_resteered):
#                 # print(value)
#                 plt.text(br1[index]-0.2, value+0.05*float(uop_cache_icache_switch_br_not_taken_resteered[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)
#         # for index, value in enumerate(uop_cache_icache_switch_br_not_taken_correct_pred):
#         #         # print(value)
#         #         plt.text(br2[index]-0.1, value+0.05*float(uop_cache_icache_switch_br_not_taken_correct_pred[0]),
#         #         str('{:.2g}'.format(value)), fontsize = 15)
#         for index, value in enumerate(uop_cache_icache_switch_br_taken_resteered):
#                 # print(value)
#                 plt.text(br2[index], value+0.2*float(uop_cache_icache_switch_br_taken_resteered[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)
#         for index, value in enumerate(uop_cache_icache_switch_br_taken_correct_pred):
#                 # print(value)
#                 plt.text(br3[index]-0.2, value-0.2*float(uop_cache_icache_switch_br_taken_correct_pred[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)

#         # twin object for two different y-axis on the sample plot
#         ax2=ax.twinx()
#         # make a plot with different y-axis using second axis object
#         ax2.plot(br4, BASELINE, color ='y', marker='o', markersize=12,linestyle='dashed',linewidth=2,
#                  label ="IPC") #width = barWidth, edgecolor ='grey',
#         ax2.set_ylabel("IPC",color="y", fontweight ='bold', fontsize = 15)
#         for index, value in enumerate(BASELINE):
#                 # print(value)
#                 ax2.text(br4[index]-0.1, value+0.05*float(BASELINE[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)

#         ax.legend(fontsize = 15)
#         plt.savefig("dc_bench_uopcach_switches.pdf", format="pdf", bbox_inches="tight")
#         plt.clf()
        

#         barWidth = 0.15
#         fig,ax = plt.subplots(figsize =(30, 8))

#         # Set position of bar on X axis
#         br1 = np.arange(len(uop_cache_icache_switch_br_not_taken_correct_pred))
#         br2 = [x + 1.5*barWidth for x in br1]

#         # Make the plot
#         ax.bar(br1, uop_cache_icache_switch_br_not_taken_correct_pred, color ='g', width = barWidth,
#                 edgecolor ='grey', label = plot_name[1])
        

#         # Adding Xticks
#         ax.set_xlabel('Benchmarks', fontweight ='bold', fontsize = 15)
#         ax.set_ylabel(plot_y,color="g", fontweight ='bold', fontsize = 15)
#         plt.xticks([r +0.8*barWidth for r in range(len((plot_lists[0][0])))],
#                 tests, fontsize = 15)

#         # twin object for two different y-axis on the sample plot
#         ax2=ax.twinx()
#         # make a plot with different y-axis using second axis object
#         # ax2.plot(gapminder_us.year, gapminder_us["gdpPercap"],color="blue",marker="o")
#         ax2.plot(br2, BASELINE, color ='y', marker='o', markersize=12,linestyle='dashed',linewidth=2,
#                  label ="IPC") #width = barWidth, edgecolor ='grey',
#         ax2.set_ylabel("IPC",color="y", fontweight ='bold', fontsize = 15)


#         for index, value in enumerate(uop_cache_icache_switch_br_not_taken_correct_pred):
#                 # print(value)
#                 ax.text(br1[index]-0.2, value+0.05*float(uop_cache_icache_switch_br_not_taken_correct_pred[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)
#         for index, value in enumerate(BASELINE):
#                 # print(value)
#                 ax2.text(br2[index]-0.1, value+0.05*float(BASELINE[0]),
#                 str('{:.2g}'.format(value)), fontsize = 15)



#         ax.legend(fontsize = 15)
#         plt.savefig("dc_bench_uopcach_switches_2.pdf", format="pdf", bbox_inches="tight")
#         plt.clf()