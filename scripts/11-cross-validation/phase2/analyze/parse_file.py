import pathlib
import re
import click
from io import TextIOWrapper
from pathlib import Path
from typing import TextIO


# file_root = Path.cwd() /".."/"gdata-sim-result"
# output_path = Path.cwd() / "out_modified.txt"


def get_default_stat_result():
    result = {
        "app": str(),
        "ipc": float("nan"),
        "instructions": float("nan"),
        "UOP_Miss": -1,
        "UOP_Hit": -1,
        "UOP_MPKI": float("nan"),
        "UOP_MR": float("nan"),
        "Success": False
    }
    for i in range(1,9):
        result["UOP_PW_LENGTH_"+str(i)] = 0
    result["UOP_PW_LENGTH_MORE"] = 0
    for i in range(1,9):
        result["UOP_BLOCK_OPNUM_"+str(i)] = 0
    result["UOP_BLOCK_OPNUM_MORE"] = 0
    return result


def print_header(output_file: TextIO):
    value_list = [str(item) for item in get_default_stat_result().keys()]
    output_file.write(",".join(value_list))
    output_file.write("\n")


def print_to_summary(output_file: TextIO, result: dict):
    value_list = [str(item) for item in result.values()]
    output_file.write(",".join(value_list))
    output_file.write("\n")
    # print(",".join(value_list))


def parse_one_file(trace_dir: pathlib.Path, trace_name: str):
    result = get_default_stat_result()
    result["app"] = trace_name
    log2File = trace_dir / "log2.txt"
    if log2File.exists() :
        s = (trace_dir / "log2.txt").open().read()
        result["Success"] = True
        if not "Scarab finished at" in s:
            result["Success"] = False
    else:
        result["Success"] = True
    if(not (trace_dir / "memory.stat.0.out").exists()):
        return get_default_stat_result()
    
    s = (trace_dir / "memory.stat.0.out").open().read()
    # print(tid)
    line_ipc = re.search('Cumulative:[\\s]*Cycles: (.*)[\\s]*Instructions: (.*)[\\s]*IPC: (.*)', s)
    result["ipc"] = float(line_ipc.group(3))
    result["instructions"] = int(line_ipc.group(2))
    if result["instructions"] == 0:
        return result
    line_uop_miss = re.search('UOP_CACHE_MISS[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%', s)
    result["UOP_MR"] = float(line_uop_miss.group(2))
    result["UOP_Miss"] = int(line_uop_miss.group(1))
    result["UOP_MPKI"] = result["UOP_Miss"] * 1.0 / result["instructions"]

    line_uop_hit = re.search('UOP_CACHE_HIT[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%', s)
    result["UOP_Hit"] = int(line_uop_hit.group(1))
    
    rList=[str(i) for i in range(1,9)] + ["MORE"]
    for i in rList:
        # print("UOP_PW_LENGTH_" +i+"[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]")
        # print(s)
        line_of_pw_length = re.search("UOP_PW_LENGTH_" +i+"[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*",s,re.IGNORECASE)
        result["UOP_PW_LENGTH_"+str(i)] = int(line_of_pw_length.group(1))
        # print("UOP_BLOCK_OPNUM_" +i+"[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]")
        line_of_bl = re.search("UOP_BLOCK_OPNUM_" +i+"[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*",s,re.IGNORECASE)
        result["UOP_BLOCK_OPNUM_"+str(i)] = int(line_of_bl.group(1))
    return result


@click.command()
@click.option("--inputPath", required=True)
@click.option("--outputFile", required=True)
def main(inputpath: str, outputfile: str):
    file_root = Path(inputpath)
    print(file_root)
    output_path = Path(outputfile)
    output_file = output_path.open("w")
    print_header(output_file)
    traceName = file_root
    print_to_summary(output_file, parse_one_file(traceName, traceName.stem))
    output_file.close()


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
