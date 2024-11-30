#include <iostream>
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <fstream>

using namespace std;

extern "C" {
#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"
#include "isa/isa_macros.h"

#include "bp/bp.h"
#include "bp/bp_targ_mech.h"
#include "libs/cache_lib.h"
#include "sim.h"

#include "bp/bp.param.h"
#include "core.param.h"
#include "debug/debug.param.h"
#include "statistics.h"

#include <stdlib.h>

#include "core.param.h"
#include "general.param.h"
#include "libs/cache_lib.h"
#include "memory/memory.param.h"

#include <search.h>
#include <math.h>
}

#define PRINT_FUNC 0



/**************************************************************************************/
/* bp_btb_shotgun_init: */

// Shotgun data structures
Cache shotgun_prefetch_buffer;
Cache ubtb;
Cache cbtb;
Cache rib;
unordered_map<Addr,vector<unordered_map<Addr,Addr>>> call_footprints;
unordered_map<Addr,vector<unordered_map<Addr,Addr>>> return_footprints;
unordered_map<Addr,set<uns64>> cl_decoded_entries;
unordered_map<Addr,uint64_t> last_prefetched_cycle;
unordered_map<Addr,uns64> last_evicted_cycle;
unordered_map<Addr,uns64> overall_execution_count;
unordered_map<pair<Addr,Addr>,uns64,boost::hash<pair<Addr, Addr>>> overall_execution_count_concat;
unordered_map<Addr, uns64> second_overall_execution_count;
vector<pair<string, int> > ordered_overall_execution_count;
unordered_map<Addr,uns64> granu_overall_execution_count;
map<string, int> second_overall_execution_count_switch;
vector<pair<string, int> > ordered_overall_execution_count_switch;
unordered_map<Addr,uns64> granu_overall_execution_count_switch;
unordered_map<Addr,pair<uns64,uns64>> overall_hit_to_access;
unordered_map<pair<Addr,Addr>,pair<uns64,uns64>,boost::hash<pair<Addr, Addr>>> overall_hit_to_access_concat;
unordered_map<Addr,float> overall_hit_to_access_rate;
unordered_map<pair<Addr,Addr>,float,boost::hash<pair<Addr, Addr>>> overall_hit_to_access_rate_concat;
unordered_map<Addr,set<Addr>> unconditional_jit_checker;
unordered_map<Addr,uns64> overall_miss_count;
unordered_map<Addr,pair<uns64,uns64>> overall_cycle_count;
bool is_return = false;
Addr last_unconditional_branch_pc = 0;
stack<Addr> call_stack;

void read_file(std::string file_path, std::vector<std::string> &data_destination){
  std::ifstream infile(file_path);
  std::string line;
  data_destination.clear();
  while(std::getline(infile, line)) {
    data_destination.push_back(line);
  }
  infile.close();
}

void read_init() {
  if (FOOTPRINT) {
    std::cout << "Read New OPT traces from: " << FOOTPRINT << std::endl;
    std::vector<std::string> all_strings;
    read_file(FOOTPRINT, all_strings);
    std::cout<<all_strings.size()<<std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      //boost::trim_if(line, boost::is_any_of("\n"));
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      // std::cout<<parsed.size()<<' '<<j<<std::endl;
      // std::cout<<j<<' '<<line<<std::endl;
      if(parsed.size()>2) {
        printf("Read error\n");
      }
      uint64_t function_start = strtoul(parsed[0].c_str(), NULL, 10);
      // uint64_t function_end = strtoul(parsed[1].c_str(), NULL, 10);
      uint64_t target = strtoul(parsed[1].c_str(), NULL, 10);
      // cl_decoded_entries[function_start] =set<uns64>();
      // std::cout<<function_start<<' '<<target<<std::endl;
      cl_decoded_entries[function_start].insert(target);
    }
    printf("Initializing NEW OPT with size %u\n", cl_decoded_entries.size());
    all_strings.clear();
  }
}

void read_init_cacheaccess() {
  if (FOOTPRINT) {
    std::cout<<FOOTPRINT<<std::endl;
    std::vector<std::string> all_strings;
    read_file(FOOTPRINT,all_strings);
    std::cout<<all_strings.size()<<std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      //boost::trim_if(line, boost::is_any_of("\n"));
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      // std::cout<<parsed.size()<<' '<<j<<std::endl;
      // std::cout<<j<<' '<<line<<std::endl;
      if(parsed.size()>2) {
        printf("Read error\n");
      }
      uint64_t addr = strtoul(parsed[0].c_str(), NULL, 10);
      // uint64_t function_end = strtoul(parsed[1].c_str(), NULL, 10);
      uint64_t count = strtoul(parsed[1].c_str(), NULL, 10);
      // std::cout<<function_start<<' '<<target<<std::endl;
      overall_execution_count[addr]=(count);
    }
    printf("Initializing footprint with size %u\n", overall_execution_count.size());
    // for(auto it = overall_execution_count.begin(); it != overall_execution_count.end(); ++it)
    // {
    //     std::cout << it->first << " " << it->second << endl;
    // }
    
    all_strings.clear();
  }
}

void read_init_cacheaccess_concat() {
  if (FOOTPRINT) {
    std::cout<<FOOTPRINT<<std::endl;
    std::vector<std::string> all_strings;
    read_file(FOOTPRINT,all_strings);
    std::cout<<all_strings.size()<<std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      //boost::trim_if(line, boost::is_any_of("\n"));
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      // std::cout<<parsed.size()<<' '<<j<<std::endl;
      // std::cout<<j<<' '<<line<<std::endl;
      if(parsed.size()>3) {
        printf("Read error\n");
      }
      uint64_t addr = strtoul(parsed[0].c_str(), NULL, 10);
      uint64_t addr_end = strtoul(parsed[1].c_str(), NULL, 10);
      // uint64_t function_end = strtoul(parsed[1].c_str(), NULL, 10);
      uint64_t count = strtoul(parsed[2].c_str(), NULL, 10);
      // std::cout<<function_start<<' '<<target<<std::endl;
      overall_execution_count_concat[make_pair(addr,addr_end)]=(count);
    }
    printf("Initializing footprint with size %u\n", overall_execution_count_concat.size());
    // for(auto it = overall_execution_count_concat.begin(); it != overall_execution_count_concat.end(); ++it)
    // {
    //     std::cout << (it->first).first << " " << (it->first).first << " " << it->second << endl;
    // }
    
    all_strings.clear();
  }
}


bool cmp(pair<string, int>& a,
         pair<string, int>& b)
{
    return a.second < b.second;
}

// Function to sort the map according
// to value in a (key-value) pairs
vector<pair<string, int> > sorta(map<string, int>& M)
{
    // Declare vector of pairs
    vector<pair<string, int> > A;
    // Copy key-value pair from Map
    // to vector of pairs
    for (auto& it : M) {
        A.push_back(it);
    }
    // Sort using comparator function
    sort(A.begin(), A.end(), cmp);
    // Print the sorted value
    // for (auto& it : A) {
    //     cout << it.first << ' '
    //          << it.second << endl;
    // }
    return A;
}


void read_init_cacheaccess_granularities() {
  if (PMC_FOOTPRINT) {
    std::cout << "Read PMC Traces from: " << PMC_FOOTPRINT << std::endl;
    std::vector<std::string> all_strings;
    read_file(PMC_FOOTPRINT, all_strings);
    std::cout << all_strings.size() << std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      if(parsed.size()>2) {
        printf("Read error\n");
      }
      Addr addr = strtoul(parsed[0].c_str(), NULL, 10);
      int count = stoi(parsed[1].c_str());
      granu_overall_execution_count[addr] = count;
    }
    printf("Initializing PMC with size %u\n", granu_overall_execution_count.size());
    all_strings.clear();
  }
}

void read_init_cacheaccess_granularities_switch() {
  if (FOOTPRINT_SWITCH) {
    std::cout<<FOOTPRINT_SWITCH<<std::endl;
    std::vector<std::string> all_strings;
    read_file(FOOTPRINT_SWITCH,all_strings);
    std::cout<<all_strings.size()<<std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      if(parsed.size()>2) {
        printf("Read error\n");
      }
      string addr = parsed[0].c_str(); //strtoul(parsed[0].c_str(), NULL, 10);
      int count = stoi(parsed[1].c_str()); //strtoul(parsed[1].c_str(), NULL, 10);
      second_overall_execution_count_switch[addr]=(count);
    }
    ordered_overall_execution_count_switch = sorta(second_overall_execution_count_switch); 
    uns64 length = ordered_overall_execution_count_switch.size();
    for (size_t i = 0; i < length; i++)
    {
      uns64 ori_count = ordered_overall_execution_count_switch[i].second;
      granu_overall_execution_count_switch[strtoul((ordered_overall_execution_count_switch[i].first).c_str(), NULL, 10)]= ori_count; //floor(i/(length/pow(2, UOP_ACCESS_COUNT_BIT)));
    }
    printf("Initializing footprint with size %u\n", granu_overall_execution_count_switch.size());
    all_strings.clear();
  }
}


void read_init_ratecacheaccess() {
  if (FOOTPRINT) {
    std::cout<<FOOTPRINT<<std::endl;
    std::vector<std::string> all_strings;
    read_file(FOOTPRINT,all_strings);
    std::cout<<all_strings.size()<<std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      //boost::trim_if(line, boost::is_any_of("\n"));
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      // std::cout<<parsed.size()<<' '<<j<<std::endl;
      // std::cout<<j<<' '<<line<<std::endl;
      if(parsed.size()>3) {
        printf("Read error\n");
      }
      uint64_t addr = strtoul(parsed[0].c_str(), NULL, 10);
      // uint64_t function_end = strtoul(parsed[1].c_str(), NULL, 10);
      uint64_t hit = strtoul(parsed[1].c_str(), NULL, 10);
      uint64_t count = strtoul(parsed[2].c_str(), NULL, 10);
      // std::cout<<function_start<<' '<<target<<std::endl;
      uint64_t updated_hit=0, updated_count=0;
      if (overall_hit_to_access.count(addr)){
        updated_hit = overall_hit_to_access[addr].first;
        updated_count = overall_hit_to_access[addr].second;
      }
      updated_hit += hit;
      updated_count += count;
      overall_hit_to_access[addr]=make_pair(updated_hit,updated_count);
    }
    printf("Initializing footprint with size %u\n", overall_hit_to_access.size());
    for(auto it = overall_hit_to_access.begin(); it != overall_hit_to_access.end(); ++it)
    {
        float rate = static_cast<float>((it->second).first)/(it->second).second;
        overall_hit_to_access_rate[it->first] = rate;
        if(PRINT_FUNC)  std::cout << it->first << " " << (it->second).first << " " << (it->second).second << " " << rate << endl;
    }
    
    all_strings.clear();
  }
}

void read_init_ratecacheaccess_concat() {
  if (FOOTPRINT) {
    std::cout<<FOOTPRINT<<std::endl;
    std::vector<std::string> all_strings;
    read_file(FOOTPRINT,all_strings);
    std::cout<<all_strings.size()<<std::endl;
    for(uint64_t j = 0; j < all_strings.size(); j++) {
      std::string line = all_strings[j];
      //boost::trim_if(line, boost::is_any_of("\n"));
      std::vector<std::string> parsed;
      boost::split(parsed, line, boost::is_any_of(" "),boost::token_compress_on);
      // std::cout<<parsed.size()<<' '<<j<<std::endl;
      // std::cout<<j<<' '<<line<<std::endl;
      if(parsed.size()>4) {
        printf("Read error\n");
      }
      uint64_t addr = strtoul(parsed[0].c_str(), NULL, 10);
      uint64_t addr_end = strtoul(parsed[1].c_str(), NULL, 10);
      // uint64_t function_end = strtoul(parsed[1].c_str(), NULL, 10);
      uint64_t hit = strtoul(parsed[2].c_str(), NULL, 10);
      uint64_t count = strtoul(parsed[3].c_str(), NULL, 10);
      // std::cout<<function_start<<' '<<target<<std::endl;
      uint64_t updated_hit=0, updated_count=0;
      if (overall_hit_to_access_concat.count(make_pair(addr,addr_end))){
        updated_hit = overall_hit_to_access_concat[make_pair(addr,addr_end)].first;
        updated_count = overall_hit_to_access_concat[make_pair(addr,addr_end)].second;
      }
      updated_hit += hit;
      updated_count += count;
      overall_hit_to_access_concat[make_pair(addr,addr_end)]=make_pair(updated_hit,updated_count);
    }
    printf("Initializing footprint with size %u\n", overall_hit_to_access_concat.size());
    for(auto it = overall_hit_to_access_concat.begin(); it != overall_hit_to_access_concat.end(); ++it)
    {
        float rate = static_cast<float>((it->second).first)/(it->second).second;
        overall_hit_to_access_rate_concat[make_pair((it->first).first,(it->first).second)] = rate;
        if(PRINT_FUNC)  
        std::cout << (it->first).first << " " << (it->first).second << " " << (it->second).first << " " << (it->second).second << " " << rate << endl;
    }
    
    all_strings.clear();
  }
}

void write_ratecacheaccess_concat(Addr addr, Addr addr_end, uns hit, uns count) {
      uint64_t updated_hit=0, updated_count=0;
      if (overall_hit_to_access_concat.count(make_pair(addr,addr_end))){
        updated_hit = overall_hit_to_access_concat[make_pair(addr,addr_end)].first;
        updated_count = overall_hit_to_access_concat[make_pair(addr,addr_end)].second;
      }
      updated_hit += hit;
      updated_count += count;
      overall_hit_to_access_concat[make_pair(addr,addr_end)]=make_pair(updated_hit,updated_count);

    // printf("Initializing footprint with size %u\n", overall_hit_to_access_concat.size());
    
}

void write_countcacheaccess(Addr addr, uns64 count) {
  uint64_t updated_count=0;
  if (granu_overall_execution_count.count(addr)){
    updated_count = granu_overall_execution_count[addr];
  }
  updated_count += count;
  granu_overall_execution_count[addr]=updated_count;

// printf("Initializing footprint with size %u\n", granu_overall_execution_count.size());
}

void write_countcacheaccess_switch(Addr addr, uns64 count) {
  uint64_t updated_count=0;
  if (granu_overall_execution_count_switch.count(addr)){
    updated_count = granu_overall_execution_count_switch[addr];
  }
  updated_count += count;
  granu_overall_execution_count_switch[addr]=updated_count;

// printf("Initializing footprint with size %u\n", granu_overall_execution_count.size());
}

void minus_countcacheaccess(Addr addr, uns64 count) {
  uint64_t updated_count=0;
  if (granu_overall_execution_count.count(addr)){
    updated_count = granu_overall_execution_count[addr];
  }
  if (updated_count>0)
  updated_count -= count;
  granu_overall_execution_count[addr]=updated_count;

// printf("Initializing footprint with size %u\n", granu_overall_execution_count.size());
}

void write_countcacheaccess_concat(Addr addr, Addr addr_end, uns64 count) {
  uint64_t updated_count=0;
  if (overall_execution_count_concat.count(make_pair(addr,addr_end))){
    updated_count = overall_execution_count_concat[make_pair(addr,addr_end)];
  }
  updated_count += count;
  overall_execution_count_concat[make_pair(addr,addr_end)]=updated_count;

// printf("Initializing footprint with size %u\n", overall_execution_count_concat.size());
}

void write_miss(Addr addr, uns64 count) {
  uint64_t updated_count=0;
  if (overall_miss_count.count(addr)){
    updated_count = overall_miss_count[addr];
  }
  updated_count += count;
  overall_miss_count[addr]=updated_count;

// printf("Initializing footprint with size %u\n", overall_miss_count.size());
}

// if (overall_hit_to_access.count(addr)){
//         updated_hit = overall_hit_to_access[addr].first;
//         updated_count = overall_hit_to_access[addr].second;
//       }
//       updated_hit += hit;
//       updated_count += count;
//       overall_hit_to_access[addr]=make_pair(updated_hit,updated_count);
//     }
//     printf("Initializing footprint with size %u\n", overall_hit_to_access.size());
//     for(auto it = overall_hit_to_access.begin(); it != overall_hit_to_access.end(); ++it)
//     {
//         float rate = static_cast<float>((it->second).first)/(it->second).second;
//         overall_hit_to_access_rate[it->first] = rate;
//         if(PRINT_FUNC)  std::cout << it->first << " " << (it->second).first << " " << (it->second).second << " " << rate << endl;
//     }

void write_cycle(Addr addr, uns64 cycle, Flag reset_cycle) {
  uint64_t updated_cycle=0;
  if ((!overall_cycle_count.count(addr))||(overall_cycle_count[addr].first==0)){
    // updated_cycle = overall_cycle_count[addr];
    overall_cycle_count[addr].first = cycle;
  }
  if (reset_cycle)
  overall_cycle_count[addr].first = 0;
  else
  overall_cycle_count[addr].second = cycle;
  // updated_cycle = cycle;

  // printf("0x%s cycle is %llu %llu\n", hexstr64s(addr), overall_cycle_count[addr].first, overall_cycle_count[addr].second);
}

uns64 find_miss_count(Addr addr){
  if (overall_miss_count.count(addr)){
    return overall_miss_count[addr];
  } else return 0;
  // return overall_miss_count[addr];
}

uns64 find_cycle_count(Addr addr, Flag first){
  if (overall_cycle_count.count(addr)){
    if (first){
      return overall_cycle_count[addr].first;
    } else {
      return overall_cycle_count[addr].second;
    }
  } else return 0;
  // return overall_miss_count[addr];
}

uns64 find_entry_count(Addr addr){
  if (granu_overall_execution_count.count(addr)){
    return granu_overall_execution_count[addr];
  } else return 4; // 3bits for 8->4
}

uns64 find_entry_count_switch(Addr addr){
  if (granu_overall_execution_count_switch.count(addr)){
    return granu_overall_execution_count_switch[addr];
  } else return 0;
}

uns64 find_entry_count_concat(Addr addr, Addr addr_end){
  if (overall_execution_count_concat.count(make_pair(addr,addr_end))){
    return overall_execution_count_concat[make_pair(addr,addr_end)];
  } else return 1;
}

float find_entry_rate(Addr addr) {
  if(PRINT_FUNC)  printf("In find: %llu %f\n", addr, overall_hit_to_access_rate[addr]);
  if (overall_hit_to_access_rate.count(addr)){
    return overall_hit_to_access_rate[addr];
  } else return 0;
  // return (overall_hit_to_access_rate[addr]);
}

float find_entry_rate_transient_concat(Addr addr, Addr addr_end) {
  if(PRINT_FUNC)  printf("In find: %llu %f\n", addr, overall_hit_to_access_rate[addr]);
  if (overall_hit_to_access_concat.count(make_pair(addr,addr_end))){
    float rate = static_cast<float>(overall_hit_to_access_concat[make_pair(addr,addr_end)].first)/overall_hit_to_access_concat[make_pair(addr,addr_end)].second;
    return rate;//overall_execution_count_concat[make_pair(addr,addr_end)];
  } else return 0;//1;
  // return (overall_hit_to_access_rate[addr]);
}

float find_entry_rate_holistic_concat(Addr addr, Addr addr_end) {
  if(PRINT_FUNC)  printf("In find: %llu %f\n", addr, overall_hit_to_access_rate[addr]);
  if (overall_hit_to_access_rate_concat.count(make_pair(addr,addr_end))){
    return overall_hit_to_access_rate_concat[make_pair(addr,addr_end)];
  } else return 0;//1;
  // return (overall_hit_to_access_rate[addr]);
}

uns64 find_timestamp(Cache* cache, Addr addr){
  uns64 pre_timestamp = cache->pre_timestamp_new_opt;
  uns64 cur_timestamp;
  if (cl_decoded_entries.count(addr)){
    if(FALSE /*PRINT_FUNC*/) 
    {
      std::cout << addr << " size: " << cl_decoded_entries[addr].size() << endl;
      for (auto itt = cl_decoded_entries[addr].begin(); itt != cl_decoded_entries[addr].end(); ++itt)
          cout << ' ' << *itt;
      cout << endl;
    }
    auto it = cl_decoded_entries[addr].upper_bound(cache->pre_timestamp_new_opt);//last_evicted_cycle[addr]);
    if(FALSE /*PRINT_FUNC*/) cout << "Upper bound is " << (*it) << endl;
    cur_timestamp = (*it); 
    if(FALSE /*PRINT_FUNC*/) cout << "Middle: " << addr << " " << cur_timestamp << endl;
    if (cur_timestamp==*(cl_decoded_entries[addr].end())){
    // if (cur_timestamp>*(--cl_decoded_entries[addr].end())){
      cur_timestamp = pre_timestamp;//*(--cl_decoded_entries[addr].end()); //last_evicted_cycle[addr];
    }
    return cur_timestamp;
  } else return pre_timestamp;

}

uns64 find_future_timestamp(Cache* cache, Addr addr, uns64 cur_timestamp){
  uns64 read_addr, read_counter;
  uns64 nearest_timestamp=MAX_CTR;
  if (cl_decoded_entries.count(addr)){
    // for(const auto &kv: cl_decoded_entries[addr]) {
      if(PRINT_FUNC) cout << cl_decoded_entries[addr].size() << "\t";
      for (auto itt = cl_decoded_entries[addr].begin(); itt != cl_decoded_entries[addr].end(); ++itt)
          if(PRINT_FUNC) cout << ' ' << *itt;
      if(PRINT_FUNC) cout << endl;
      if (!last_evicted_cycle.count(addr)){
        if(PRINT_FUNC) cout << "Addr " << addr << " is missing in last_evicted_cycle"<< endl;
        last_evicted_cycle[addr] = *(cl_decoded_entries[addr]).begin();
      }
        if(PRINT_FUNC) cout << "Before: " << addr << " " << nearest_timestamp << " " << cur_timestamp << " " << last_evicted_cycle[addr] << endl;
        if (cur_timestamp<=last_evicted_cycle[addr]){
          nearest_timestamp = last_evicted_cycle[addr];
        } else {
          auto it = cl_decoded_entries[addr].upper_bound(cur_timestamp);//last_evicted_cycle[addr]);
          if(PRINT_FUNC) cout << "Upper bound is " << (*it) << endl;
          nearest_timestamp = (*it); 
          if(PRINT_FUNC) cout << "Middle: " << addr << " " << nearest_timestamp << " " << cur_timestamp << " " << last_evicted_cycle[addr] << endl;
          if (nearest_timestamp==*(cl_decoded_entries[addr].end())){
          // if (cur_timestamp>*(--cl_decoded_entries[addr].end())){
            nearest_timestamp = MAX_CTR;//cur_timestamp-*(--cl_decoded_entries[addr].end());//MAX_CTR;//*(--cl_decoded_entries[addr].end()); //last_evicted_cycle[addr];
          }
          if(PRINT_FUNC) cout << "Middle 2: " << addr << " " << nearest_timestamp << " " << cur_timestamp << " " << last_evicted_cycle[addr] << endl;
          if (nearest_timestamp<last_evicted_cycle[addr]){
            nearest_timestamp = last_evicted_cycle[addr];
            if(PRINT_FUNC) cout << "Middle 2.1: " << addr << " " << nearest_timestamp << " " << cur_timestamp << " " << last_evicted_cycle[addr] << endl;
          } else {
            last_evicted_cycle[addr] = nearest_timestamp;
            if(PRINT_FUNC) cout << "Middle 2.2: " << addr << " " << nearest_timestamp << " " << cur_timestamp << " " << last_evicted_cycle[addr] << endl;
          } 
        }
        if(PRINT_FUNC) cout << "After: " << addr << " " << nearest_timestamp << " " << cur_timestamp << " " << last_evicted_cycle[addr] << endl;
      // else {
      //   // if(PRINT_FUNC) cout << addr << " " << nearest_timestamp << " " << cur_timestamp  << " ";
      //   nearest_timestamp = *(cl_decoded_entries[addr]).begin();
      //   last_evicted_cycle[addr] = nearest_timestamp;
      //   if(PRINT_FUNC) cout << addr << " " << nearest_timestamp << " " << cur_timestamp  << " "  << last_evicted_cycle[addr] << endl;
      // }
    // }
  } else {
    nearest_timestamp = MAX_CTR;
    if (PRINT_FUNC)
    printf("%llu %llu, Read to end but not found\n", read_addr, read_counter);
  }
  
  return nearest_timestamp;
}

/******************************************************/
/***Value to Calculate unseen PW***/
unordered_map<pair<Addr, Counter>, Flag, boost::hash<pair<Addr, Counter>>> newPWCheck;
long long totalNewPWCount = 0;
long long uniqueNewPWCount = 0;

long long intervalCount = 0;
long long lastUniqueValue = 0;
long long lastTotalValue = 0;
long long totalUopCount = 0;

/*************************/
/* For Hit Times Repl*/
ifstream hitTimesFile;
unordered_map<pair<Addr, Counter>, Counter, boost::hash<pair<Addr, Counter>>> hitTimesMap;

void initHitTimesMap(){
  cout << "[Hittimes] Read traces from : hit.csv" << endl;
  hitTimesFile.open("hit.csv");
  if(!(hitTimesFile)){
    printf("Fail to Open the Hittimes traces\n");
    ASSERT(0,0);
  }
  Addr _addr;
  char occupy;
  Counter _uops, hitTime;
  int tmpCount = 0;
  while(hitTimesFile >> _addr >> occupy >> _uops >> occupy >> hitTime){
    tmpCount++;
    pair<Addr, Counter> uniqueKey = make_pair(_addr, _uops);
    if(hitTimesMap.count(uniqueKey) > 0){
      if(hitTime > hitTimesMap[uniqueKey]) hitTimesMap[uniqueKey] = hitTime;
    }else hitTimesMap[uniqueKey] = hitTime;
  }
  cout << "[HitTimes] Read " << tmpCount << " hitTimes value" << endl;
}

Counter getHitTimesValue(Addr _pwStartAddr, Counter _pwUopSize){
  pair<Addr, Counter> uniqueKey = make_pair(_pwStartAddr, _pwUopSize);
  if(hitTimesMap.count(uniqueKey) > 0){
    // printf("[HitTimes] %lld indeed in map: %d\n", _pwStartAddr, hitTimesMap[uniqueKey]);
    return hitTimesMap[uniqueKey];
  }else return MAX_CTR; // This means Evicted by LRU
}




/*************************/
/* For Bypass Bit*/
ifstream byPassFile;
unordered_map<Addr, Flag> byPassMap;
void initBypassMap(){
  cout << "[ByPass] Read traces from : bypass.out" << endl;
  byPassFile.open("bypass.out");
  if(!(byPassFile)){
    printf("Fail to Open the ByPass traces\n");
    ASSERT(0,0);
  }
  Addr _addr;
  int tmpCount = 0;
  while(byPassFile >> _addr){
    byPassMap[_addr] = TRUE;
    tmpCount++;
  }
  cout << "[ByPass] Read " << tmpCount << " bypass value" << endl;
}

Flag checkByPass(Addr startAddr, Counter _uops){
  if(byPassMap.count(startAddr) > 0) return TRUE;
  else return FALSE;
}

/*************************/

  class FOO_Core{
    public:
    unordered_map<pair<Addr, Counter>, Flag, boost::hash<pair<Addr, Counter>>> oracleStatus;
    ifstream traceFile;
    bool endOfTrace;

    Counter curPWOps;
    pair<Addr, Counter> curPW;
    bool curOracleFlag;
  };

  FOO_Core fooCore;

  /* Tool Function for reading next oracle line*/
  void readNextTraceLine(){
    Addr timeId, pwStartAddr;
    Counter entrySize, uopSize, hitFlag, insertFlag;
    char occupy;
    if(fooCore.traceFile >> timeId >> occupy >> 
        pwStartAddr >> occupy >> entrySize >> occupy >> uopSize
        >> occupy >> hitFlag >> occupy >> insertFlag){

      fooCore.curPWOps = uopSize;
      fooCore.curPW = make_pair(pwStartAddr, uopSize);
      fooCore.curOracleFlag = (insertFlag == 1) ? true : false;
    }else fooCore.endOfTrace = true;
  }

  void initFooCore(){
    cout << "[FOO] Read traces from : " << FOO_TRACE_PATH << endl;
    fooCore.traceFile.open(FOO_TRACE_PATH);
    if(!(fooCore.traceFile)){
      printf("Fail to Open the oracle FOO traces\n");
      ASSERT(0,0);
    }

    fooCore.endOfTrace = false;
    readNextTraceLine();
    ASSERT(0,!(fooCore.endOfTrace));
  }

  /* Called on when each op is packeted by packet_build.c */
  void updateFooPW(){
    /* Count New PW interval */
    intervalCount++;
    if(intervalCount > 1000000){
      // printf("**************\n");
      // printf("%lld, %lld\n", uniqueNewPWCount - lastUniqueValue, totalNewPWCount - lastTotalValue);
      // printf("%lld, %lld, %lld\n", uniqueNewPWCount, totalNewPWCount, totalUopCount);

      intervalCount = 0;
      lastUniqueValue = uniqueNewPWCount;
      lastTotalValue = totalNewPWCount;
    }
    /***********************/
    if(fooCore.endOfTrace){
      printf("[FOO] oracle file runs out\n");
      return;
    }
    ASSERT(0, fooCore.curPWOps > 0);
    --(fooCore.curPWOps);
    if(fooCore.curPWOps == 0){ // Update Oracle PW status
      (fooCore.oracleStatus)[fooCore.curPW] = (fooCore.curOracleFlag) ? TRUE : FALSE;
      readNextTraceLine();
    }else return;
  }

  Flag checkExistCache(Addr _pwStartAddr, Counter _pwUopSize){
    pair<Addr, Counter> toCheckPW = make_pair(_pwStartAddr, _pwUopSize);
    if((fooCore.oracleStatus).count(toCheckPW) > 0){
      return (fooCore.oracleStatus)[toCheckPW];
    }else{
      /* Here not seen in FOO offline traces. Undermine unseen PW*/
      if(newPWCheck.count(toCheckPW) == 0){
        uniqueNewPWCount++;
        newPWCheck[toCheckPW] = TRUE;
      }
      totalNewPWCount++;
      totalUopCount += _pwUopSize;
      /**************************************/
      return FALSE;
    }
  }

  /* Dynamic Evict */
  ifstream evictionFile;
  unordered_map<Addr, Counter> evictionMap;

  void initEvictionMap(){
    cout << "[Evict] Read traces from : evict.csv" << endl;
    evictionFile.open("evict.csv");
    if(!(evictionFile)){
      printf("Fail to Open the Evict traces\n");
      ASSERT(0,0);
    }
    Addr _addr;
    char occupy;
    Counter evictionCount;
    int tmpCount = 0;
    while(evictionFile >> _addr >> occupy >> evictionCount){
      evictionMap[_addr] = evictionCount;
      tmpCount++;
    }
    cout << "[Evict] Read " << tmpCount << " eviction hints value" << endl;
  }

  Counter getEvictionHints(Addr _pwStartAddr){
    if(evictionMap.count(_pwStartAddr) > 0) return evictionMap[_pwStartAddr];
    else return MAX_CTR;
  }

  /***Cold Misses***/
  unordered_map<Addr, Counter> coldSeeMap;
  Flag checkSeen(Addr _pwStartAddr){
    if(coldSeeMap.count(_pwStartAddr) > 0) return FALSE;
    else{
      coldSeeMap[_pwStartAddr] = 1;
      return TRUE;
    }
  }


  /* First for predTable implementation */
  int GHRP_counter_max;
  int GHRP_counter_min;
  int GHRP_entrySize;
  int** GHRP_tables;

  uint64_t globalHistory = 0;

  void initGHRP(){
    GHRP_counter_max = (1 << GHRP_COUNTER_WIDTH) - 1;
    GHRP_counter_min = -GHRP_counter_max;
    GHRP_entrySize = (1 << GHRP_PREDICT_TABLE_INDEX_BITS);
    GHRP_tables = new int*[GHRP_PREDICT_TABLE_NUM];
    for(int i = 0; i < GHRP_PREDICT_TABLE_NUM; i++){
      GHRP_tables[i] = new int[GHRP_entrySize];
      for(int j = 0; j < GHRP_entrySize; j++){
        GHRP_tables[i][j] = 0;
      }
    }
  }

  Flag GHRP_get_prediction(uint64_t* indexes){
    int deadNum = 0;
    for(int i = 0; i < GHRP_PREDICT_TABLE_NUM; i++){
      if(GHRP_tables[i][indexes[i]] >= GHRP_THRESHOLD) deadNum++;
    }
    return (deadNum > (GHRP_PREDICT_TABLE_NUM >> 1)) ? TRUE : FALSE;
  }

  void GHRP_update_predict_table(uint64_t* indexes, Flag dead){
    for(int i = 0; i < GHRP_PREDICT_TABLE_NUM; i++){
      if(dead == TRUE){
        if(GHRP_tables[i][indexes[i]] < GHRP_counter_max) GHRP_tables[i][indexes[i]]++;
      }else{
        if(GHRP_tables[i][indexes[i]] > GHRP_counter_min) GHRP_tables[i][indexes[i]]--;
      }
    }
  }

  uint64_t* GHRP_get_pred_indexes(uint64_t signature){
    uint64_t* indexes = new uint64_t[GHRP_PREDICT_TABLE_NUM];
    for(int i = 0; i < GHRP_PREDICT_TABLE_NUM; i++){
      indexes[i] = GHRP_sign_hash(signature, i) & ((1 << GHRP_PREDICT_TABLE_INDEX_BITS) - 1);
    }
    return indexes;
  }

  void GHRP_free_pred_indexes(uint64_t* indexes){
    delete[] indexes;
  }

  uint64_t GHRP_mix(uint64_t a, uint64_t b, uint64_t c){
    a -= b; a -= c; a ^= (c>>13);
    b -= c; b -= a; b ^= (a<<8);
    c -= a; c -= b; c ^= (b>>13);
    a -= b; a -= c; a ^= (c>>12);
    b -= c; b -= a; b ^= (a<<16);
    c -= a; c -= b; c ^= (b>>5);
    a -= b; a -= c; a ^= (c>>3);
    b -= c; b -= a; b ^= (a<<10);
    c -= a; c -= b; c ^= (b>>15);
	  return c;
  }

  uint64_t GHRP_sign_hash(uint64_t sign, int index) {
    uint64_t magicNumber[GHRP_PREDICT_TABLE_NUM << 2] = {
                                                          0xfeedface, 0xdeadb10c, 0xc001d00d, 0xfade2b1c, 
                                                          0xabcdefed, 0x12e56e78, 0x87520321, 0xdeadbeef,
                                                          0xefedf00d, 0x1314babe, 0xfaceb00c, 0x1d00d1e5
                                                        };
    uint64_t ind = GHRP_mix(magicNumber[(index << 2) + 0], magicNumber[(index << 2) + 1], sign) + 
                    GHRP_mix(magicNumber[(index << 2) + 2], magicNumber[(index << 2) + 3], sign);
    return ind;
  }

  uint64_t GHRP_get_signature(uint64_t _globalHistory, Addr pc){
    uint64_t signature = (uint64_t) pc ^ _globalHistory;
    return signature & ((1 << GHRP_SIGNATURE_BITS) - 1);
  }

  void GHRP_update_global_history(Addr pc){
    globalHistory = globalHistory << 4;
    pc = (pc >> 2) & ((1 << 3) - 1); /* 4 is for shift bits */
    globalHistory = (globalHistory | pc) & ((1 << GHRP_GLOBAL_HISTORY_BITS) - 1);
  }

  /* Below are for Thermometer */

  class Thermo_Core{
    private:
    ifstream traceFile;
    unordered_map<Addr, Counter> thermoMap;

    public:
    Thermo_Core(){}
    void init_core(){
      cout << "[Thermo] Read traces from : thermo.csv" << endl;
      traceFile.open("thermo.csv");
      if(!(traceFile)){
        printf("Fail to Open the Thermo traces\n");
        ASSERT(0,0);
      }
      Addr _addr;
      Counter _weight;
      int tmpCount = 0;
      while(traceFile >> _addr >> _weight){
        thermoMap[_addr] = _weight;
        tmpCount++;
      }
      cout << "[Thermo] Read " << tmpCount << " thermo value" << endl;
    }
    Counter getThermoWeight(Addr _addr){
      if(thermoMap.count(_addr) > 0) return thermoMap[_addr];
      else return 0;
    }
  };

  Thermo_Core thermoCore;
  void init_thermo_core(){
    thermoCore.init_core();
  }

  Counter get_thermo_weight(Addr _addr){
    return thermoCore.getThermoWeight(_addr);
  }


  /* Below are PMC of IC Cache Replacement Policy */

  class IC_PMC_Core{
    private:
    ifstream traceFile;
    unordered_map<Addr, Counter> thermoMap;

    public:
    IC_PMC_Core(){}
    void init_core(){
      cout << "[IC PMC] Read traces from : IC_PMC.csv" << endl;
      traceFile.open("IC_PMC.csv");
      if(!(traceFile)){
        printf("Fail to Open the Thermo traces\n");
        ASSERT(0,0);
      }
      Addr _addr;
      Counter _weight;
      int tmpCount = 0;
      while(traceFile >> _addr >> _weight){
        thermoMap[_addr] = _weight;
        tmpCount++;
      }
      cout << "[IC PMC] Read " << tmpCount << " PMC value" << endl;
    }
    Counter getWeight(Addr _addr){
      if(thermoMap.count(_addr) > 0) return thermoMap[_addr];
      else return 0;
    }
  };

  IC_PMC_Core IC_Core;
  void init_ic_pmc_core(){
    IC_Core.init_core();
  }

  Counter get_ic_pmc_weight(Addr _addr){
    return IC_Core.getWeight(_addr);
  }