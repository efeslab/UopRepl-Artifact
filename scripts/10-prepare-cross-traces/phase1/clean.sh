
for allDir in result/*/; do
    echo "rm -r $allDir"
done

for allDir in analyze/*/; do 
    echo "rm $allDir*"
done

printf "Start cleaning? (Y/N): "
read answer
if [ "$answer" != "${answer#[Yy]}" ] ;then 
    for allDir in result/*/; do
      rm -r $allDir
    done

    for allDir in analyze/*/; do 
      rm $allDir*
    done
else
    echo Cancelled
fi