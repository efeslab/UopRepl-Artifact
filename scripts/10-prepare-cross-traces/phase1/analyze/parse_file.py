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
        "UOP_Miss_on": -1,
        "UOP_Hit_on": -1,
        "UOP_Miss_off": -1,
        "UOP_Hit_off": -1,
        "UOP_MPKI": float("nan"),
        "UOP_MR": float("nan"),
        "Cycle_saved": -1,
        "UOP_UC_TO_IC_SWITCH": -1,
        "Redirect": -1,
        "Wasteful_Cycles": -1,
        "Success": False,
        "fetch_cycles": -1,
        "switch_cycles": -1,
        "mispred_cycles": -1,
        "miss_cycles": -1,
        "redirect_cycles": -1,
        "uops_from_ic": -1,
        "uops_from_uc": -1,
        "cycles_from_ic": -1,
        "cycles_from_uc": -1,
        "cycles_starve": -1,
    }
    for i in range(1,9):
        result["UOP_PW_LENGTH_"+str(i)] = 0
    result["UOP_PW_LENGTH_MORE"] = 0
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
    result["UOP_Miss"] = int(line_uop_miss.group(1))
    result["UOP_MPKI"] = result["UOP_Miss"] * 1.0 / result["instructions"]

    line_uop_hit = re.search('UOP_CACHE_HIT[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%', s)
    result["UOP_Hit"] = int(line_uop_hit.group(1))

    result["UOP_MR"] = float(result["UOP_Miss"] / (result["UOP_Miss"] + result["UOP_Hit"]))
    
    line_uop_hit = re.search('UOP_CACHE_CYCLES_SAVED[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    result["Cycle_saved"] = int(line_uop_hit.group(1))

    line_uop_switch = re.search('UOP_CACHE_UC_TO_IC_SWITCH[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    result["UOP_UC_TO_IC_SWITCH"] = int(line_uop_switch.group(1))
    
    rList=[str(i) for i in range(1,9)] + ["MORE"]
    for i in rList:
        # print("UOP_PW_LENGTH_" +i+"[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]")
        # print(s)
        line_of_pw_length = re.search("UOP_PW_LENGTH_" +i+"[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*",s,re.IGNORECASE)
        result["UOP_PW_LENGTH_"+str(i)] = int(line_of_pw_length.group(1))
    
    s = (trace_dir / "fetch.stat.0.out").open().read()
    line_uop_hit = re.search('INST_LOST_WAIT_FOR_REDIRECT[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%[\\s]*([0-9\.]*)[\\s]*([0-9\.]*)%', s)
    result["Redirect"] = int(line_uop_hit.group(1))

    redirect_cycles = re.search('CYCLE_USED_REDIRECT[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    redirect_cycles = int(redirect_cycles.group(1))

    fetch_cycles = re.search('CYCLE_USED_FETCH[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    fetch_cycles = int(fetch_cycles.group(1))

    switch_cycles = re.search('CYCLE_USED_SWITCH[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    switch_cycles = int(switch_cycles.group(1))

    mispred_cycles = re.search('CYCLE_USED_MISPRED[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    mispred_cycles = int(mispred_cycles.group(1))

    miss_cycles = re.search('CYCLE_USED_ICACHE_MISS[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    miss_cycles = int(miss_cycles.group(1))

    result["Wasteful_Cycles"] = redirect_cycles + fetch_cycles + switch_cycles + mispred_cycles + miss_cycles
    result["fetch_cycles"] = fetch_cycles
    result["switch_cycles"] = switch_cycles
    result["mispred_cycles"] = mispred_cycles
    result["miss_cycles"] = miss_cycles
    result["redirect_cycles"] = redirect_cycles

    s = (trace_dir / "fetch.stat.0.out").open().read()
    uops_from_ic = re.search('UOPS_FROM_LEGACY[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    result["uops_from_ic"] = int(uops_from_ic.group(1))

    uops_from_uc = re.search('UOPS_FROM_UC[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    result["uops_from_uc"] = int(uops_from_uc.group(1))
    
    cycles_from_ic = re.search('CYCLES_FROM_LEGACY[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    result["cycles_from_ic"] = int(cycles_from_ic.group(1))

    cycles_from_uc = re.search('CYCLES_FROM_UC[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    result["cycles_from_uc"] = int(cycles_from_uc.group(1))

    total_cycles = re.search('ICACHE_CYCLE[\\s]*([0-9]*)[\\s]*([0-9]*)[\\s]*', s)
    total_cycles = int(total_cycles.group(1))

    result["cycles_starve"] = total_cycles - result["cycles_from_ic"] - result["cycles_from_uc"]
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
    for traceName in file_root.iterdir():
        if traceName.is_dir():
            print_to_summary(output_file, parse_one_file(traceName, traceName.stem))
    output_file.close()


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
