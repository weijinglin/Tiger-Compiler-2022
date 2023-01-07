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
        // update the free_list
        (*iter).start_pos += size;

        return this->heap + (*iter).start_pos;
      }
      else if(block_size == size){
        // delete this block
        std::vector<free_block> free_copy;
        for(auto a_block : this->free_list){
          if(a_block.start_pos == (*iter).start_pos){

          } else {
            free_copy.push_back(a_block);
          }
        }

        this->free_list = free_copy;

        return this->heap + (*iter).start_pos;
      }

      iter++;
    }

    // do some trick to prove my code
    // return new char[size];
    return nullptr;
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

    std::istringstream iss(mes);	// 输入流
    std::string token;			// 接收缓冲区
    while (std::getline(iss, token, '/'))	// 以split为分隔符
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

  int str2Int(std::string token){
    std::stringstream stream;     //声明一个stringstream变量
    int n;
    //string转int
    stream << token;     //向stream中插入字符串"1234"
    stream >> n; 
    return n;
  }

  void DFS(uint64_t* root){
    // do mark job in the root
    // check is a array or record(use different strategy)
    // use first 5 char to recognize
    char **buf = (char**)(*root);
    char* dep_word = (*buf) + 4;
    // TODO : test code
    printf("can reach here\n");
    printf("address is %d\n",dep_word);
    if(dep_word[0] != 'r'){
      printf("normal return\n");
      return;
    }
    printf("can reach here\n");
    std::string dep_str = dep_word;
    printf("first field has %s\n",dep_str.c_str());

  }

  // TODO : define some useful struct
  // the allocate buffer
  char* heap;

  // buf size
  unsigned int heap_size;

  // used to find Root in stack
  gc::Roots roots;

  struct free_block
  {
    /* data */
    int start_pos;
    int end_pos;
  };

  // free list that store the free block
  std::vector<free_block> free_list;
  
};

} // namespace gc

