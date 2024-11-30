for file in ../result/*
do 
    if [ -d $file ] && [ $file != "./collect" ]
    then 
        echo $file 
        name=$(basename $file)
        cp $file/$name.csv collect/$name.csv 
    fi 
done