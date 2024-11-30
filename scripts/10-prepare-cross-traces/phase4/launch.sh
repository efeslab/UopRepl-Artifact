echo "Start1"
bash run.sh 0 &
bash run.sh 1 &
echo "Start2"
bash run.sh 2 &
wait

echo "Start3"
bash run.sh 3 &
echo "Start4"
bash run.sh 4 &
wait

bash run.sh 5 &
echo "Start2"
bash run.sh 6 &
wait

echo "Start3"
bash run.sh 7 &
echo "Start4"
bash run.sh 8 & 
wait

echo "Start5"
bash run.sh 9 &

bash runConcat.sh

wait