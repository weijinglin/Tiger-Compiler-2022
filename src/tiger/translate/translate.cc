#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

// may return RegisAccess and FrameAccess
Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */

  frame::Access* new_access = level->frame_->allocLocal(escape);
  return new tr::Access(level,new_access);
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return this->exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(this->exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    tr::PatchList *trues = new tr::PatchList();
    tr::PatchList *falses = new tr::PatchList();
    Cx* res =  new tr::Cx(*trues,*falses,this->UnNx());
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(this->stm_,new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return this->stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    errormsg->Error(errormsg->GetTokPos(),"there should not be the kind of convert\n");
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *true_label = temp::LabelFactory::NewLabel();
    temp::Label *false_label = temp::LabelFactory::NewLabel();
    this->cx_.trues_.DoPatch(true_label);
    this->cx_.falses_.DoPatch(false_label);

    // attention : TempExp represent for the value of the register
    tree::Stm* ass_res = new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(1));

    return new tree::EseqExp(ass_res,new tree::EseqExp(
      this->cx_.stm_,new tree::EseqExp(
        new tree::LabelStm(false_label),new tree::EseqExp(
          new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(0)),
          new tree::EseqExp(new tree::LabelStm(true_label),new tree::TempExp(r))
        )
      )
    ));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return this->cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return this->cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  // translate from the root
  this->absyn_tree_.get()->Translate((this->venv_.get()),
  (this->tenv_.get()),this->main_level_.get(),
  this->main_level_.get()->frame_->fun_label,this->errormsg_.get());
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* res =  this->root_->Translate(venv,tenv,level,label,errormsg);
  frags->PushBack(new frame::ProcFrag(res->exp_->UnNx(),level->frame_));
}

// used to compute for the static link
tree::Exp* static_link_com(tr::Level *def_level,tr::Level *use_level){
  // it's needed only use_level is in the def_level
  // first , get the FP of the use_level
  temp::Temp *frame_po = reg_manager->FramePointer();
  if(def_level == use_level){
    return new tree::TempExp(frame_po);
  }
  // use the static algo
  // get the first FP to make the iteration can loop
  tree::Exp* iter = new tree::TempExp(frame_po);
  tr::Level* now_level = use_level;
  while(def_level != now_level){
    // TODO : there may be some error because of the frame_size is known , so is it necessary to store the fp of last level ?
    // here we store the fp of last level
    // can use the offset in the Access if thee fpp is stored in the formal
    iter = use_level->frame_->formals_->front()->ToExp(iter);
    if(!now_level){
      printf("what ? seg in static link\n");
    }
    now_level = now_level->parent_;
  }
  return iter;
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  
  // get the core var by access the venv
  env::EnvEntry* sim_var = venv->Look(this->sym_);

  // check for escape
  if((dynamic_cast< env::VarEntry* >( sim_var )) != nullptr){
    // check for the type
    // get the tr::Access
    tr::Access *sim_access = static_cast< env::VarEntry* >( sim_var )->access_;
    if(!sim_access){
      printf("what ? seg in SimpleVar\n");
      return;
    } else {
      // compute the correct static link
      tree::Exp *static_link = static_link_com(sim_access->level_,level);
      return new tr::ExpAndTy(new tr::ExExp(sim_access->access_->ToExp(static_link)),static_cast< env::VarEntry* >( sim_var )->ty_);
    }
  }

}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // get the core var by access the venv
  tr::ExpAndTy* mid_res = this->var_->Translate(venv,tenv,level,label,errormsg);
  // use type to compute the offset of each field
  type::Ty* record_ty = mid_res->ty_;
  // TODO(wjl): there may be buggy because of the type convert
  type::RecordTy* con_record_ty = static_cast<type::RecordTy*>(record_ty);
  std::list<type::Field *> fie_list = con_record_ty->fields_->GetList();
  int counter = 1;
  bool is_find = false;
  type::Ty* last_ty;
  for(auto fie : fie_list){
    if(fie->name_ == this->sym_){
      is_find = true;
      last_ty = fie->ty_;
      break;
    }
    counter++;
  }
  
  if(!is_find){
    printf("what , field not find ?\n");
  }

  // construct the offset
  tree::BinopExp *rec_off = new tree::BinopExp(tree::BinOp::MUL_OP,new tree::ConstExp(counter),new tree::ConstExp(reg_manager->WordSize()));
  // construct the base
  tree::MemExp *base_ = new tree::MemExp(mid_res->exp_->UnEx());

  tree::MemExp* last_ = new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, base_, rec_off));

  return new tr::ExpAndTy(new tr::ExExp(last_),last_ty);

}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // get the core var by access the venv
  tr::ExpAndTy* mid_res = this->var_->Translate(venv,tenv,level,label,errormsg);
  // parse the sub
  tr::ExpAndTy* sub_res = this->subscript_->Translate(venv,tenv,level,label,errormsg);
  tr::ExExp* e;
  if(dynamic_cast<tr::ExExp*>(mid_res->exp_) == nullptr){
    // check for type
    printf("in sub exist type not expected\n");
  }
  e = new tr::ExExp(mid_res->exp_->UnEx());

  tree::MemExp* arr_fir = new tree::MemExp(e->exp_);
  tree::BinopExp* arr_off = new tree::BinopExp(tree::BinOp::MUL_OP, sub_res->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize()));
  tree::MemExp* last_res = new tree::MemExp(new tree::BinopExp(
    tree::BinOp::PLUS_OP,arr_fir,arr_off
  ));

  // TODO(wjl): there may be buggy because of the type convert
  type::Ty* last_ty = static_cast<type::ArrayTy *>(mid_res->ty_)->ty_;

  return new tr::ExpAndTy(new tr::ExExp(last_res),last_ty);

}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

} // namespace absyn
