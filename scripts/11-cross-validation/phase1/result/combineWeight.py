import pandas as pd
import sys

def process_csv_files(file1, file2, outputPath):
    # 读取两个以空格分隔的csv文件
    df1 = pd.read_csv(file1, header=None, sep=' ')
    df2 = pd.read_csv(file2, header=None, sep=' ')

    # 将两个csv文件按照第一列进行合并
    merged_df = df1.merge(df2, on=0, how='inner')

    # 按照规则计算第二列的值
    merged_df['output'] = 0.9 * merged_df.iloc[:, 1] + 0.1 * merged_df.iloc[:, 2]

    # 选择需要的列
    result = merged_df.iloc[:, [0, 3]]

    # 输出计算结果
    result.to_csv(outputPath, header=False, index=False, sep=' ')

if __name__ == "__main__":
    csv_file1 = sys.argv[1]
    csv_file2 = sys.argv[2]
    outputPath = sys.argv[3]
    process_csv_files(csv_file1, csv_file2, outputPath)
