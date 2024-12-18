#include <vector>
#include <string>
#include <lemon/smart_graph.h>

using namespace lemon;

typedef std::tuple<uint64_t,uint64_t,bool,uint64_t,int,int,int,int> traceEntry; //id, size, hasNext, cost, arcId, hit, insert/evict, time_id

std::pair<uint64_t, uint64_t> parseTraceFile(std::vector<traceEntry> & trace, std::string & path, uint64_t set, uint64_t length, uint setSize);

void createMCF(SmartDigraph & g, std::vector<traceEntry> & trace, uint64_t cacheSize, SmartDigraph::ArcMap<int64_t> & cap, SmartDigraph::ArcMap<double> & cost, SmartDigraph::NodeMap<int64_t> & supplies);
