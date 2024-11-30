sourceDir=/mnt/storage/xianshengzhao/uop/0420_out_tests
resultDir=$(pwd)

cp $sourceDir/finalConfig/6_8_1024_lru_0_0_no.config $resultDir/config/6_8_1024_lru_0_0_no.config
cp $sourceDir/finalConfig/6_8_1024_srrip_0_0_no.config $resultDir/config/6_8_1024_srrip_0_0_no.config
cp $sourceDir/finalConfig/6_8_1024_ship_0_0_no.config $resultDir/config/6_8_1024_ship_0_0_no.config
cp /mnt/storage/xianshengzhao/uop/0601_scarab_GHRP/config/6_8_1024_ghrp_0_0_no.config $resultDir/config/6_8_1024_ghrp_0_0_no.config

cp $sourceDir/finalConfig/6_8_1024_DynamicBypass_5_1_no.config $resultDir/config/6_8_1024_DynamicBypass_5_1_no.config
cp $sourceDir/preprocess/6_8_1024_DynamicBypass_5_1_no.sh $resultDir/preprocess/6_8_1024_DynamicBypass_5_1_no.sh

cp $sourceDir/finalConfig/6_8_1024_foo_0_0_no.config $resultDir/config/6_8_1024_foo_0_0_no.config
cp $sourceDir/preprocess/6_8_1024_foo_0_0_no.sh $resultDir/preprocess/6_8_1024_foo_0_0_no.sh