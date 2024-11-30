./parse.sh 
./collect.sh

python3 ./combine.py
python3 ./sortCSV.py

python3 ./plotStat.py