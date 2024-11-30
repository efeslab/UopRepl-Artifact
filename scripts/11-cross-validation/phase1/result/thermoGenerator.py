import csv
import sys

inputFilePath = sys.argv[1]
outputFilePath = sys.argv[2]
hotBound = float(sys.argv[3])
coldBound = float(sys.argv[4])

def replace_values(filename):
    with open(filename, 'r') as file:
        reader = csv.reader(file, delimiter=' ')
        rows = list(reader)

    for row in rows:
        value = float(row[1])
        if value > hotBound:
            row[1] = '2'
        elif value < coldBound:
            row[1] = '0'
        else:
            row[1] = '1'

    with open(outputFilePath, 'w') as file:
        writer = csv.writer(file, delimiter=' ')
        writer.writerows(rows)

replace_values(inputFilePath)
