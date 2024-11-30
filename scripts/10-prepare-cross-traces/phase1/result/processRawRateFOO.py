import csv
import sys

hitDict = {}
totalAccess = {}
def process_csv(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:
        reader = csv.reader(infile)
        for row in reader:
            addr = row[1]
            hit = int(row[4])
            if hit == 1:
                if addr in hitDict:
                    hitDict[addr] += 1
                else:
                    hitDict[addr] = 1
            if addr in totalAccess:
                totalAccess[addr] += 1
            else:
                totalAccess[addr] = 1
        for addr, times in hitDict.items():
            outfile.write("{} {}\n".format(addr, times / totalAccess[addr]))

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py inputCsv outputPath")
        sys.exit(1)

    input_file = sys.argv[1]  # Get the input CSV file name from the command line argument
    output_file = sys.argv[2]  # Get the output CSV file name from the command line argument

    process_csv(input_file, output_file)