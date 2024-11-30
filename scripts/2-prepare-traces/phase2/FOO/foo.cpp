#include <fstream>
#include <lemon/lgf_writer.h>
#include <cassert>
#include "lib/parse_trace.h"
#include "lib/solve_mcf.h"

using namespace lemon;

int main(int argc, char* argv[]) {

    if (argc != 8) {
        std::cerr << argv[0] << " traceFile cacheSize solverParam resultPath set traceLength setSize" << std::endl;
        return 1;
    }

    std::string path(argv[1]);
    uint64_t cacheSize(std::stoull(argv[2]));
    int solverPar(atoi(argv[3]));
    std::string resultPath(argv[4]);

    // parse trace file
    std::vector<traceEntry> trace;
    auto [totalUniqC,totalUniqCost]= parseTraceFile(trace, path, atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));


    uint64_t totalReqc = trace.size();
    uint64_t totalCost = 0;
    for (auto i : trace) {
        totalCost+= std::get<3>(i);
    }
    std::cerr << "scanned trace n=" << totalReqc << " m=" << totalUniqC << std::endl;

    // create mcf instance
    SmartDigraph g; // mcf graph
    SmartDigraph::ArcMap<int64_t> cap(g); // mcf capacities
    SmartDigraph::ArcMap<double> cost(g); // mcf costs
    SmartDigraph::NodeMap<int64_t> supplies(g); // mcf demands/supplies

    createMCF(g, trace, cacheSize, cap, cost, supplies);
    
    std::cerr << "created graph with ";
    uint64_t nodes=0, vertices=0;
    for (SmartDigraph::NodeIt n(g); n!=INVALID; ++n) ++nodes;
    std::cerr << nodes << " nodes ";
    for (SmartDigraph::ArcIt a(g); a != INVALID; ++a) ++vertices;
    std::cerr << vertices << " arcs " << std::endl;

    SmartDigraph::ArcMap<uint64_t> flow(g);
    double solval = solveMCF(g, cap, cost, supplies, flow, solverPar);
    //    assert(solval>0);

   
    std::ofstream resultfile(resultPath);

    long double floatHits = 0;
    uint64_t integerHits = 0;
    long double totalHitValueUpper = 0;
    long double totalhitValueLower = 0;
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> lastSeen;
    // for(auto & it: trace) {
    for(uint64_t i = 0; i < trace.size();++i){
        auto & it = trace[i];
        const uint64_t id=std::get<0>(it);
        const uint64_t size=std::get<1>(it);
        const uint64_t time=std::get<3>(it); // cost
        const int arcId=std::get<4>(it);
        // resultfile << time << " " << id << " " << size << " ";
        if(arcId==-1){ 
            // resultfile << "0\n";
        }else {
            const long double dvar = (size-flow[g.arcFromId(arcId)])/static_cast<double>(size);
            totalHitValueUpper += dvar * time;
            // resultfile << dvar << "\n";
            floatHits += dvar;
            if(dvar == 1) { // Lower Bound for entire hit
                integerHits++;
                totalhitValueLower += time;

                std::get<6>(it) = 1;
            }
        }
        if(lastSeen.count(std::make_pair(id, time)) > 0){
            std::get<5>(it) = std::get<6>(trace[lastSeen[std::make_pair(id, time)]]);
        }
        lastSeen[std::make_pair(id, time)] = i;           
    }

    std::cout.precision(12);
    std::cout << std::fixed;

    std::cerr << "ExLP" << solverPar << " " << cacheSize << " hitc " <<totalHitValueUpper/totalCost<<" " << totalCost-totalUniqCost-solval << " reqc " << totalReqc << " OHR " << 1.0-(static_cast<double>(solval)+totalUniqCost)/totalCost << " " << floatHits << " " << integerHits << std::endl;
    std::cout<<totalhitValueLower <<" " << totalHitValueUpper << " " << totalCost<<std::endl;
    for(auto& it : trace){
        const uint64_t id=std::get<0>(it);
        const uint64_t size=std::get<1>(it);
        const uint64_t time=std::get<3>(it); // cost
        const int hit=std::get<5>(it);
        const int insert=std::get<6>(it);
        const int time_id=std::get<7>(it);
        resultfile << time_id << "," << id << "," << size << "," << time << "," << hit << "," << insert << "\n";
    }
    return 0;
}
