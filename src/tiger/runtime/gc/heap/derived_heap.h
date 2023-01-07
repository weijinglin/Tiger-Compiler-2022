#pragma once

#include "heap.h"
#include <vector>
#include <iostream>

#include <sstream>


#include "../roots/roots.h"

namespace gc {

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
  public:

  struct free_block
  {
    /* data */
    int start_pos;
    int end_pos;
  };

  struct allocated_block
  {
    /* data */
    int start_pos;
    int end_pos;
    bool is_mark;
  };

  /**
   * Allocate a contiguous space from heap.
   * If your heap has enough space, just allocate and return.
   * If fails, return nullptr.
   * @param size size of the space allocated, in bytes.
   * @return start address of allocated space, if fail, return nullptr.
   */
  char *Allocate(uint64_t size) override {
    // trace the block(use the first fit strategy)
    auto iter = this->free_list.begin();
    while(true){
      if(iter == this->free_list.end()){
        break;
      }
      int block_size = (*iter).end_pos - (*iter).start_pos + 1;
      if(block_size > size){
        allocated_block new_al_block;
        new_al_block.start_pos = (*iter).start_pos;
        new_al_block.end_pos = new_al_block.start_pos + size - 1;
        new_al_block.is_mark = false;
        this->al_list.push_back(new_al_block);

        // update the free_list
        (*iter).start_pos += size;

        return this->heap + (*iter).start_pos;
      }
      else if(block_size == size){
        allocated_block new_al_block;
        new_al_block.start_pos = (*iter).start_pos;
        new_al_block.end_pos = new_al_block.start_pos + size - 1;
        new_al_block.is_mark = false;
        this->al_list.push_back(new_al_block);

        // delete this block
        // std::vector<free_block> free_copy;
        // for(auto a_block : this->free_list){
        //   if(a_block.start_pos == (*iter).start_pos){

        //   } else {
        //     free_copy.push_back(a_block);
        //   }
        // }

        // this->free_list = free_copy;
        DeleteFree(*iter);

        return this->heap + (*iter).start_pos;
      }

      iter++;
    }

    // do some trick to prove my code
    // return new char[size];
    return nullptr;
  }

  void DeleteFree(free_block block){
    // delete this block
    std::vector<free_block> free_copy;
    for(auto a_block : this->free_list){
      if(a_block.start_pos == block.start_pos){

      } else {
        free_copy.push_back(a_block);
      }
    }

    this->free_list = free_copy;
  }

  /**
   * Acquire the total allocated space from heap.
   * Hint: If you implement a contigous heap, you could simply calculate the distance between top and bottom,
   * but if you implement in a link-list way, you may need to sum the whole free list and calculate the total free space.
   * We will evaluate your work through this, make sure you implement it correctly.
   * @return total size of the allocated space, in bytes.
   */
  uint64_t Used() const override
  {
    uint64_t unused_size = 0;
    for(auto free_ : this->free_list){
      unused_size += free_.end_pos - free_.start_pos + 1;
    }
    // TODO : may be buggy for the overflow
    return this->heap_size - unused_size;
  }

  /**
   * Acquire the longest contiguous space from heap.
   * It should at least be 50% of inital heap size when initalization.
   * However, this may be tricky when you try to implement a Generatioal-GC, it depends on how you set the ratio
   * between Young Gen and Old Gen, we suggest you making sure Old Gen is larger than Young Gen.
   * Hint: If you implement a contigous heap, you could simply calculate the distance between top and end,
   * if you implement in a link-list way, you may need to find the biggest free list and return it.
   * We use this to evaluate your utilzation, make sure you implement it correctly.
   * @return size of the longest , in bytes.
   */
  uint64_t MaxFree() const override
  {
    int free_size = 0;
    for(auto free_ : this->free_list){
      int size = free_.end_pos - free_.start_pos + 1;
      if(size > free_size){
        free_size = size;
      }
    }
    // TODO : may be buggy for the overflow
    return free_size;
  }

  /**
   * Initialize your heap.
   * @param size size of your heap in bytes.
   */
  void Initialize(uint64_t size) override{
    if(size == 0){
      printf("heap size can't be zero\n");
      return;
    }
    this->heap = new char[size];
    this->heap_size = size;
    free_block big_free_block;
    big_free_block.start_pos = 0;
    big_free_block.end_pos = size-1;
    this->free_list.push_back(big_free_block);
  }

  /**
   * Do Garbage collection!
   * Hint: Though we do not suggest you implementing a Generational-GC due to limited time,
   * if you are willing to try it, add a function named YoungGC, this will be treated as FullGC by default.
   */
  void GC() override{
    roots.genMaps();
    std::vector<ptrMap> all_map = roots.getMaps();

    // get the first return address
    uint64_t* rsp;
    GET_TIGER_STACK(rsp);

    uint64_t fir_ret = *rsp;
    std::string mes = "";
    uint64_t frame_size = 0;
    // find core ptrMap
    for(auto map : all_map){
      if(map.ret_label == fir_ret){
        frame_size = map.frame_size;
        mes += map.map_mes;
        break;
      }
    }

    auto rbp = rsp + frame_size / 8;

    std::istringstream iss(mes);
    std::string token;
    while (std::getline(iss, token, '/'))
    {
      int offset = str2Int(token);
      printf("parse the token is %d\n",offset);
      
      // TODO(wjl) : here is some code test for the reading from the stack
      auto ptr_var = (uint64_t*)(rbp - (uint64_t)offset / 8);
      DFS(ptr_var);
      // auto val_ptr = (uint64_t*)(*ptr_var);
      // printf("the address of array is %ld and val is %ld\n",(*ptr_var),(*val_ptr));
    }

    // move up and find the up stack
    while (true)
    {
      /* code */
      rsp = rbp;
      uint64_t fir_ret = *rsp;
      std::string mes = "";
      uint64_t frame_size = 0;

      bool is_hit = false;

      // find core ptrMap
      for(auto map : all_map){
        if(map.ret_label == fir_ret){
          frame_size = map.frame_size;
          mes += map.map_mes;
          is_hit = true;
          break;
        }
      }

      // the upmost function's RA is not recorded in ptrMap
      if(!is_hit){
        break;
      }

      auto rbp = rsp + frame_size / 8;

      std::istringstream iss(mes);
      std::string token;
      while (std::getline(iss, token, '/'))
      {
        int offset = str2Int(token);
        printf("parse the token is %d\n",offset);
        
        // TODO(wjl) : here is some code test for the reading from the stack
        auto ptr_var = (uint64_t*)(rbp - (uint64_t)offset / 8);
        DFS(ptr_var);
        // auto val_ptr = (uint64_t*)(*ptr_var);
        // printf("the address of array is %ld and val is %ld\n",(*ptr_var),(*val_ptr));
      }
    }

    Clean();
  }

  void Clean(){
    for(auto block : this->al_list){
      if(block.is_mark == false){
        AddFree(block.start_pos,block.end_pos);
      }
    }
  }

  void AddFree(int start_pos,int end_pos){
    int most_start = start_pos;
    int most_end = end_pos;
    // find which free_block's end_pos = start_pos
    for(auto block : this->free_list){
      if(block.end_pos == start_pos){
        most_start = block.start_pos;
        DeleteFree(block);
        break;
      }
    }

    // find which free_block's start_pos = end_pos
    for(auto block : this->free_list){
      if(block.start_pos == end_pos){
        most_end = block.end_pos;
        DeleteFree(block);
        break;
      }
    }

    free_block new_free;
    new_free.start_pos = most_start;
    new_free.end_pos = most_end;
    this->free_list.push_back(new_free);
  }

  int str2Int(std::string token){
    std::stringstream stream; 
    int n;
    stream << token; 
    stream >> n; 
    return n;
  }

  void DFS(uint64_t* root){
    // do mark job in the root
    // check is a array or record(use different strategy)
    // use first 5 char to recognize
    char **buf = (char**)(*root);
    char* dep_word = (*buf) + 4;

    // do mark
    auto iter = this->al_list.begin();
    while(true){
      if(iter == this->al_list.end()){
        printf("computer err in mark\n");
        break;
      }
      if((*iter).start_pos + this->heap == (char*)(*root)){
        if((*iter).is_mark == true){
          // have been marked
          return;
        }
        (*iter).is_mark = true;
        break;
      }

      iter++;
    }

    // TODO : do recognizing
    if(dep_word[0] != 'r'){
      return;
    }
    std::string dep_str = dep_word;
    printf("first field has %s\n",dep_str.c_str());

    // pass the record test
    int str_size = dep_str.size();
    int loop_time = str_size - 7;
    for(int i = 0;i < loop_time;++i){
      if(dep_str.at(i + 7) == '1'){
        uint64_t* new_root = (uint64_t*)(root + i + 1);
        DFS(new_root);
      }
    }
  }

  // TODO : define some useful struct
  // the allocate buffer
  char* heap;

  // buf size
  unsigned int heap_size;

  // used to find Root in stack
  gc::Roots roots;
  
  // free list that store the free block
  std::vector<free_block> free_list;
  std::vector<allocated_block> al_list;
    
};

} // namespace gc

