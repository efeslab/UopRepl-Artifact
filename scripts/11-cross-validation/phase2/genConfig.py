import os

# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.
import pathlib
import argparse


def choose_largest_traces(trace_dir: pathlib.Path, choose_num: int):
    return sorted(trace_dir.iterdir(), key=lambda x: x.stat().st_size, reverse=True)[
        0:choose_num
    ]


def main():
    argparser = argparse.ArgumentParser()
    argparser.add_argument("--trace_path", type=str, required=True)
    argparser.add_argument("--inst_count", type=int, required=True)
    args = argparser.parse_args()
    
    outputFolder = pathlib.Path.cwd()
    configFolder = outputFolder/"config"
    processFolder = outputFolder/"preprocess"
    config3 = open("6_8_512_DynamicBypass_3_2_no.config","r").read()
    sh3 = open("6_8_512_DynamicBypass_3_2_no.sh","r").read()

    traceBase = pathlib.Path(args.trace_path)
    appList = ["kafka" ,"mysql", "postgres","clang","cassandra","finagle-chirper","python","tomcat","mediawiki","drupal","wordpress"]
    dirList = ["kafka", "mysql_large", "pgbench","clang","cassandra","finagle-chirper","python/original","tomcat","mediawiki","drupal","wordpress"]
    for app_idx, app in enumerate(appList):
        chosen = choose_largest_traces(traceBase/dirList[app_idx],10)
        inst_count = args.inst_count
        for i in range(len(chosen)):
            configName = "{}_DynamicBypass_{}.config".format(app, i)
            genOut=open(configFolder/configName,"w")
            genOut.write(config3.format(chosen[i], inst_count))
            genOut.close()

            genOut=open(processFolder/"{}_DynamicBypass_{}.sh".format(app, i),"w")
            genOut.write(sh3.format(i, app)) # For self / cross
            genOut.close()


    print("finish")

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
