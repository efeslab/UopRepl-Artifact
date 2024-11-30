import csv
import sys

def read_csv_file(file_name):
    with open(file_name, 'r') as csvfile:
        csvreader = csv.reader(csvfile)
        data = list(csvreader)
        return data[:-3]  # 返回除最后两行之外的所有数据

def process_data(data):
    processed_data = []
    cumulative_sum = 1
    for row in data:
        processed_data.append([row[0], cumulative_sum])
        cumulative_sum += int(row[2])
    return processed_data

def write_output_data(data):
    for row in data:
        print(f"{row[0]} {row[1]}")

def main():
    input_file_name = sys.argv[1]  # 更改为你的CSV文件名
    csv_data = read_csv_file(input_file_name)
    processed_data = process_data(csv_data)
    write_output_data(processed_data)

if __name__ == "__main__":
    main()
