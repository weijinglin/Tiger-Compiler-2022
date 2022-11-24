#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */

  tree::Exp *ToExp(tree::Exp *frame_ptr) const override{
    // compute for the frame pos
    // TODO : there may left some problem : should we deal with the offset equal to zero
    tree::Exp* off_exp = new tree::ConstExp(offset);
    tree::Exp* pos = new tree::BinopExp(tree::BinOp::PLUS_OP,frame_ptr,off_exp);
    return new tree::MemExp(pos);
  }

};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    printf("call toExp num is %d\n",reg->Int());
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

  // save callee saved registers
  // std::list<temp::Temp*> save_regs = reg_manager->CalleeSaves()->GetList();
  // std::list<frame::Access*> acc_list; 
  // for(auto reg : save_regs){
  //   frame::Access* new_acc = this->allocLocal(true);
  //   acc_list.push_back(new_acc);
  //   this->view_shift->push_back(new tree::MoveStm(new_acc->ToExp(
  //     new tree::TempExp(reg_manager->FramePointer())
  //   ),new tree::TempExp(reg)));
  // }

  // restore callee saved registers
  // auto idx = acc_list.begin();
  // for(auto reg : save_regs){
  //   this->view_shift->push_back(new tree::MoveStm(new tree::TempExp(reg),
  //     (*idx)->ToExp(new tree::TempExp(reg_manager->FramePointer()))
  //   ));
  // }
}

  // assigned a new var
  frame::Access* allocLocal(bool escape) override{
    // generate core access according to the escape
    if(escape){
      // allocate on Frame
      frame::Access *new_access = new frame::InFrameAccess(this->frame_size);
      // add the use of frame
      this->frame_size += 8;
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
