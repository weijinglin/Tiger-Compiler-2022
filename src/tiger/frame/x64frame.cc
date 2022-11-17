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
    return new tree::TempExp(reg);
  }

};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */

public:
  // need to do the view shift code generation and 
  X64Frame(temp::Label* fun_name,std::list<bool> formals);

  // assigned a new var
  frame::Access* allocLocal(bool escape) override;

  ~X64Frame();

};
/* TODO: Put your lab5 code here */

X64Frame::X64Frame(temp::Label* fun_name,std::list<bool> formals) : Frame(fun_name,formals)
{
  // code for code generation , such as put the escape variable to the stack

}

X64Frame::~X64Frame(){

}

} // namespace frame