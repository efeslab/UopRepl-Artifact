from sklearn.cluster import KMeans
from sklearn import metrics
from scipy.spatial.distance import cdist
import numpy as np
import matplotlib.pyplot as plt
from threadpoolctl import threadpool_limits
import math
import sys

# Creating the data
tests = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"] 
        # ['postgres']
        # ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        # "mediawiki",  "tomcat",  "verilator",  "wordpress",
        # "clang", "mysql", "python"] #, "postgres"
size_file_name = ["/mnt/storage/shuwend/weekly_work/2022-10-23/groups_scarab/scarab_hlitz_allPW/test/uop_tests/"+sys.argv[1]] 
# ["/mnt/storage/shuwend/weekly_work/2022-10-16/scarab_hlitz_1011/test/uop_tests/"+sys.argv[1]] 
# ["/mnt/storage/shuwend/weekly_work/2022-9-25/scarab_hlitz_accesscount/test/uop_tests/dc_benchv77_accesscount_0920_v41"] 


def get_jenks_breaks(data_list, number_class):
    data_list.sort()
    mat1 = []
    for i in range(len(data_list) + 1):
        temp = []
        for j in range(number_class + 1):
            temp.append(0)
        mat1.append(temp)
    mat2 = []
    for i in range(len(data_list) + 1):
        temp = []
        for j in range(number_class + 1):
            temp.append(0)
        mat2.append(temp)
    for i in range(1, number_class + 1):
        mat1[1][i] = 1
        mat2[1][i] = 0
        for j in range(2, len(data_list) + 1):
            mat2[j][i] = float('inf')
    v = 0.0
    for l in range(2, len(data_list) + 1):
        s1 = 0.0
        s2 = 0.0
        w = 0.0
        for m in range(1, l + 1):
            i3 = l - m + 1
            val = float(data_list[i3 - 1])
            s2 += val * val
            s1 += val
            w += 1
            v = s2 - (s1 * s1) / w
            i4 = i3 - 1
            if i4 != 0:
                for j in range(2, number_class + 1):
                    if mat2[l][j] >= (v + mat2[i4][j - 1]):
                        mat1[l][j] = i3
                        mat2[l][j] = v + mat2[i4][j - 1]
        mat1[l][1] = 1
        mat2[l][1] = v
    k = len(data_list)
    kclass = []
    for i in range(number_class + 1):
        kclass.append(min(data_list))
    kclass[number_class] = float(data_list[len(data_list) - 1])
    count_num = number_class
    while count_num >= 2:  # print "rank = " + str(mat1[k][count_num])
        idx = int((mat1[k][count_num]) - 2)
        # print "val = " + str(data_list[idx])
        kclass[count_num - 1] = data_list[idx]
        k = int((mat1[k][count_num] - 1))
        count_num -= 1
    return kclass


def return_class(raw_data,break_group):
    for i in range(1, len(breaks)):
        if raw_data <= breaks[i]:
            return i
    return len(breaks) - 1


dict_group=[]
# distortions_group=[]
# inertia_group=[]
length = len(tests)#:1#len(tests):

# num_typs = 11 #8 #7 #12

import jenkspy
# from jenks import jenks

# type 0 LRU; type 1 OPT
for i in range(length): #len(tests)):
    dict_test = dict()
    given_file = open(size_file_name[0]+"/"+tests[i]+"3bitcacheaccess/cacheaccess_stream.out", 'r')
    print(size_file_name[0]+"/"+tests[i]+"3bitcacheaccess/cacheaccess_stream.out")
    if (sys.argv[3])=="whole":
      given_file = open(size_file_name[0]+"/"+tests[i]+"wholecacheaccess/cacheaccess_stream.out", 'r')
      print(size_file_name[0]+"/"+tests[i]+"wholecacheaccess/cacheaccess_stream.out", 'r')
    lines = given_file.readlines()

    for line in lines:
        addr = int(" ".join(line.strip().split()).split(" ")[0])
        addr_end = int(" ".join(line.strip().split()).split(" ")[1])
        count = int(" ".join(line.strip().split()).split(" ")[2])
        
        dict_test[tuple([addr, addr_end])] = count
        # dict_test[addr] = math.log10(count)

    
    given_file.close()
    dict_group.append(dict_test)

    print("len of dict: " + str(len(dict_test)))

    # x01 = list(dict_test.items())
    # x01.sort(key=lambda y: y[0][0])
    # for ii in range(3): #(len(x1)):
    #   print(x01[ii][0], x01[ii][1])
    
    # start_pointer=0
    # end_pointer=1
    # x02 = list()
    # while (start_pointer!=len(x01)):
    #   if (x01[start_pointer][0][0]<x01[end_pointer][0][1])
    
    # results = []
    # intervals = sorted(dict_test.items(), key=lambda x: (x[0][0], x[0][1]))
    # # intervals = sorted(intervals_tmp, key=lambda x: x[0][1])
    # for ii in range(3): #(len(x1)):
    #   print(intervals[ii][0], intervals[ii][1])
    # a = intervals[0]
    # for b in intervals[1:]:
    #   if a[0][1] >= b[0][0] and a[1] == b[1]:
    #     # print(a, b)
    #     a = ((a[0][0], max(a[0][1], b[0][1])), a[1])
    #   else:
    #     results.append(a)
    #     a = b
    # results.append(a)

    # print("len of res: " + str(len(results)))

    # results_pure = []
    # intervals_pure = sorted(dict_test.items(), key=lambda x: (x[0][0], x[0][1]))
    # # intervals = sorted(intervals_tmp, key=lambda x: x[0][1])
    # for ii in range(3): #(len(x1)):
    #   print(intervals_pure[ii][0], intervals_pure[ii][1])
    # a = intervals_pure[0]
    # for b in intervals_pure[1:]:
    #   # print("before", a, b)
    #   if a[0][1] >= b[0][0]: # and a[1] == b[1]:
    #     # print(a, b)
    #     a = ((a[0][0], max(a[0][1], b[0][1])), a[1])
    #   else:
    #     results_pure.append(a)
    #     a = b
    # results_pure.append(a)

    # print("len of res_pure: " + str(len(results_pure)))

    # for ii in range(3): #(len(x1)):
    #   print(results[ii][0], results[ii][1])

    # results_detach = dict()
    # # a = results[0]
    # addr = results[0][0][0]
    # # while addr <= results[-1][0][1]:

    # # for addr in range(results[0][0][0], results[-1][0][1]):
    # for iii in results_pure:
    #   for addr in range(iii[0][0], iii[0][1]):
    #     value = 0
    #     for addr_range in results:
    #       if addr >= addr_range[0][0] and addr <= addr_range[0][1]: # and addr_range[1] > value:
    #         value += addr_range[1] 
    #     if (value > 0):
    #       results_detach[addr] = value

    # print("len of res_detach: " + str(len(results_detach)))

    # for ii in range(3): #(len(x1)):
    #   print(results[ii][0], results[ii][1])



    results = dict()
    intervals = sorted(dict_test.items(), key=lambda x: (x[0][0], x[0][1]))
    for iii in intervals:
      for addr in range(iii[0][0], iii[0][1]+1):
        if addr in results.keys():
          results[addr] += iii[1]
        else:
          results[addr] = iii[1]

    print("len of res_detach: " + str(len(results)))




    # for key1 in list(dict_test):
    #   for key2 in list(dict_test):
    #     if (key1!=key2 and key1[1]==key2[0] and dict_test[key1]==dict_test[key2]):
    #       tmp_value = dict_test[key1]
    #       print(key1, dict_test[key1], key2, dict_test[key2])
    #       del dict_test[key1]
    #       del dict_test[key2]
    #       dict_test[tuple([key1[0], key2[1]])] = tmp_value
    # print("len of dict: " + str(len(dict_test)))



    x01 = list(results.items()) #results_detach
    x01.sort(key=lambda y: y[1])
    x02 = [tup[1] for tup in x01]
    x1 = np.array(x02)
    x03 = [tup[0] for tup in x01]
    x13 = np.array(x03)
    print("len of array: " + str(len(x1)))

    # Visualizing the data
    plt.plot()
    # plt.xlim([0, 10])
    # plt.ylim([0, 10])
    plt.title('Dataset')
    # plt.scatter(x1, x2)
    plt.plot(x1)
    print(sys.argv[1], sys.argv[2], sys.argv[3])
    plt.savefig(sys.argv[1]+"/jenks"+str(2**(int(sys.argv[2])))+tests[i]+".pdf", format="pdf", bbox_inches="tight")
    # plt.savefig("folder_jenks/jenks_orilog10_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
    plt.clf()
    # plt.show()

    if (sys.argv[3])=="whole":
      print("print to: " + size_file_name[0]+"/"+tests[i]+"wholecacheaccess/cacheaccess_stream_w.out")
      f = open(size_file_name[0]+"/"+tests[i]+"wholecacheaccess/cacheaccess_stream_w.out", "w")
      for iy in range(len(x1)):
        # f.write(str(x01[iy][0])+" "+str(x1[iy]))
        f.write(str(x01[iy][0])+" "+str(x01[iy][1])+"\n")
        
      f.close()

    else:

      # x = np.array([3, 1, 1, 2, 1, 6, 6, 6, 5, 6, 7, 8, 9, 8, 9, 9, 8, 5, 4, 5, 6, 5, 8, 6, 7, 6, 7, 1, 2, 1, 2, 3, 2, 3])
      #np.random.random(30)
      # print(x)
      print("len of array: " + str(len(x1)))
      breaks = jenkspy.jenks_breaks(x1, 2**(int(sys.argv[2]))) #4) #8) #jenks
      # breaks = get_jenks_breaks(x1, 8)
      print(breaks)

      f = open(size_file_name[0]+"/"+tests[i]+"3bitcacheaccess/cacheaccess_stream_w"+str(2**(int(sys.argv[2])))+".out", "w")
      for iy in range(len(x1)):
        # f.write(str(x01[iy][0])+" "+str(x1[iy]))
        f.write(str(x01[iy][0])+" "+str(return_class(x1[iy],breaks))+"\n")
        
      f.close()
      fig, ax1 = plt.subplots()
      for line in breaks:
        plt.plot([line for _ in range(len(x1))], 'k--')
      plt.plot(x1)
      plt.grid(True)
      ax1.set_yscale('log')
      print(i, tests[i])
      plt.savefig(sys.argv[1]+"/jenks"+str(2**(int(sys.argv[2])))+"linedivide_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
      # plt.savefig("folder_jenks/jenks_log10_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
      plt.clf()

