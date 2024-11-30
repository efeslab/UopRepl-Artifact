targetDir="/home/kanzhu/uop/1120_final_all/ablation/phase2"
mkdir -p $targetDir
cp -r ./analyze $targetDir/
cp -r ./baseline $targetDir/
cp -r ./config $targetDir/
cp -r ./post-config $targetDir/
cp -r ./process $targetDir/
cp -r ./preprocess $targetDir/
cp clean.sh $targetDir/
cd result
mkdir -p $targetDir/result
for file in *[!no]*; do
    cp $file $targetDir/result/
done