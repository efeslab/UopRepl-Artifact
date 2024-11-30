import pathlib
import re
import click
from io import TextIOWrapper
from pathlib import Path
from typing import TextIO
import sys


benchmark_arr = ["cassandra", "drupal", "finagle-chirper", "kafka", "mediawiki", "tomcat", "postgres", "clang", "wordpress", "mysql", "python"]
X = {}
Y = {}
def getResult():
    for testSet in benchmark_arr:
        with open("./6_8_1024_DynamicBypass_3_1_no"+"/"+testSet+"/memory.stat.0.out", "r") as f:
            s = f.read()
            line_coldMiss = re.search('UOP_CACHE_REPL_ALL[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s,re.IGNORECASE)
            x = int(line_coldMiss.group(1))
            line_coldMiss = re.search('UOP_CACHE_REPL_DEGRADE[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s,re.IGNORECASE)
            y = int(line_coldMiss.group(1))
            X[testSet] = x
            Y[testSet] = y
def printResult():
    print("app,coverage,repl")
    for testSet in benchmark_arr:
        print(f"{testSet},{1-Y[testSet]/X[testSet]},degrade")
if __name__ == "__main__":
    getResult()
    printResult()