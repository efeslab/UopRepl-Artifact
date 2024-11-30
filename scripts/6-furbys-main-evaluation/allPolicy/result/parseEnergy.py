import os
benchmark_arr = ["cassandra", "drupal", "finagle-chirper", "kafka", "mediawiki", "tomcat", "postgres", "clang", "wordpress", "mysql", "python"]
def list_folders(directory):
    """List all folders under the specified directory."""
    folders = []
    for item in os.listdir(directory):
        if os.path.isdir(os.path.join(directory, item)):
            folders.append(item)
    return folders

# Replace '/path/to/directory' with the path to the directory you're interested in
directory_path = './'
folders = list_folders(directory_path)
outFile = open('energy.csv', 'w')
outFile.write('app,repl,Total,I-cache,Micro-op Cache,Decoder\n')

for str in folders:
    for app in benchmark_arr:

        repl = str.split('_')[3]
        if repl == "Belady" or repl == "foo":
            continue
        str2 = "./" + str +  "/"+app+"/mcpat.out"
        print(repl)
        # open file
        f = open(str2, 'r')
        if f is None:
            continue
        # read first four lines
        outFile.write(app + ',')
        outFile.write(repl + ',')
        for i in range(4):
            s = f.readline()
            # remove newline
            s = s[:-1]
            outFile.write(s)
            if i != 3 : outFile.write(',')
        outFile.write('\n')
