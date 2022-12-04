//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
  
  public:
    X64RegManager(){
      for(int i = 0;i < 17;++i){
        temp::Temp *reg = temp::TempFactory::NewTemp();
        this->regs_.push_back(reg);
      }

      // map to the core register
      this->temp_map_->Enter(regs_[0],new std::string("%rax"));
      this->temp_map_->Enter(regs_[1],new std::string("%rbx"));
      this->temp_map_->Enter(regs_[2],new std::string("%rcx"));
      this->temp_map_->Enter(regs_[3],new std::string("%rdx"));
      this->temp_map_->Enter(regs_[4],new std::string("%rsi"));
      this->temp_map_->Enter(regs_[5],new std::string("%rdi"));
      this->temp_map_->Enter(regs_[6],new std::string("%rbp"));
      this->temp_map_->Enter(regs_[7],new std::string("%rsp"));
      this->temp_map_->Enter(regs_[8],new std::string("%r8"));
      this->temp_map_->Enter(regs_[9],new std::string("%r9"));
      this->temp_map_->Enter(regs_[10],new std::string("%r10"));
      this->temp_map_->Enter(regs_[11],new std::string("%r11"));
      this->temp_map_->Enter(regs_[12],new std::string("%r12"));
      this->temp_map_->Enter(regs_[13],new std::string("%r13"));
      this->temp_map_->Enter(regs_[14],new std::string("%r14"));
      this->temp_map_->Enter(regs_[15],new std::string("%r15"));
      this->temp_map_->Enter(regs_[16],new std::string("%rip"));
    }
    /**
       * Get general-purpose registers except RSI
       * NOTE: returned temp list should be in the order of calling convention
       * @return general-purpose registers
       */
    temp::TempList *Registers() override{
      temp::TempList* res = new temp::TempList({regs_[0]});
      for(int i = 1;i < 4;++i){
        res->Append(regs_[i]);
      }
      for(int i = 5;i < 17;++i){
        res->Append(regs_[i]);
      }
      return res;
    }
    /**
     * Get registers which can be used to hold arguments
     * NOTE: returned temp list must be in the order of calling convention
     * @return argument registers
     */
    temp::TempList *ArgRegs() override{
      // TODO(wjl) : here return the register from rdi to r9
      temp::TempList* res = new temp::TempList({regs_[5]});
      for(int i = 4;i > 1;--i){
        res->Append(regs_[i]);
      }
      res->Append(regs_[8]);
      res->Append(regs_[9]);
      return res;
    }
    /**
     * Get caller-saved registers
     * NOTE: returned registers must be in the order of calling convention
     * @return caller-saved registers
     */
    temp::TempList *CallerSaves() override{
      temp::TempList *res = new temp::TempList({regs_[10]});
      res->Append(regs_[11]);
      return res;
    }
    temp::TempList *CalleeSaves() override{
      temp::TempList* res = new temp::TempList({regs_[1]});
      res->Append(regs_[6]);
      for(int i = 12;i < 16;++i){
        res->Append(regs_[i]);
      }
      return res;
    }
    temp::TempList *ReturnSink() override{
      temp::TempList *temp_list = CalleeSaves();
      temp_list->Append(StackPointer());
      temp_list->Append(ReturnValue());
      return temp_list;
    }
    int WordSize() override{
      // TODO(wjl) : assume the size of a word is 8 bytes
      return 8;
    }
    temp::Temp *FramePointer() override{
      return regs_[6];
    }
    temp::Temp *StackPointer() override{
      return regs_[7];
    }
    temp::Temp *ReturnValue() override{
      return regs_[0];
    }


};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
