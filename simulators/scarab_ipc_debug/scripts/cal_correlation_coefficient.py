import numpy as np
import matplotlib.pyplot as plt
import sys
from collections import Counter
from scipy.stats import pearsonr   

# folder_name = sys.argv[1] #dc_benchv15
tests = [sys.argv[2]]#["verilator"] #["drupal", "kafka", "verilator"]
# ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
#         "mediawiki",  "tomcat",  "verilator",  "wordpress",
#         "clang", "mysql", "postgres", "python"] #,  "drupal"
tests_name = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"]
tests_name_speedup = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python", "Avg", "Avg w/o verilator"]
        #tests

for i in tests:
        print(i, end='\t')
print()

size_file_name = ["dc_benchv65","dc_benchv66_calculatecount_0913_v32"]
uop_cache_misses=[[],[],[],[],[],[],[],[],[],[],[],[],[],[],[]]
uop_cache_miss_rates=[[],[],[],[],[],[],[],[],[],[],[],[],[],[],[]]
icache_misses=[[],[],[],[],[],[],[],[],[],[],[],[],[],[],[]]
IPC_lists=[[],[],[],[],[],[],[],[],[],[],[],[],[],[],[]]



num_typs = 2 #5

exe_count=[]

# type 0 LRU; type 1 OPT
for i in range(len(tests)):
  for typ in range(num_typs):
    if typ==0: # cache_access
      given_file = open("../test/uop_tests/"+size_file_name[1]+"/"+tests[i]+sys.argv[1]+"/execution_count_stream.out", 'r') 
      # given_file = open("../test/uop_tests/"+size_file_name[0]+"/"+tests[i]+sys.argv[1]+"/execution_count_stream.out", 'r')    
    elif typ==1: # OPT
      given_file = open("../test/uop_tests/"+size_file_name[1]+"/"+tests[i]+"opt/opt_stream.out", 'r')
      # given_file = open("../test/uop_tests/"+size_file_name[0]+"/"+tests[i]+"optlrurepl1/opt_stream.out", 'r')
    lines = given_file.readlines()

    if typ==0: # cacheaccess
      tmp_arr = []
      for line in lines:
        addr_num = " ".join(line.strip().split()).split(" ")[0]
        # print(addr_num)
        tmp_arr.append(addr_num)
      answer = Counter(tmp_arr)
      a2=answer.most_common() #sorted(answer.items())
      exe_count.append(answer) #a2)
      print(a2[0:3])
    elif typ==1: # OPT
      tmp_hit = []
      tmp_count = []
      for line in lines:
        addr_num = " ".join(line.strip().split()).split(" ")[0]
        hit = int(" ".join(line.strip().split()).split(" ")[1])
        tmp_count.append(addr_num)
        if (hit==1):
          tmp_hit.append(addr_num)
      hit_arr = Counter(tmp_hit) #(Counter(tmp_hit)).most_common() # sorted((Counter(tmp_hit)).items())
      count_arr = Counter(tmp_count) #(Counter(tmp_count)).most_common() # sorted((Counter(tmp_count)).items())
      print(hit_arr.most_common(3))
      print(count_arr.most_common(3))
      ratio_arr = Counter()
      for i in count_arr.keys():
        # print(i)
        found=True
        try:
          hit_arr[i]
        except KeyError:
          found=False #pass #
          print("Key wrong")
        if found:
          ratio_arr = ratio_arr + Counter({i:float(hit_arr[i])/count_arr[i]})
          # ratio_arr.append((i,float(hit_arr[i])/count_arr[i]))
        else:
          ratio_arr = ratio_arr + Counter({i:0})
          # ratio_arr.append((i,0))
        # print(ratio_arr)
      ratio_arr_ordered = ratio_arr.most_common()
      print(ratio_arr_ordered[0:3])
      exe_count.append(ratio_arr) #ratio_arr_ordered)
      
    # elif typ==1:
    #   tmp_hit = []
    #   tmp_count = []
    #   for line in lines:
    #     addr_num = " ".join(line.strip().split()).split(" ")[0]
    #     hit = int(" ".join(line.strip().split()).split(" ")[1])
    #     tmp_count.append(addr_num)
    #     if (hit==1):
    #       tmp_hit.append(addr_num)
    #   hit_arr = (Counter(tmp_hit)).most_common() # sorted((Counter(tmp_hit)).items())
    #   count_arr = (Counter(tmp_count)).most_common() # sorted((Counter(tmp_count)).items())
    #   print(hit_arr[0:3])
    #   print(count_arr[0:3])
    #   ratio_arr = []
    #   for i in range(len(count_arr)):
    #     found=False
    #     index=0
    #     for j in range(len(hit_arr)):
    #       if hit_arr[j][0]==count_arr[i][0]:
    #         found=True
    #         index=j
    #         break 
    #     if found:
    #       ratio_arr.append((count_arr[i][0],float(hit_arr[j][1])/count_arr[i][1]))
    #     else:
    #       ratio_arr.append((count_arr[i][0],0))
    #   exe_count.append(ratio_arr)
    #   print(ratio_arr[0:3])

corr_arr = [[],[]]
corr_counter=[Counter(),Counter()]
for i in answer.keys():
  # print(i)
  found=True
  try:
    ratio_arr[i]
  except KeyError:
    found=False #pass #
    print("Key wrong")
  if found:
    corr_counter[0] = corr_counter[0] + Counter({i:answer[i]}) #append(answer[i])
    corr_counter[1] = corr_counter[1] + Counter({i:ratio_arr[i]}) #.append(ratio_arr[i])
corr_counter[1] = corr_counter[1].most_common() # OPT ordered
n = 1
for i in corr_counter[1]:
  corr_arr[1].append(i[1])
  corr_arr[0].append(corr_counter[0][i[0]])

  #   ratio_arr = ratio_arr + Counter({i:float(hit_arr[i])/count_arr[i]})
  #   # ratio_arr.append((i,float(hit_arr[i])/count_arr[i]))
  # else:
  #   ratio_arr = ratio_arr + Counter({i:0})
# n = 1 # N. . .
# corr_arr[0]=[x[n] for x in exe_count[0]]
# corr_arr[1]=[x[n] for x in exe_count[1]]
# print(len(corr_arr[0]))
# print(len(corr_arr[1]))
# # for i in range(len(exe_count[0])):
# #   for j in range(len(exe_count[1])):
# #     if (exe_count[0][i][0]==exe_count[1][j][0]):
# #       corr_arr[0].append(exe_count[0][i][1])
# #       corr_arr[1].append(exe_count[1][j][1])
# # print(pearsonr(corr_arr[0],corr_arr[1]))
# minlen=min(len(corr_arr[0]), len(corr_arr[1]))

maxvalue=corr_arr[0][0]
for i in range(len(corr_arr[0])):
  corr_arr[0][i] = float(corr_arr[0][i])/maxvalue
print(pearsonr(corr_arr[0],corr_arr[1]))

# print(corr_arr)



fig, ax1 = plt.subplots(figsize =(10, 8))
ax2 = ax1.twinx()

ax1.plot(corr_arr[0], color ='r', label ='CACHE_ACCESS_COUNT')
ax2.plot(corr_arr[1], color ='g', label ='REPL_OPT')



# Adding Xticks
ax1.set_xlabel('All unique execution addresses', fontweight ='bold', fontsize = 15)
ax1.set_ylabel('Log Ratio', fontweight ='bold', fontsize = 15)
ax1.set_yscale('log')
ax2.set_ylabel('Ratio', fontweight ='bold', fontsize = 15)
# plt.xticks([r + 4.5*barWidth for r in range(len(IPC_lists[0]))],
#         tests_name, fontsize = 15)


 
ax1.legend(loc='upper left', fontsize = 15)
ax2.legend(loc='upper right', fontsize = 15)
plt.savefig("corr_graph/dc_bench_corr_v"+sys.argv[1]+"_"+tests[0]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()


        


                                
 