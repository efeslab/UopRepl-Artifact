#include <iostream>
#include <fstream>
#include <string>

using namespace std;


int main (int argc, char* argv[]){
    if(argc != 3) {
      cerr << "traceFile outputFile" << endl;
      return 1;
    }
    const char* path = argv[1];
    ifstream infile;
    infile.open(path);

    const char* outpath = argv[2];
    ofstream outfile(outpath);

    long long startAddr, endAddr, cost, size;
    char c;
    int tmpCount = 0;
    while (infile >> startAddr >> c >> endAddr >> c >> cost >> c >> size){
      outfile << tmpCount << "," << startAddr << "," << endAddr << "," << cost << "," << size << "\n";
      tmpCount++;
    }
    return 0;
}