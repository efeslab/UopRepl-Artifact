for file in ../result/*
do 
    if [ -d $file ] 
    then 
        echo $file 
        name=$(basename $file)
        python3 parse_file.py --inputPath $file --outputFile $file/$name.csv
    fi 
done