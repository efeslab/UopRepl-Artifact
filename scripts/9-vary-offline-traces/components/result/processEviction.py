import sys
import pandas as pd
import numpy as np
from collections import defaultdict

hintResults = defaultdict(lambda: [0, []])
missBucket = defaultdict(lambda: 0)
def process_csv(input_file, boundary, bitsNum, multFactor: int):
    df = pd.read_csv(input_file, sep=',',  header=None, names=['startAddr','hit'])
    df = df.reset_index(drop=True)
    for index, row in df.iterrows():
        if int(row['hit']) == 0:
            missBucket[row['startAddr']] += 1
            hintResults[row['startAddr']][0] = index
        if int(row['hit']) == 1:
            hintResults[row['startAddr']][1].append(index - hintResults[row['startAddr']][0])
            hintResults[row['startAddr']][0] = index
    for key, value in hintResults.items():
        if len(value[1]) < 2000:
            continue
        if missBucket[key] < 20:
            continue
        k = np.quantile(value[1], boundary) * multFactor
        if k <= bitsNum:
            print(f"{key},{(int)(k)}")

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python script.py inputCsv quantileBoundary bits multFactor")
        sys.exit(1)

    input_file = sys.argv[1]  # Get the input CSV file name from the command line argument
    boundary = float(sys.argv[2])  # Get the output CSV file name from the command line argument
    bitsNum = int(sys.argv[3])
    multFactor = int(sys.argv[4])
    process_csv(input_file, boundary, bitsNum, multFactor)