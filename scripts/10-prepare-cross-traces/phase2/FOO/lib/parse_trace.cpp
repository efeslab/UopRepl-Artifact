//#include <iostream>
#include <fstream>
#include <map>
#include <bitset>
#include <unordered_map>
#include <tuple>
#include <cassert>
#include "parse_trace.h"

using namespace lemon;

uint shiftBits = 6;

uint getIndex(long long addr, uint setSize){
  uint _addr = (uint) addr;
  uint indexMask = setSize - 1;
  return (_addr >> shiftBits) & indexMask;
}

std::pair<uint64_t, uint64_t> parseTraceFile(std::vector<traceEntry> & trace, std::string & path, uint64_t set, uint64_t length, uint setSize) {
    std::ifstream traceFile(path);
    uint64_t cost, id, endAddr, size, reqc=0, uniqc=0, uniqCost = 0;
    int time_id;

    char c; uint64_t tmpCount = 0;


    std::map<std::pair<uint64_t, uint64_t>, uint64_t> lastSeen;

    while(traceFile >> time_id >> c >> id >> c >> endAddr >> c >> cost >> c >> size) {
        tmpCount++;
        if(tmpCount == length) break;
        assert(getIndex(id, setSize) < setSize);
        if(getIndex(id, setSize) != set) continue;
        if(lastSeen.count(std::make_pair(id, cost))>0) {
            std::get<2>(trace[lastSeen[std::make_pair(id, cost)]]) = true;
        } else {
            uniqc += 1;
            uniqCost += cost;
        }
        trace.emplace_back(id, size, false, cost, -1, 0, 0, time_id);
        lastSeen[std::make_pair(id, cost)]=reqc++;
    }
    return {uniqc,uniqCost};
}
                    
void createMCF(SmartDigraph & g, std::vector<traceEntry> & trace, uint64_t cacheSize, SmartDigraph::ArcMap<int64_t> & cap, SmartDigraph::ArcMap<double> & cost, SmartDigraph::NodeMap<int64_t> & supplies) {

    // we consider (id,size) as unique identification of an object (sizes can change, but then it's a different object)
    // lastSeen maps (id,size) to (nodeId,traceIndex) of the last time this object was seen
    /* Under the context of Uop Cache: we use (id, cost) as unique identifier*/
    std::map<std::pair<uint64_t, uint64_t>, std::pair<uint64_t, int> > lastSeen;
    SmartDigraph::Arc curArc;
    SmartDigraph::Node curNode = g.addNode(); // initial node
    SmartDigraph::Node prevNode;

    // iterate over trace
    for(uint64_t i=0; i<trace.size(); i++) {
        const traceEntry thisTrEntry = trace[i];
        const uint64_t id=std::get<0>(thisTrEntry);
        const uint64_t size=std::get<1>(thisTrEntry);
        const bool nextRequest=std::get<2>(thisTrEntry);
        const uint64_t cost_all = std::get<3>(thisTrEntry);
        // first: check if previous interval ended here
        if(lastSeen.count(std::make_pair(id, cost_all))>0) {
            // create "outer" request arc
            const SmartDigraph::Node lastReq = g.nodeFromId(lastSeen[std::make_pair(id, cost_all)].second);
            curArc = g.addArc(lastReq,curNode);
            cap[curArc] = size;
            cost[curArc] = cost_all / static_cast <double>(size);
            supplies[lastReq] += size;
            supplies[curNode] -= size;
            std::get<4>(trace[lastSeen[std::make_pair(id, cost_all)].first]) = g.id(curArc);
        }
        // second: if there is another request for this object
        if(nextRequest) {
            // save prev node as anchor for future arcs
            prevNode = curNode;
            lastSeen[std::make_pair(id, cost_all)]=std::make_pair(i,g.id(prevNode));
            // create another node, "inner" capacity arc
            curNode = g.addNode(); // next node
            curArc = g.addArc(prevNode,curNode);
            cap[curArc] = cacheSize; 
            cost[curArc] = 0;
        }
    }
}
