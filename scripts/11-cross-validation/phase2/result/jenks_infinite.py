import sys

def read_file(file_name):
    with open(file_name, 'r') as file:
        lines = file.readlines()
        data = [tuple(line.strip().split()) for line in lines]
    return data

def sort_data(data):
    return sorted(data, key=lambda x: float(x[1]), reverse=False)

def write_output_file(data, output_file_name):
    with open(output_file_name, 'w') as file:
        for index, item in enumerate(data, 1):
            file.write(f"{item[0]} {index}\n")

def main():
    if len(sys.argv) != 3:
        print("Usage: python script.py input_file output_file")
        sys.exit(1)
    
    input_file_name = sys.argv[1]
    output_file_name = sys.argv[2]
    
    data = read_file(input_file_name)
    sorted_data = sort_data(data)
    write_output_file(sorted_data, output_file_name)

if __name__ == "__main__":
    main()
