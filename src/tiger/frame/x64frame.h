//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

extern frame::RegManager *reg_manager;

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

    temp::Temp* Rdx() override {
      return regs_[3];
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
      temp::TempList *res = new temp::TempList({regs_[0]});
      res->Append(regs_[10]);
      res->Append(regs_[11]);
      res->Append(regs_[5]);
      res->Append(regs_[4]);
      res->Append(regs_[3]);
      res->Append(regs_[2]);
      res->Append(regs_[8]);
      res->Append(regs_[9]);
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

    temp::TempList* AllRegisters() override{
      temp::TempList* res = new temp::TempList();
      for(auto reg : regs_){
        res->Append(reg);
      }
      return res;
    }

    std::string* getCoreString(int idx) override{
      assert(idx < 15);
      if(idx < 7){
        return (temp_map_->Look(regs_.at(idx)));
      } else {
        return (temp_map_->Look(regs_.at(idx + 1)));
      }
    }


};

class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *ToExp(tree::Exp *frame_ptr) const override{
    // compute for the frame pos
    // TODO : there may left some problem : should we deal with the offset equal to zero
    tree::Exp* off_exp = new tree::ConstExp(offset);
    tree::Exp* pos = new tree::BinopExp(tree::BinOp::MINUS_OP,frame_ptr,off_exp);
    return new tree::MemExp(pos);
  }

};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }

};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */

public:
  // need to do the view shift code generation and 
  X64Frame(temp::Label* fun_name,std::list<bool> formals) : Frame(fun_name,formals)
  {
  // code for code generation , such as put the escape variable to the stack
  // 1, do some escape frame saving
  // 2, save callee saved registers
  // 3, restore callee saved registers

  // init the formals of frame object
  for(auto for_bu : formals){
    frame::Access* a_for = this->allocLocal(for_bu);
    this->formals_->push_back(a_for);
  }


  int counter = 0;
  std::list<temp::Temp*> temp_list = reg_manager->ArgRegs()->GetList();
  auto iter = temp_list.begin();
  auto iter_fa = formals_->begin();
  auto frame_pointer = new tree::TempExp(reg_manager->FramePointer());
  for(auto formal : formals){

    if(counter <= 5){
      this->view_shift->push_back(new tree::MoveStm(
        (*iter_fa)->ToExp(frame_pointer)
        ,new tree::TempExp(*iter)));
    }
    if(counter >= 6){
      // only 6 registers
      this->view_shift->push_back(new tree::MoveStm(
        (*iter_fa)->ToExp(frame_pointer)
        ,new tree::MemExp(
              new tree::BinopExp(
                tree::BinOp::PLUS_OP,
                frame_pointer,
                new tree::ConstExp(reg_manager->WordSize() * (counter - 5))
              )
            )));
    }
    iter++;
    iter_fa++;
    counter++;
  }

  // // save callee saved registers
  // std::list<temp::Temp*> save_regs = reg_manager->CalleeSaves()->GetList();
  // std::list<frame::Access*> acc_list; 
  // temp::TempList* save_reg = new temp::TempList();
  // for(auto reg : save_regs){
  //   frame::Access* new_acc = this->allocLocal(false);
  //   save_reg->Append(static_cast<frame::InRegAccess*>(new_acc)->reg);
  //   this->view_shift->push_back(new tree::MoveStm(new tree::TempExp(static_cast<frame::InRegAccess*>(new_acc)->reg),new tree::TempExp(reg)));
  // }

  // // restore callee saved registers
  // auto idx = acc_list.begin();
  // auto iter_ = save_reg->GetList().begin();
  // for(auto reg : save_regs){
  //   this->view_shift->push_back(new tree::MoveStm(new tree::TempExp(reg),new tree::TempExp(*iter_)));
  //   iter_++;
  // }
}

  // assigned a new var
  frame::Access* allocLocal(bool escape) override{
    // generate core access according to the escape
    if(escape){
      // add the use of frame
      this->frame_size += 8;
      // allocate on Frame
      frame::Access *new_access = new frame::InFrameAccess(this->frame_size);
      if(!new_access){
        printf("what wrong\n");
      }
      return new_access;
    } else {
      // alloc on register
      temp::Temp* new_temp = temp::TempFactory::NewTemp();
      frame::Access *new_access = new frame::InRegAccess(new_temp);
      if(!new_access){
        printf("what wrong\n");
      }
      return new_access;
    }
  };

  ~X64Frame(){};

};
/* TODO: Put your lab5 code here */

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
