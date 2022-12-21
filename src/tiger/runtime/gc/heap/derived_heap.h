#pragma once

#include "heap.h"
#include <vector>

namespace gc {

class DerivedHeap : public TigerHeap {
  // // TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
  // public:
  // /**
  //  * Allocate a contiguous space from heap.
  //  * If your heap has enough space, just allocate and return.
  //  * If fails, return nullptr.
  //  * @param size size of the space allocated, in bytes.
  //  * @return start address of allocated space, if fail, return nullptr.
  //  */
  // char *Allocate(uint64_t size) override;

  // /**
  //  * Acquire the total allocated space from heap.
  //  * Hint: If you implement a contigous heap, you could simply calculate the distance between top and bottom,
  //  * but if you implement in a link-list way, you may need to sum the whole free list and calculate the total free space.
  //  * We will evaluate your work through this, make sure you implement it correctly.
  //  * @return total size of the allocated space, in bytes.
  //  */
  // uint64_t Used() const override
  // {

  // }

  // /**
  //  * Acquire the longest contiguous space from heap.
  //  * It should at least be 50% of inital heap size when initalization.
  //  * However, this may be tricky when you try to implement a Generatioal-GC, it depends on how you set the ratio
  //  * between Young Gen and Old Gen, we suggest you making sure Old Gen is larger than Young Gen.
  //  * Hint: If you implement a contigous heap, you could simply calculate the distance between top and end,
  //  * if you implement in a link-list way, you may need to find the biggest free list and return it.
  //  * We use this to evaluate your utilzation, make sure you implement it correctly.
  //  * @return size of the longest , in bytes.
  //  */
  // uint64_t MaxFree() const override
  // {

  // }

  // /**
  //  * Initialize your heap.
  //  * @param size size of your heap in bytes.
  //  */
  // void Initialize(uint64_t size) override;

  // /**
  //  * Do Garbage collection!
  //  * Hint: Though we do not suggest you implementing a Generational-GC due to limited time,
  //  * if you are willing to try it, add a function named YoungGC, this will be treated as FullGC by default.
  //  */
  // void GC() override;

  // static constexpr uint64_t WORD_SIZE = 8;
};

} // namespace gc

