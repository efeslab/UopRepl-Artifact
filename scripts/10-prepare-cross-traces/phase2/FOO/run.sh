rm combine.txt


for i in {0..511}
do
./foo cassandra_pw_clean.txt 8 4 out.txt $i > out/$i.txt &
    activeCount=$(ps aux | grep 'foo' | awk '{print $2}' | wc -l)
  while ((activeCount>=110)); do
    sleep 1
    activeCount=$(ps aux | grep 'foo' | awk '{print $2}' | wc -l)
  done
done

wait

for file in ./out/*
do
    cat $file >> combine.txt
done

python3 calc.py | tee result.txt