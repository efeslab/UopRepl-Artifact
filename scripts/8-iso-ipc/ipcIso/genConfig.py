templatePath = "./template.config"
outputPath = "./config/"
templateName = "6_8_{}_{}_0_0_no.config"

setSizePool = [512, 576, 640, 704, 768, 832, 896, 960, 1024, 1088, 1152, 1216, 1280, 1344, 1408]

template = open(templatePath, "r")
templateStr = template.read()

for size in setSizePool:
    output = open(outputPath + templateName.format(size, size), "w")
    # get all string from template
    output.write(templateStr.format(size))
    output.close()
template.close()