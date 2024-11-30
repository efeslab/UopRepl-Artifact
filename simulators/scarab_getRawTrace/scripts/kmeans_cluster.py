# import numpy as np  
# import matplotlib.pyplot as plt  
# import colorsys  
# import sys  

# # Creating the data
# tests = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
#         "mediawiki",  "tomcat",  "verilator",  "wordpress",
#         "clang", "mysql", "postgres", "python"]
# size_file_name = ["/mnt/storage/shuwend/code_library/scarab_hlitz/test/uop_tests/dc_benchv77_accesscount_0920_v41"] 
# # ["/mnt/storage/shuwend/weekly_work/2022-9-25/scarab_hlitz_accesscount/test/uop_tests/dc_benchv77_accesscount_0920_v41"] 


# dict_group=[]
# # distortions_group=[]
# # inertia_group=[]
# length = 1: #len(tests)#:1#len(tests):


# # type 0 LRU; type 1 OPT
# for i in range(length): #len(tests)):
#     dict_test = dict()
#     given_file = open(size_file_name[0]+"/"+tests[i]+"recordopt/cacheaccess_stream_0.out", 'r')
#     print(size_file_name[0]+"/"+tests[i]+"recordopt/cacheaccess_stream_0.out")
#     lines = given_file.readlines()

#     for line in lines:
#         addr = int(" ".join(line.strip().split()).split(" ")[0])
#         count = int(" ".join(line.strip().split()).split(" ")[1])
        
#         # dict_test[addr] = count
#         dict_test[addr] = math.log10(count)

    
#     given_file.close()
#     dict_group.append(dict_test)


#     x01 = list(dict_test.items())
#     x01.sort(key=lambda y: y[1])
#     x02 = [tup[1] for tup in x01]
#     x1 = np.array(x02)
#     print("len of array: " + str(len(x1)))



#     K = 3   # no. of centroids for computing  
#     nmbrClusters = 3 # actual no. of clusters that are to be generated  
#     pntsPerCluster = 40  # no. of points per actual cluster  
#     xCntrBounds = (-2, 2) # the limits within which the to actual cluster centers are to be placed  
#     # Randomly placing the cluster centers within the area of xCntrBounds.  
#     centers = np.random.random_sample((nmbrClusters,))  
#     centers = centers * (xCntrBounds[1] - xCntrBounds[0]) + xCntrBounds[0]  
#     # Initializing the array of data pts.  
#     points = np.zeros((nmbrClusters * pntsPerCluster,))  
#     # Normally distributing the pntsPerCluster points around the each center.  
#     stDev = 0.15  
#     for i in range(nmbrClusters):  
#         points[i*pntsPerCluster:(i+1)*pntsPerCluster] = (  
#             stDev * np.random.randn(pntsPerCluster) + centers[i])  
#     # Selecting K points randomly as initial centroid locations.  
#     centroids = np.zeros((K,))  
#     indices = []  
#     while len(indices) < K:  
#         index = np.random.randint(0, nmbrClusters * pntsPerCluster)  
#         if not index in indices:  
#             indices.append(index)  
#     centroids = points[indices]  
#     # Assigning each point to its closest centroid and then Storing this in the classifications,  
#     # where every single element must be an int ranging from 0 to K-1.  
#     classifications = np.zeros((points.shape[0],), dtype=np.int)  
#     def assignPntsToCentroids():  
#         for i in range(points.shape[0]):  
#             smallestDist = 0  
#             for k in range(K):  
#                 distance = abs(points[i] - centroids[k])  
#                 if k == 0:  
#                     smallestDist = distance  
#                     classifications[i] = k  
#                 elif distance < smallestDist:  
#                     smallestDist = distance  
#                     classifications[i] = k  
#     assignPntsToCentroids()  
#     # Defining a func for recalculatinge the cluster's centroid   
#     def reCalcCentroids():  
#         for k in range(K):  
#             if sum(classifications == k) > 0:  
#                 centroids[k] = sum(points[classifications == k]) / sum(classifications == k)  
#     # Using HSV color scheme, generating a different color for each K cluster.  
#     # Simultaneously, initializing the matplotlib line objects for every centroid and their cluster.  
#     hues = np.linspace(0, 1, K+1)[:-1]  
#     fig, ax = plt.subplots()  
#     clusterPntsList = []  
#     centroidPntsList = []  
#     for k in range(K):  
#         clusterColor = tuple(colorsys.hsv_to_rgb(hues[k], 0.8, 0.8))  
#         clusterLineObj, = ax.plot([], [], ls='None', marker='x', color=clusterColor)  
#         clusterPntsList.append(clusterLineObj)  
#         centroidLineObj, = ax.plot([], [], ls='None', marker='o',  
#             markeredgecolor='k', color=clusterColor)  
#         centroidPntsList.append(centroidLineObj)  
#     iterText = ax.annotate('', xy=(0.01, 0.01), xycoords='axes fraction')  
#     # Defining a func for updating the plot.  
#     def updatePlot(iteration):  
#         for k in range(K):  
#             xDataNew = points[classifications == k]  
#             clusterPntsList[k].set_data(xDataNew, np.zeros((len(xDataNew),)))  
#             centroidPntsList[k].set_data(centroids[k], 0)  
#         iterText.set_text('i = {:d}'.format(iteration))  
#         plt.savefig('folder_kmeans/{:d}.png'.format(iteration))  
#         plt.pause(0.5)  
#     dataRange = np.amax(points) - np.amin(points)  
#     ax.set_xlim(np.amin(points) - 0.05*dataRange, np.amax(points) + 0.05*dataRange)  
#     ax.set_ylim(-1, 1)  
#     iteration = 0  
#     updatePlot(iteration)  
#     plt.ion()  
#     # plt.show()  
#     plt.savefig("folder_kmeans/kmeans_try.pdf", format="pdf", bbox_inches="tight")
#     plt.clf()

#     # Executing and animating the algorithm using a while loop. Which isn't the best considered way for  
#     # animating a matplotlib plot rather the matplotlib animation module should be taken into consideration,  
#     # but we are using while loop here for making it easy to understand.  
#     last_Centroids = centroids + 1  
#     while not np.array_equal(centroids, last_Centroids):  
#         last_Centroids = np.copy(centroids)  
#         reCalcCentroids()  
#         assignPntsToCentroids()  
#         iteration += 1  
#         updatePlot(iteration)  
#     pythonMajorVersion = sys.version_info[0]  
#     if pythonMajorVersion < 3:  
#         raw_input("Press Enter to continue.")  
#     else:  
#         input("Press Enter to continue.")  









# # Method 1






from sklearn.cluster import KMeans
from sklearn import metrics
from scipy.spatial.distance import cdist
import numpy as np
import matplotlib.pyplot as plt
from threadpoolctl import threadpool_limits
import colorsys
import sys
import matplotlib.colors as mcolors

# Creating the data
tests = ["cassandra",  "drupal", "finagle-chirper",  "finagle-http",  "kafka",
        "mediawiki",  "tomcat",  "verilator",  "wordpress",
        "clang", "mysql", "postgres", "python"]
size_file_name = ["/mnt/storage/shuwend/code_library/scarab_hlitz/test/uop_tests/dc_benchv77_accesscount_0920_v41"] 
# ["/mnt/storage/shuwend/weekly_work/2022-9-25/scarab_hlitz_accesscount/test/uop_tests/dc_benchv77_accesscount_0920_v41"] 

dict_group=[]
distortions_group=[]
inertia_group=[]
length = len(tests)#:1#len(tests):

num_typs = 11 #8 #7 #12

# with threadpool_limits(limits=1, user_api='blas'):

def change_to_serial(x1):
    label = 0
    x2 = []
    pre_num = x1[0]
    for i in (x1):
        if i!=pre_num:
            label+=1
        x2.append(label)
        pre_num = i
    return(x2)

# type 0 LRU; type 1 OPT
for i in range(length): #len(tests)):
    dict_test = dict()
    given_file = open(size_file_name[0]+"/"+tests[i]+"recordopt/cacheaccess_stream.out", 'r')
    lines = given_file.readlines()

    for line in lines:
        addr = int(" ".join(line.strip().split()).split(" ")[0])
        count = int(" ".join(line.strip().split()).split(" ")[1])
        
        dict_test[addr] = count

    given_file.close()
    dict_group.append(dict_test)


    x01 = list(dict_test.items())
    # x01.sort(key=lambda y: y[1])
    x02 = [tup[1] for tup in x01]
    x1 = np.array(x02)

    print("len of array: " + str(len(x1)))
    #np.array([3, 1, 1, 2, 1, 6, 6, 6, 5, 6, 7, 8, 9, 8, 9, 9, 8])
    # x2 = np.array([5, 4, 5, 6, 5, 8, 6, 7, 6, 7, 1, 2, 1, 2, 3, 2, 3])
    # print(list(zip(x1,x2)))
    X = x1.reshape(-1,1)
    #np.array(list(zip(x1, x2))).reshape(len(x1), 2)
    # print(X)

    # Visualizing the data
    plt.plot()
    # plt.xlim([0, 10])
    # plt.ylim([0, 10])
    plt.title('Dataset')
    # plt.scatter(x1, x2)
    plt.plot(x1)
    plt.savefig("folder_kmeans/kmeans1_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
    plt.clf()
    # plt.show()



    distortions = []
    inertias = []
    mapping1 = {}
    mapping2 = {}
    K = [8] #range(1, 10)
    xaxis = range(1, len(x1)+1)
    # print(X)
    
    for k in K:
        # Building and fitting the model
        kmeanModel = KMeans(n_clusters=k).fit(X)
        kmeanModel.fit(X)
    
        distortions.append(sum(np.min(cdist(X, kmeanModel.cluster_centers_,
                                            'euclidean'), axis=1)) / X.shape[0])
        inertias.append(kmeanModel.inertia_)
    
        mapping1[k] = sum(np.min(cdist(X, kmeanModel.cluster_centers_,
                                    'euclidean'), axis=1)) / X.shape[0]
        mapping2[k] = kmeanModel.inertia_
        
        print(len(kmeanModel.labels_))
        colors = kmeanModel.labels_
        # print(colors)
        # print(change_to_serial(colors))

    for key, val in mapping1.items():
        print(f'{key} : {val}')

    fig, ax1 = plt.subplots()
    plt.plot()
    # plt.xlim([0, 10])
    # plt.ylim([0, 10])
    plt.title('Dataset')
    # print(mcolors.CSS4_COLORS)
    color_values = []
    for c in colors:
        color_values.append(list(mcolors.CSS4_COLORS.values())[c])
    plt.scatter(xaxis, x1, color=color_values)
    # plt.plot(x1)
    ax1.set_yscale('log')
    plt.savefig("folder_kmeans/kmeans1cluster_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
    plt.clf()

    print("folder_kmeans/kmeans1cluster_"+tests[i]+".pdf")

    f = open(size_file_name[0]+"/"+tests[i]+"recordopt/cacheaccess_stream_wkmeans.out", "w")
    print(size_file_name[0]+"/"+tests[i]+"recordopt/cacheaccess_stream_wkmeans.out")
    # f = open("folder_kmeans/cacheaccess_stream_wkmeans.out", "w")
    x_change = change_to_serial(colors)
    for iy in range(len(x1)):
      # f.write(str(x01[iy][0])+" "+str(x1[iy]))
      f.write(str(x01[iy][0])+" "+str(x_change[iy])+"\n")


    # for key, val in mapping1.items():
    #     print(f'{key} : {val}')

    # plt.plot(K, distortions, 'bx-')
    # distortions_group.append(distortions)
    # print(distortions)
    # plt.xlabel('Values of K')
    # plt.ylabel('Distortion')
    # plt.title('The Elbow Method using Distortion for ' + tests[i])
    # plt.savefig("folder_kmeans/kmeans2_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
    # plt.clf()

    # print()

    # for key, val in mapping2.items():
    #     print(f'{key} : {val}')


    # plt.plot(K, inertias, 'bx-')
    # inertia_group.append(inertias)
    # print(inertias)
    # plt.xlabel('Values of K')
    # plt.ylabel('Inertia')
    # plt.title('The Elbow Method using Inertia for ' + tests[i])
    # plt.savefig("kmeans3_"+tests[i]+".pdf", format="pdf", bbox_inches="tight")
    # plt.clf()




# from numpy import random
# distortions_group = [[884.8555483357925, 843.5071008256203, 625.5139741610175, 561.3401184868964, 428.1067969824576, 352.2177589469506, 351.4274141135474, 269.8631925140066, 242.85649105618572],
# [1086.6925260717499, 729.8986854540586, 674.3782712584493, 493.98970619065426, 413.6014919632205, 352.25482660504025, 325.2662240788543, 302.6236135087643, 254.87739881858946],
# [907.6510012352977, 864.0770460822366, 649.2135360843068, 637.1269252980454, 464.99658116751084, 381.274085653859, 299.3758382047016, 293.4031234682507, 257.32003896864285],
# [890.788705206539, 805.7758233886328, 794.5181929169659, 703.5569250294247, 585.6536336156144, 503.6805162885108, 458.7676256372898, 350.05824454529176, 330.00408244294084],
# [545.7078286237575, 426.4005600871963, 407.3870530175088, 333.7263268084207, 259.17866529165445, 244.9400445234103, 191.53563476247996, 178.9266693804347, 134.23866923657684],
# [950.8773998417565, 790.0866337092584, 616.3351599014356, 512.023031127828, 409.5371165104173, 397.73950254680256, 312.3623254343231, 282.6728754361945, 278.39094809356703],
# [1179.3811979869581, 1078.2584516121472, 808.0812553001591, 753.366885709354, 648.5148963155591, 519.8038853101576, 418.0379355977478, 422.5331202030053, 357.00870042875914],
# [740.168509898891, 671.2619552308194, 656.1091684788129, 353.64694808110175, 338.43394844374126, 334.56201080734326, 249.81041609143017, 250.989838774532, 172.99052400939289],
# [1312.223496584369, 874.8925175527278, 737.5792560753764, 562.697686028673, 484.00033947420764, 379.5960282975412, 303.9779511613454, 271.037312102186, 257.37235958958183],
# [800.0840932164747, 689.7815823013977, 504.6816439836638, 353.1566663624445, 263.1067409338471, 230.6814516402968, 218.48073943739627, 165.276788045654, 140.48797336437042],
# [1671.094857643881, 1614.8571304913353, 1147.7438828904344, 818.4831560409685, 714.2379845242676, 709.8801494873843, 610.5151898831745, 477.36895057420577, 442.6538637411752],
# [681.4777843881336, 630.2730964785226, 582.9820045798017, 440.3062314610628, 437.6065593247911, 319.05439208236714, 269.44133836634535, 231.13413819227216, 225.60465237803692],
# [835.2631659282706, 708.3689653164213, 516.3240289308925, 433.3144928792706, 388.1295556599219, 321.65218999111187, 253.04702512857358, 254.14702440628406, 248.96981700567983]]
# #random.randint(100, size=(13, 9))
# inertia_group = [[4041834608169.8643, 1316597408675.7073, 663509553919.4436, 358615460428.0781, 200942315592.98807, 140219433378.04675, 106877004981.60503, 75953162351.26622, 55211207001.36761],
# [2086943121383.5251, 942168104134.712, 459482101838.8422, 289689223171.25885, 194259850393.98392, 146044864122.54358, 102425530986.53145, 79517720095.57533, 62034519905.89582],
# [2972691347962.216, 1314163379499.2856, 701829924342.2626, 441333224746.6595, 254003879069.02084, 179437434481.85132, 135724221135.91405, 99312781197.73624, 76649425467.88042],
# [17001869296101.334, 5462879313321.961, 2568559774568.002, 1580376486940.1643, 1084295725112.2582, 673724762281.7483, 436877872307.4246, 335452664910.0979, 275778966125.9663],
# [1968819521588.0088, 1063384354374.9406, 597188141044.2098, 314212232482.1281, 192339662709.25436, 137371939826.93211, 92529115721.36876, 70644533320.95941, 54707832005.2218],
# [2752293642095.3345, 1236260579090.6028, 622018918916.1759, 396185665481.38965, 280208592618.27844, 183652386623.56772, 138377271976.75397, 110730278535.96819, 90094588584.27779],
# [2448146371397.163, 1010152451010.4769, 621931973356.2563, 384608464779.4226, 227116912198.99554, 160596491320.2679, 121692677441.04536, 97269054709.61893, 75775329370.29623],
# [1016773142674.285, 408347480450.7105, 191443676941.41406, 114454173443.95644, 84415311493.16048, 57653261686.81952, 41523289791.33648, 33463444994.601906, 28148486371.285263],
# [2061812624520.6836, 806588824557.197, 427844572642.49005, 265197007983.54587, 178827812260.7209, 127265998628.44507, 94266042386.17358, 75316067829.45331, 59297398015.701355],
# [2291243326349.069, 1033873480501.0825, 546415192541.56256, 326752298684.7857, 225258609019.9642, 157837190241.28827, 122272944952.30737, 93933380786.5101, 71692326045.56638],
# [3887830915215.9795, 1831342556939.093, 753774029650.511, 450714704317.6632, 341692078336.3251, 249032770026.69275, 173322481624.88763, 128432926892.69469, 100801278319.9759],
# [13776411420934.2, 3077446500673.1763, 1745922583862.9075, 1012583169666.8088, 691815242839.9199, 509481643217.82227, 386560777291.7646, 286313007661.79047, 223565842688.69867],
# [1322579561462.1467, 650961720731.8601, 356516922655.85724, 243530163999.37518, 160233412551.01678, 110458101369.30191, 84303716198.58026, 60893597375.07633, 48250098589.27402]]
# #random.randint(100, size=(13, 9))
# length=13
# K = range(1, 10)


# color_group = ['#800000', '#F08080', '#FFD700', '#808000', '#ADFF2F', '#228B22', '#32CD32', '#20B2AA',
# '#00FFFF', '#AFEEEE', '#4682B4', '#191970', '#8A2BE2']
# for i in range(length):
#     plt.plot(K, distortions_group[i], 'x-', color=color_group[i], label=tests[i])
# plt.xlabel('Values of K', fontsize = 15)
# plt.ylabel('Distortion', fontsize = 15)
# plt.legend(fontsize = 10, loc='upper right')
# plt.title('The Elbow Method using Distortion')
# plt.savefig("kmeans2_"+"DISTORTIONS"+".pdf", format="pdf", bbox_inches="tight")
# plt.clf()

# fig, ax1 = plt.subplots()
# for i in range(length):
#     plt.plot(K, inertia_group[i], 'x-', color=color_group[i], label=tests[i])
# plt.xlabel('Values of K', fontsize = 15)
# plt.ylabel('Log Inertia', fontsize = 15)
# ax1.set_yscale('log')
# plt.legend(fontsize = 10, loc='upper right')
# plt.title('The Elbow Method using Inertia')
# plt.savefig("folder_kmeans/kmeans3_"+"INERTIASlog"+".pdf", format="pdf", bbox_inches="tight")
# plt.clf()














# # from jenks import jenks
# import jenkspy
# import numpy as np
# def goodness_of_variance_fit(array, classes):
#     # get the break points
#     # classes = jenks(array, classes)
#     classes = jenkspy.jenks_breaks(array, classes) #nb_class=3)
#     # do the actual classification
#     classified = np.array([classify(i, classes) for i in array])
#     # max value of zones
#     maxz = max(classified)
#     # nested list of zone indices
#     zone_indices = [[idx for idx, val in enumerate(classified) if zone + 1 == val] for zone in range(maxz)]
#     # sum of squared deviations from array mean
#     sdam = np.sum((array - array.mean()) ** 2)
#     # sorted polygon stats
#     array_sort = [np.array([array[index] for index in zone]) for zone in zone_indices]
#     # sum of squared deviations of class means
#     sdcm = sum([np.sum((classified - classified.mean()) ** 2) for classified in array_sort])
#     # goodness of variance fit
#     gvf = (sdam - sdcm) / sdam
#     return gvf

# def classify(value, breaks):
#     for i in range(1, len(breaks)):
#         if value < breaks[i]:
#             return i
#     return len(breaks) - 1

# vf = 0.0
# nclasses = 2
# gvf = 0
# array = np.array([3, 1, 1, 2, 1, 6, 6, 6, 5, 6, 7, 8, 9, 8, 9, 9, 8, 5, 4, 5, 6, 5, 8, 6, 7, 6, 7, 1, 2, 1, 2, 3, 2, 3])
# X=[]
# Y=[]
# while gvf < .95:
#     gvf = goodness_of_variance_fit(array, nclasses)
#     print(nclasses, gvf)
#     X.append(nclasses)
#     Y.append(1-gvf)
#     nclasses += 1

# plt.plot(X, Y, 'bx-')
# plt.xlabel('Num of Classes')
# plt.ylabel('1-GVF')
# plt.savefig("jenks.pdf", format="pdf", bbox_inches="tight")
# plt.clf()

# import seg1d 

# #retrieve the sample reference, target, and weight data
# r,t,w = seg1d.sampleData()
# print(t)
# # define scaling percentage and rolling step size
# minW, maxW, step  = 70, 150, 1 
# #call the segmentation algorithm
# a = seg1d.segment_data(r,t,w,minW,maxW,step)
# print(a)