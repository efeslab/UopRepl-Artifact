import pathlib
from pathlib import Path

inputFolder = Path.cwd() / "collect"
outputPath = Path.cwd() /"stat"/"combine_result.csv"
output_file = outputPath.open("w")
first_file = True
for fileIter in inputFolder.iterdir():
    filename = fileIter.name
    core=filename.split(".")[0]
    parameter_list = core.split("_")
    useful_list = parameter_list
    front_str = ','.join(useful_list)
    fileInput = fileIter.open("r")
    header = fileInput.readline()
    if first_file:
        temp_str=""
        for i in range(len(parameter_list)):
            temp_str =temp_str+ "Para"+str(i) + ","
        header = temp_str + header
        output_file.write(header)
        first_file = False
    while fileInput:
        useful = fileInput.readline()
        if useful == "":
            break
        output_file.write(front_str + "," + useful)
    fileInput.close()
output_file.close()
