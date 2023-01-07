#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <iostream>

#include <vector>

// this begin of the list
extern uint64_t GLOBAL_GC_ROOTS;

namespace gc {

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

struct ptrMap
{
  /* data */
  std::string map_mes;
  uint64_t frame_size;
  
  // this long* represent for next label and we will use a long* to parse a ptrMap
  // uint64_t* next_label;
  // use long to record the address of return label is efficient for comparing
  uint64_t ret_label;
};

class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
  public:
    std::vector<ptrMap> maps;

    void genMaps(){
      // get zero label(head point of list)
      uint64_t *zero_label = (uint64_t*)(&GLOBAL_GC_ROOTS);

      uint64_t* curr_label = (uint64_t*)(*zero_label);
      while (true)
      {
        /* code */
        ptrMap new_map;
        // new_map.next_label = (uint64_t*)(*curr_label);
        auto ret_ptr = curr_label + 1;
        new_map.ret_label = (*ret_ptr);
        auto size_ptr = ret_ptr + 1;
        new_map.frame_size = (*size_ptr);
        char* buf = (char*)(size_ptr + 1);
        new_map.map_mes = buf;

        printf("in root genMaps test for map_mes is %s\n",buf);

        this->maps.push_back(new_map);

        curr_label = (uint64_t*)(*curr_label);
        if(!curr_label){
          break;
        }
      }
    }

    std::vector<ptrMap> getMaps(){
      return maps;
    }
};

}

#endif // TIGER_RUNTIME_GC_ROOTS_H