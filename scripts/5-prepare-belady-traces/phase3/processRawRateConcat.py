import csv
import sys

hitDict = {}
totalAccess = {}
def process_csv(inputFileList, output_file):
    for fileName in inputFileList:
        with open(fileName, 'r') as infile:
            reader = csv.reader(infile)
            for row in reader:
                addr = row[1]
                hit = int(row[2])
                if hit == 1:
                    if addr in hitDict:
                        hitDict[addr] += 1
                    else:
                        hitDict[addr] = 1
                if addr in totalAccess:
                    totalAccess[addr] += 1
                else:
                    totalAccess[addr] = 1
    
    with open(output_file, 'w', newline='') as outfile:
        for addr, times in hitDict.items():
            outfile.write("{} {}\n".format(addr, times / totalAccess[addr]))

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python script.py inputCsv outputPath")
        sys.exit(1)

    inputFileList = [sys.argv[i] for i in range(1,4)]  # Get the input CSV file name from the command line argument
    output_file = sys.argv[4]  # Get the output CSV file name from the command line argument

    process_csv(inputFileList, output_file)