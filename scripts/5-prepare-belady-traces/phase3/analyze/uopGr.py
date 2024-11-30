import csv
import sys
from collections import defaultdict

# 用于存储结果的字典
hitResults = defaultdict(lambda: [0, 0])

# 读取 CSV 文件
csvPath = sys.argv[1]
appName = sys.argv[2]
boundary = float(sys.argv[3])
with open(csvPath, 'r') as csvfile:
    reader = csv.reader(csvfile)
    
    for row in reader:
        row = [cell.strip() for cell in row]

        key = row[0]
        value = int(row[1])
        
        hitResults[key][0] += 1
        if value == 0:
            hitResults[key][1] += 1

sorted_results = sorted(hitResults.items(), key=lambda x: x[1][0], reverse=True)

# 计算比例并输出结果
# print("Addr,\tBypass Rate,\tTotal Access,\tHit rate,\tTotal Lookup")
totalTotal = 0
byPassTotal = 0
byPassTotalTotal = 0
# boundary = 0.6
missTotal = 0

totalCount = 0
bypassCount = 0
for key, (total, zero_count) in sorted_results:
    totalCount += 1
    zero_ratio = zero_count / total
    totalTotal += total
    missTotal += zero_count
    if zero_ratio > boundary :
        byPassTotal += zero_count
        byPassTotalTotal += total
        bypassCount += 1
        # print(f"{key}, {zero_ratio:.2f}, {total}, {(hitResults[key][1] / hitResults[key][0]):.2f}, {hitResults[key][0]}")
        print(f"{key}")
# print(f"{bypassCount},{totalCount}")
# print(f"{byPassTotal},{byPassTotalTotal},{missTotal},{totalTotal}")

# print(len(sorted_results))
# print(f"*****{csvPath}*****")
# print(f"Total Access: {totalTotal}")
# print(f"Bypass Probability Boundary: {boundary}, Bypass: {byPassTotal}, occupy: {byPassTotal/totalTotal:.2f}")
# print(f"Total Access: {totalTotal}, Bypass Total: {byPassTotalTotal}, occupy {byPassTotalTotal/totalTotal:.2f}")
# print(f"{appName},TotalBypassRate,{byPassTotalTotal/totalTotal:.3f}")
# print(f"{appName},BoundaryBypassRate,{byPassTotal/totalTotal:.3f}")

