import csv
import sys
from collections import defaultdict

# 用于存储结果的字典
results = defaultdict(lambda: [0, 0])
hitResults = defaultdict(lambda: [0, 0])

# 读取 CSV 文件
csvPath = sys.argv[1]
appName = sys.argv[2]
boundary = float(sys.argv[3])
with open(csvPath, 'r') as csvfile:
    reader = csv.reader(csvfile)
    
    for row in reader:
        row = [cell.strip() for cell in row]

        key = (row[1], row[3])  # 第二列作为键
        value = int(row[4])  # 第四列的值
        if value == 0:
            results[key][0] += 1  # 计数器增加
        if value == 0 and int(row[5]) == 0:
            results[key][1] += 1  # 如果第四列的值为0，计数器增加
        
        hitResults[key][0] += 1
        if value == 1:
            hitResults[key][1] += 1

sorted_results = sorted(results.items(), key=lambda x: x[1][0], reverse=True)

# 计算比例并输出结果
# print("Addr,\tBypass Rate,\tTotal Access,\tHit rate,\tTotal Lookup")
totalTotal = 0
byPassTotal = 0
# boundary = 0.6
byPassTotalTotal = 0
for key, (total, zero_count) in sorted_results:
    zero_ratio = zero_count / total
    totalTotal += total
    byPassTotalTotal += zero_count
    if zero_ratio > boundary and (hitResults[key][1] / hitResults[key][0]) < 0.05:
        byPassTotal += zero_count
        # print(f"{key}, {zero_ratio:.2f}, {total}, {(hitResults[key][1] / hitResults[key][0]):.2f}, {hitResults[key][0]}")
        print(f"{key[0]},{key[1]}")
# print(len(sorted_results))
# print(f"*****{csvPath}*****")
# print(f"Total Access: {totalTotal}")
# print(f"Bypass Probability Boundary: {boundary}, Bypass: {byPassTotal}, occupy: {byPassTotal/totalTotal:.2f}")
# print(f"Total Access: {totalTotal}, Bypass Total: {byPassTotalTotal}, occupy {byPassTotalTotal/totalTotal:.2f}")
# print(f"{appName},TotalBypassRate,{byPassTotalTotal/totalTotal:.3f}")
# print(f"{appName},BoundaryBypassRate,{byPassTotal/totalTotal:.3f}")

