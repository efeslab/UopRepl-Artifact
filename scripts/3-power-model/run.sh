runDir=$(pwd)

g++ -o toxml toXML.cpp


# Build McPAT and Cacti
cd $runDir/mcpat/cacti
make clean
cd ..
make clean
make -j
cd cacti
make -j

cd $runDir