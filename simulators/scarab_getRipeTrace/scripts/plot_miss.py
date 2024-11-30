import numpy as np
import matplotlib.pyplot as plt
import sys

folder_name = "dc_benchv1_pmc" #sys.argv[1] #dc_benchv9
tests = ["tomcat"]
    # ["cassandra",  "drupal",  "finagle-chirper",  "finagle-http",  "kafka",
    #     "mediawiki",  "tomcat",  "verilator",  "wordpress",
    #     "clang", "mysql", "postgres", "python"]
setnum=[]
missesnum=[]
countnum=[]

for i in range(len(tests)):
        for typ in range(1):
                given_file = open(folder_name + "/"+tests[i]+"wholecacheaccess/log1.txt", 'r')
                lines = given_file.readlines()

                for line in lines:
                  if "misses" in line:
                    # print(line)
                    setn = int(line.strip().split(" ")[3])
                    missn = int(line.strip().split(" ")[6])
                    countn = int(line.strip().split(" ")[9])
                    
                    setnum.append(setn)
                    missesnum.append(missn)
                    countnum.append(countn)

                given_file.close()

print(setnum[0:100])


plot_name=['uop_cache_single_ratio']
plot_y='percentage(%100)'

start=0 #38000
end=len(setnum) #39000
for plot_item in range(1):
        # set width of bar
        barWidth = 0.15
        fig,ax = plt.subplots(figsize =(50, 10))
        # plt.plot(setnum[start:end], label ='Set Num') #'o-', 
        plt.plot(missesnum[start:end], label ='Miss Num') #[47000:53000] #[50000:50500]
        plt.plot(countnum[start:end], label ='Count Num', alpha=0.5)
        plt.legend(fontsize=15)
        plt.xticks(fontsize = 15)
        plt.savefig("../../scripts/misscheckset_lru.pdf", format="pdf", bbox_inches="tight")
        plt.clf()

 
