import csv
import operator
import sys

# 从命令行参数中获取 CSV 文件的路径
csv_file = sys.argv[1]

# 读取 CSV 文件中的数据，并删除空白行
with open(csv_file, 'r') as f:
    reader = csv.reader(f)
    data = [row for row in reader if row]

# 将第一列的元素转换成整数
data = [[int(row[0])] + row[1:] for row in data]

# 按照第一列的值排序数据
sorted_data = sorted(data, key=operator.itemgetter(0))

# 将排序后的数据写回原始 CSV 文件
with open(csv_file, 'w', newline='') as f:
    writer = csv.writer(f)
    # 将第一列的元素转换回字符串
    sorted_data = [[str(row[0])] + row[1:] for row in sorted_data]
    writer.writerows(sorted_data)
