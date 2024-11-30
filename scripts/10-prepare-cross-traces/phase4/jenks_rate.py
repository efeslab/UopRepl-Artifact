# from sklearn.cluster import KMeans
# from sklearn import metrics
from scipy.spatial.distance import cdist
import numpy as np
import matplotlib.pyplot as plt
# from threadpoolctl import threadpool_limits
import math
import sys

# Creating the data
tests = ["cassandra",  "drupal", "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "python", "finagle-chirper"] #, "postgres"
        # ['postgres']
        # ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        # "mediawiki",  "tomcat",  "verilator",  "wordpress",
        # "clang", "mysql", "python"] #, "postgres"

inputFile = sys.argv[1]
outputFile = sys.argv[2]
num=2**(int(sys.argv[3]))
figOutDir=sys.argv[4]
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

dict_test = dict()
given_file = open(inputFile, 'r')
print(inputFile)
lines = given_file.readlines()

for line in lines:
    addr = int(" ".join(line.strip().split()).split(" ")[0])
    count = float(" ".join(line.strip().split()).split(" ")[1])
    
    dict_test[addr] = count
    # dict_test[addr] = math.log10(count)


given_file.close()
dict_group.append(dict_test)


x01 = list(dict_test.items())
x01.sort(key=lambda y: y[1])
x02 = [tup[1] for tup in x01]
x1 = np.array(x02)
print("len of array: " + str(len(x1)))

# Visualizing the data
plt.plot()
# plt.xlim([0, 10])
# plt.ylim([0, 10])
plt.title('Dataset')
# plt.scatter(x1, x2)
plt.plot(x1)
plt.savefig(figOutDir+"/jenks_"+str(num)+".pdf", format="pdf", bbox_inches="tight")
# plt.savefig("folder_jenks/jenks_orilog10_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()
# plt.show()

# x = np.array([3, 1, 1, 2, 1, 6, 6, 6, 5, 6, 7, 8, 9, 8, 9, 9, 8, 5, 4, 5, 6, 5, 8, 6, 7, 6, 7, 1, 2, 1, 2, 3, 2, 3])
#np.random.random(30)
# print(x)
print("len of array: " + str(len(x1)))
print("Group number: " + str(num))
breaks = jenkspy.jenks_breaks(x1, num) #2**(int(sys.argv[2]))) #4) #8) #jenks
# breaks = get_jenks_breaks(x1, 8)
print(breaks)

# f = open(size_file_name[0]+"/"+tests[i]+"3bitcacheaccess/cacheaccess_stream_w"+str(2**(int(sys.argv[2])))+".out", "w")
f = open(outputFile, "w")
for iy in range(len(x1)):
    # f.write(str(x01[iy][0])+" "+str(x1[iy]))
    f.write(str(x01[iy][0])+" "+str(return_class(x1[iy],breaks))+"\n")
    
f.close()
fig, ax1 = plt.subplots()
for line in breaks:
    plt.plot([line for _ in range(len(x1))], 'k--')
plt.plot(x1)
plt.grid(True)
# ax1.set_yscale('log')
# print(i, tests[i])
plt.savefig(figOutDir+"/jenks_"+str(num)+"_linedivide.pdf", format="pdf", bbox_inches="tight")
# plt.savefig("folder_jenks/jenks_log10_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
plt.clf()

