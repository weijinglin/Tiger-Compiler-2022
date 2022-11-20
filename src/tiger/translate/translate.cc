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
  tr::ExpAndTy* res =  this->absyn_tree_.get()->Translate((this->venv_.get()),
  (this->tenv_.get()),this->main_level_.get(),
  this->main_level_.get()->frame_->name_,this->errormsg_.get());

  frags->PushBack(new frame::ProcFrag(res->exp_->UnNx(),this->main_level_.get()->frame_));
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* res =  this->root_->Translate(venv,tenv,level,label,errormsg);
  return res;
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
      return nullptr;
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
  return this->var_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO(wjl): left Exp* nullptr may be buggy
  return new tr::ExpAndTy(nullptr,type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(this->val_)),type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO(wjl): deal with a string_Frag , but the instr is not care here
  temp::Label *str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label,this->str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str_label)),type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // need to push the static link to the first formals
  tree::Exp* fun_name = new tree::NameExp(this->func_);
  tree::ExpList* args = new tree::ExpList();
  if(label == this->func_){
    // transfer static link
    frame::Access* static_acc = level->frame_->formals_->front();
    tree::Exp* static_exp = static_acc->ToExp(new tree::TempExp(reg_manager->FramePointer()));
    args->Append(static_exp);
    for(auto arg : this->args_->GetList()){
      tr::ExpAndTy* mid_res = arg->Translate(venv,tenv,level,label,errormsg);
      args->Append(mid_res->exp_->UnEx());
    }
  } else {
    // transfer the frame pointer
    tree::Exp* static_exp = new tree::TempExp(reg_manager->FramePointer());
    args->Append(static_exp);
    for(auto arg : this->args_->GetList()){
      tr::ExpAndTy* mid_res = arg->Translate(venv,tenv,level,label,errormsg);
      args->Append(mid_res->exp_->UnEx());
    }
  }

  // get the function return type
  env::EnvEntry *fun_entry = venv->Look(this->func_);
  type::Ty* last_ty;
  if(dynamic_cast<env::FunEntry*>(fun_entry) != nullptr){
    last_ty = static_cast<env::FunEntry*>(fun_entry)->result_;
  } else {
    printf("what ? type of function not match ?\n");
  }

  return new tr::ExpAndTy(new tr::ExExp(new tree::CallExp(fun_name,args)),last_ty);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* left_res = this->left_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* right_res = this->right_->Translate(venv,tenv,level,label,errormsg);
  switch (this->oper_)
  {
  case absyn::Oper::PLUS_OP:
    /* code */
    // TODO(wjl): assume only int can operate on plus , minus , times and so on
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(tree::BinOp::PLUS_OP,left_res->exp_->UnEx(),right_res->exp_->UnEx()))
        ,type::IntTy::Instance());
    break;
  case absyn::Oper::MINUS_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(tree::BinOp::MINUS_OP,left_res->exp_->UnEx(),right_res->exp_->UnEx()))
        ,type::IntTy::Instance());
    break;
  case absyn::Oper::TIMES_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(tree::BinOp::MUL_OP,left_res->exp_->UnEx(),right_res->exp_->UnEx()))
        ,type::IntTy::Instance());
    break;
  case absyn::Oper::DIVIDE_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(tree::BinOp::DIV_OP,left_res->exp_->UnEx(),right_res->exp_->UnEx()))
        ,type::IntTy::Instance());
    break;

  case absyn::Oper::AND_OP:
  {
    // TODO(wjl) : parse the op of || and && as a CxExp(may be wrong)
    // implement the short execv
    temp::Label* mid_pos = temp::LabelFactory::NewLabel();
    // std::list<temp::Label **> pat_trues;
    // std::list<temp::Label **> pat_falses;
    // pat_trues.push_back(&(static_cast<tree::CjumpStm*>(right_res->exp_->UnCx(errormsg).stm_)->true_label_));
    // pat_falses.push_back(&(static_cast<tree::CjumpStm*>(left_res->exp_->UnCx(errormsg).stm_)->false_label_));
    // pat_falses.push_back(&(static_cast<tree::CjumpStm*>(right_res->exp_->UnCx(errormsg).stm_)->false_label_));
    // TODO(wjl) : may be buggy because of the type convert
    // static_cast<tree::CjumpStm*>(left_res->exp_->UnCx(errormsg).stm_)->true_label_ = mid_pos;
    left_res->exp_->UnCx(errormsg).trues_.DoPatch(mid_pos);
    tree::Stm* res = new tree::SeqStm(left_res->exp_->UnCx(errormsg).stm_,
    new tree::SeqStm(new tree::LabelStm(mid_pos),right_res->exp_->UnCx(errormsg).stm_));
    tr::PatchList trues = tr::PatchList::JoinPatch(tr::PatchList() ,right_res->exp_->UnCx(errormsg).trues_);
    // tr::PatchList* falses = new tr::PatchList(pat_falses);
    tr::PatchList falses = tr::PatchList::JoinPatch(left_res->exp_->UnCx(errormsg).falses_,right_res->exp_->UnCx(errormsg).falses_);
    // TODO(wjl) : may be buggy because of the type (not sure for int)
    return new tr::ExpAndTy(
      new tr::CxExp(trues,falses,res),type::IntTy::Instance()
    );
    break;
  }
  case absyn::Oper::OR_OP:
  {
    // TODO(wjl) : parse the op of || and && as a CxExp(may be wrong)
    // implement the short execv
    temp::Label* mid_pos = temp::LabelFactory::NewLabel();
    // std::list<temp::Label **> pat_trues;
    // std::list<temp::Label **> pat_falses;
    // pat_trues.push_back(&(static_cast<tree::CjumpStm*>(right_res->exp_->UnCx(errormsg).stm_)->true_label_));
    // pat_falses.push_back(&(static_cast<tree::CjumpStm*>(left_res->exp_->UnCx(errormsg).stm_)->false_label_));
    // pat_falses.push_back(&(static_cast<tree::CjumpStm*>(right_res->exp_->UnCx(errormsg).stm_)->false_label_));
    // TODO(wjl) : may be buggy because of the type convert
    // static_cast<tree::CjumpStm*>(left_res->exp_->UnCx(errormsg).stm_)->true_label_ = mid_pos;
    left_res->exp_->UnCx(errormsg).falses_.DoPatch(mid_pos);
    tree::Stm* res = new tree::SeqStm(left_res->exp_->UnCx(errormsg).stm_,
    new tree::SeqStm(new tree::LabelStm(mid_pos),right_res->exp_->UnCx(errormsg).stm_));
    tr::PatchList trues = tr::PatchList::JoinPatch(left_res->exp_->UnCx(errormsg).trues_ ,right_res->exp_->UnCx(errormsg).trues_);
    // tr::PatchList* falses = new tr::PatchList(pat_falses);
    tr::PatchList falses = tr::PatchList::JoinPatch(tr::PatchList(), right_res->exp_->UnCx(errormsg).falses_);
    // TODO(wjl) : may be buggy because of the type (not sure for int)
    return new tr::ExpAndTy(
      new tr::CxExp(trues,falses,res),type::IntTy::Instance()
    );
    break;
  }
  
  case absyn::Oper::EQ_OP:
  {
    // TODO(wjl) : left true_label and false_label empty (may be buggy)
    tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::EQ_OP,
    left_res->exp_->UnEx(),right_res->exp_->UnEx(),nullptr,nullptr);
    std::list<temp::Label **> pat_trues;
    std::list<temp::Label **> pat_falses;
    pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
    pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
    tr::PatchList *trues = new tr::PatchList(pat_trues);
    tr::PatchList *falses = new tr::PatchList(pat_falses);
    return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
    break;
  } 
  case absyn::Oper::GE_OP:
  {
    // TODO(wjl) : left true_label and false_label empty (may be buggy)
    tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::GE_OP,
    left_res->exp_->UnEx(),right_res->exp_->UnEx(),nullptr,nullptr);
    std::list<temp::Label **> pat_trues;
    std::list<temp::Label **> pat_falses;
    pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
    pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
    tr::PatchList *trues = new tr::PatchList(pat_trues);
    tr::PatchList *falses = new tr::PatchList(pat_falses);
    return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
    break;
  } 
  case absyn::Oper::LE_OP:
  {
    // TODO(wjl) : left true_label and false_label empty (may be buggy)
    tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::LE_OP,
    left_res->exp_->UnEx(),right_res->exp_->UnEx(),nullptr,nullptr);
    std::list<temp::Label **> pat_trues;
    std::list<temp::Label **> pat_falses;
    pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
    pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
    tr::PatchList *trues = new tr::PatchList(pat_trues);
    tr::PatchList *falses = new tr::PatchList(pat_falses);
    return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
    break;
  } 
  case absyn::Oper::GT_OP:
  {
    // TODO(wjl) : left true_label and false_label empty (may be buggy)
    tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::GT_OP,
    left_res->exp_->UnEx(),right_res->exp_->UnEx(),nullptr,nullptr);
    std::list<temp::Label **> pat_trues;
    std::list<temp::Label **> pat_falses;
    pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
    pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
    tr::PatchList *trues = new tr::PatchList(pat_trues);
    tr::PatchList *falses = new tr::PatchList(pat_falses);
    return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
    break;
  } 
  case absyn::Oper::LT_OP:
  {
    // TODO(wjl) : left true_label and false_label empty (may be buggy)
    tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::LT_OP,
    left_res->exp_->UnEx(),right_res->exp_->UnEx(),nullptr,nullptr);
    std::list<temp::Label **> pat_trues;
    std::list<temp::Label **> pat_falses;
    pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
    pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
    tr::PatchList *trues = new tr::PatchList(pat_trues);
    tr::PatchList *falses = new tr::PatchList(pat_falses);
    return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
    break;
  } 
  case absyn::Oper::NEQ_OP:
 {
    // TODO(wjl) : left true_label and false_label empty (may be buggy)
    tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::NE_OP,
    left_res->exp_->UnEx(),right_res->exp_->UnEx(),nullptr,nullptr);
    std::list<temp::Label **> pat_trues;
    std::list<temp::Label **> pat_falses;
    pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
    pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
    tr::PatchList *trues = new tr::PatchList(pat_trues);
    tr::PatchList *falses = new tr::PatchList(pat_falses);
    return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
    break;
  } 
  default:
    printf("don't take care of all case in OpExp\n");
    break;
  }

}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::EField*> eFie_list = this->fields_->GetList();
  std::list<tr::ExpAndTy*> *exp_list = new std::list<tr::ExpAndTy*>();
  for(auto efie : eFie_list){
    exp_list->push_back(efie->exp_->Translate(venv,tenv,level,label,errormsg));
  }

  // call the external function
  // first , prepare for the params
  int counter = exp_list->size();
  tree::ConstExp* _size = new tree::ConstExp(counter);

  // TODO(wjl) : may be buggy (external call)
  tree::Exp* _call = frame::externalCall("alloc_record",new tree::ExpList({_size}));

  // store the Record address and act as the result of this exp
  temp::Temp *res = temp::TempFactory::NewTemp();
  tree::Stm* ini_stm = new tree::MoveStm(new tree::TempExp(res),_call);

  // do the init job in a loop
  // TODO(wjl) : maybe buggy : too conflict
  int idx = 0;
  std::list<tree::Stm*> move_list;
  for(auto exp : *exp_list){
    tree::MoveStm *mov_stm = new tree::MoveStm(new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP,new tree::TempExp(res)
    ,new tree::ConstExp(idx * reg_manager->WordSize())))
    ,exp->exp_->UnEx());
    idx++;
    move_list.push_back(mov_stm);
  }

  // build the result
  tree::Stm* pro_stm = move_list.back();
  int loop_time = 0;
  auto iter = move_list.end();
  iter--;
  iter--;
  for(;;--iter){
    pro_stm = new tree::SeqStm(*(iter),pro_stm);
    loop_time++;
    if(loop_time + 1 == move_list.size()){
      if(iter != move_list.begin()){
        printf("count err\n");
      }
      break;
    }
  }

  pro_stm = new tree::SeqStm(ini_stm,pro_stm);
  tree::Exp* last_ = new tree::EseqExp(pro_stm,new tree::TempExp(res));

  type::Ty* res_ty = tenv->Look(this->typ_);

  return new tr::ExpAndTy(new tr::ExExp(last_),res_ty);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> exp_list = this->seq_->GetList();
  std::list<tr::ExpAndTy*> res_list;
  // TODO(wjl) : parse all Exp but only the last one will return(may be a bug) and type is the same as the last one ,that would be buggy too!!
  for(auto exp : exp_list){
    res_list.push_back(exp->Translate(venv,tenv,level,label,errormsg));
  }
  return new tr::ExpAndTy(new tr::ExExp(res_list.back()->exp_->UnEx()),res_list.back()->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : how to deal with assign's not return value ?
  tr::ExpAndTy* left_val = this->var_->Translate(venv,tenv,level,label,errormsg);
  tr::ExpAndTy* rig_val = this->exp_->Translate(venv,tenv,level,label,errormsg);

  tree::Stm* mov_stm = new tree::MoveStm(left_val->exp_->UnEx(),rig_val->exp_->UnEx());
  return new tr::ExpAndTy(new tr::NxExp(mov_stm),type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* test_ = this->test_->Translate(venv,tenv,level,label,errormsg);

  temp::Label* _true = temp::LabelFactory::NewLabel();
  temp::Label* _false = temp::LabelFactory::NewLabel();

  temp::Label* joint_ = temp::LabelFactory::NewLabel();

  temp::Temp* res = temp::TempFactory::NewTemp();

  // build the true branch
  tr::ExpAndTy* true_exp = this->then_->Translate(venv,tenv,level,label,errormsg);
  tree::MoveStm* true_mov = new tree::MoveStm(new tree::TempExp(res),true_exp->exp_->UnEx());

  // build the false branch
  tree::Stm* all_;
  if(this->elsee_){
    tr::ExpAndTy* false_exp = this->elsee_->Translate(venv,tenv,level,label,errormsg);
    tree::MoveStm* false_mov = new tree::MoveStm(new tree::TempExp(res),false_exp->exp_->UnEx());
    all_ = new tree::SeqStm(
      test_->exp_->UnCx(errormsg).stm_,
      new tree::SeqStm(
        new tree::LabelStm(_true),
        new tree::SeqStm(
          true_mov,
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(joint_),new std::vector<temp::Label*>({joint_})),
            new tree::SeqStm(
              new tree::LabelStm(_false),
              new tree::SeqStm(
                false_mov,
                new tree::SeqStm(
                  new tree::JumpStm(new tree::NameExp(joint_),new std::vector<temp::Label*>({joint_})),
                  new tree::LabelStm(joint_)
                )
              )
            )
          )
        )
      ));
  } else {
    // TODO(wjl) : here may be too redundancy
    all_ = new tree::SeqStm(
      test_->exp_->UnCx(errormsg).stm_,
      new tree::SeqStm(
        new tree::LabelStm(_true),
        new tree::SeqStm(
          true_mov,
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(joint_),new std::vector<temp::Label*>({joint_})),
            new tree::SeqStm(
              new tree::LabelStm(_false),
              new tree::SeqStm(
                  new tree::JumpStm(new tree::NameExp(joint_),new std::vector<temp::Label*>({joint_})),
                  new tree::LabelStm(joint_)
                )
          )
      )
    )));
  }

  return new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(
    all_,new tree::TempExp(res)
  )),true_exp->ty_);

}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : maybe buggy because of adding label (different with PPT)
  temp::Label* begin_ = temp::LabelFactory::NewLabel();
  temp::Label* done_ = temp::LabelFactory::NewLabel();
  temp::Label* true_ = temp::LabelFactory::NewLabel();
  temp::Label* false_ = temp::LabelFactory::NewLabel();
  tr::ExpAndTy* _test = this->test_->Translate(venv,tenv,level,label,errormsg);
  _test->exp_->UnCx(errormsg).falses_.DoPatch(false_);
  _test->exp_->UnCx(errormsg).trues_.DoPatch(true_);
  tree::Stm* while_stm = new tree::SeqStm(
    new tree::LabelStm(begin_),
    new tree::SeqStm(
      _test->exp_->UnCx(errormsg).stm_,
      new tree::SeqStm(
        new tree::LabelStm(false_),
        new tree::SeqStm(
          new tree::JumpStm(
            new tree::NameExp(done_),
            new std::vector<temp::Label*>({done_})
          ),
          new tree::SeqStm(
            new tree::LabelStm(true_),
            new tree::SeqStm(
              this->body_->Translate(venv,tenv,level,done_,errormsg)->exp_->UnNx(),
              new tree::SeqStm(
                new tree::JumpStm(
                  new tree::NameExp(begin_),
                  new std::vector<temp::Label*>({begin_})
                ),
                new tree::LabelStm(done_)
              )
            )
          )
        )
      )
    )
  );

  return new tr::ExpAndTy(new tr::NxExp(while_stm),type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // get the loop variable
  env::EnvEntry* iter = venv->Look(this->var_);
  tree::Exp* iter_val = static_cast<env::VarEntry*>(iter)->access_->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));

  // define some jump label
  temp::Label* tr_fir = temp::LabelFactory::NewLabel();
  temp::Label* fal_fir = temp::LabelFactory::NewLabel();
  temp::Label* tr_sec = temp::LabelFactory::NewLabel();
  temp::Label* Loop = temp::LabelFactory::NewLabel();
  temp::Label* tr_thi = temp::LabelFactory::NewLabel();
  temp::Label* done_  = temp::LabelFactory::NewLabel();

  tree::Exp* limit_ = this->hi_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx(); 
  tree::Exp* low_lim = this->lo_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx(); 
  // predefine some condition jump
  tree::CjumpStm* cx_1 = new tree::CjumpStm(tree::RelOp::GT_OP,
  iter_val,limit_, tr_fir,fal_fir);
  tree::CjumpStm* cx_2 = new tree::CjumpStm(tree::RelOp::EQ_OP,
  iter_val, limit_, tr_sec, Loop);
  tree::CjumpStm* cx_3 = new tree::CjumpStm(tree::RelOp::LE_OP,
  iter_val, limit_, tr_thi, done_);

  // goto done_
  tree::Stm* goto_done = new tree::JumpStm(new tree::NameExp(done_), new std::vector<temp::Label*>({done_}));

  // body
  tree::Stm* body_ = this->body_->Translate(venv,tenv,level,label,errormsg)->exp_->UnNx();

  tree::Stm* inner_loop = new tree::SeqStm(
    cx_1,
    new tree::SeqStm(
      new tree::LabelStm(tr_fir),
      new tree::SeqStm(
        goto_done,
        new tree::SeqStm(
          new tree::LabelStm(fal_fir),
          new tree::SeqStm(
            body_,
            new tree::SeqStm(
              cx_2,
              new tree::SeqStm(
                new tree::LabelStm(tr_sec),
                new tree::SeqStm(
                  goto_done,
                  new tree::SeqStm(
                    new tree::LabelStm(Loop),
                    new tree::SeqStm(
                      new tree::MoveStm(iter_val, new tree::BinopExp(tree::BinOp::PLUS_OP,iter_val,new tree::ConstExp(1))),
                      new tree::SeqStm(
                        body_,
                        new tree::SeqStm(
                          cx_3,
                          new tree::SeqStm(
                            new tree::LabelStm(tr_thi),
                            new tree::SeqStm(
                              new tree::JumpStm(new tree::NameExp(Loop), new std::vector<temp::Label*>({Loop})),
                              new tree::LabelStm(done_)
                            )
                          )
                        )
                      )
                    )
                  )
                )
              )
            )
          )
        )
      )
    )
  );

  tree::Stm* last_ = new tree::SeqStm(
    new  tree::MoveStm(iter_val,low_lim),
    inner_loop
  );

  return new tr::ExpAndTy(new tr::NxExp(last_),type::VoidTy::Instance());

}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tree::Stm* jump_ = new tree::JumpStm(new tree::NameExp(label),new std::vector<temp::Label*>({label}));
  return new tr::ExpAndTy(new tr::NxExp(jump_),type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<tr::Exp*> dec_list;
  for(auto dec : this->decs_->GetList()){
    dec_list.push_back(dec->Translate(venv,tenv,level,label,errormsg));
  }

  // link all the exp together
  tr::ExpAndTy* res;
  if(dec_list.size() != 0){
    auto iter = dec_list.end();
    iter--;
    iter--;
    tree::Stm* left_stm = new tree::ExpStm(dec_list.back()->UnEx());
    int counter = 0;
    if(dec_list.size() == 1){
      tr::ExpAndTy* body_res = this->body_->Translate(venv,tenv,level,label,errormsg);
      res = new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(left_stm,
      body_res->exp_->UnEx())),body_res->ty_);
    } else {
      while(true){
        left_stm = new tree::SeqStm((*iter)->UnNx(),left_stm);
        iter--;
        counter++;
        if(counter + 1 == dec_list.size()){
          break;
        }
      }

      tr::ExpAndTy* body_res = this->body_->Translate(venv,tenv,level,label,errormsg);
      res = new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(left_stm,
      body_res->exp_->UnEx())),body_res->ty_);
    }
  } else {
    tr::ExpAndTy* body_res = this->body_->Translate(venv,tenv,level,label,errormsg);
    res = new tr::ExpAndTy(new tr::ExExp(body_res->exp_->UnEx()),body_res->ty_);
  }

  return res;  
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // prepare for malloc size
  tree::Exp* _size = this->size_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx();
  tree::Exp* _init = this->init_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx();

  tree::Exp* _call = frame::externalCall("init_array",new tree::ExpList({_size,_init}));

  // build the Temp to store the last return value
  temp::Temp *ans = temp::TempFactory::NewTemp();

  tree::Stm* move_stm = new tree::MoveStm(new tree::TempExp(ans),_call);

  // TODO(wjl) : array type or one primitive type
  type::Ty* res_ty = tenv->Look(this->typ_);

  return new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(move_stm,new tree::TempExp(ans))),res_ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : use const(0) to represent the VoidExp's return value
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // parse the label of the function first
  std::list<tree::LabelStm*> all_lab; 
  for(auto fun_dec : this->functions_->GetList()){
    // construct the function Label
    temp::Label* fun_label = temp::LabelFactory::NamedLabel(fun_dec->name_->Name());
    all_lab.push_back(new tree::LabelStm(fun_label));
    
    // change the venv
    std::list<absyn::Field *> fie_list = fun_dec->params_->GetList();
    type::TyList* ty_list = new type::TyList();
    for(auto fie : fie_list){
      type::Ty* a_ty = tenv->Look(fie->typ_);
      ty_list->Append(a_ty);
    }

    // construct the escape list
    std::list<bool> _escape;

    for(auto fie : fie_list){
      _escape.push_back(fie->escape_);
    }

    tr::Level* new_level = tr::Level::NewLevel(level,fun_label,_escape);

    type::Ty* re_ty = tenv->Look(fun_dec->result_);
    env::FunEntry* new_fun = new env::FunEntry(new_level, fun_label, ty_list, re_ty);

    venv->Enter(fun_dec->name_,new_fun);
  }

  // parse the function body
  // prologue and epilogue is not care in translate stage
  for(auto fun_bo : this->functions_->GetList()){
    // prepare for the params of a function
    std::list<absyn::Field*> fie_list = fun_bo->params_->GetList();
    auto iter = fie_list.begin();
    // skip static link
    iter++;

    venv->BeginScope();

    for(;iter != fie_list.end();++iter){
      venv->Enter((*iter)->name_, new env::VarEntry(tenv->Look((*iter)->typ_)->ActualTy(),false));
    }

    // get the core fun entry
    env::EnvEntry* _fun = venv->Look(fun_bo->name_);

    // translate body
    tr::ExpAndTy* fun_res = fun_bo->body_->Translate(venv,tenv,static_cast<env::FunEntry*>(_fun)->level_,static_cast<env::FunEntry*>(_fun)->label_,errormsg);
    tree::Stm* mov_stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),fun_res->exp_->UnEx());

    tree::Stm* all_stm = frame::procEntryExit1(static_cast<env::FunEntry*>(_fun)->level_->frame_,mov_stm);

    frame::ProcFrag* _frag = new frame::ProcFrag(all_stm,static_cast<env::FunEntry*>(_fun)->level_->frame_);
    // add the function to the frags
    frags->PushBack(_frag);
    venv->EndScope();
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // assign the core frame
  tr::Access* var_acc = tr::Access::AllocLocal(level, this->escape_);

  tree::Exp* var_exp = var_acc->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  tree::Stm* out_stm = new tree::MoveStm(var_exp, this->init_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx());

  type::Ty* act_ty = tenv->Look(this->typ_);
  venv->Enter(this->var_, new env::VarEntry(act_ty,false));

  return new tr::NxExp(out_stm);
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // look all type label and put it into the tenv
  // TODO(wjl) : here don't implement cycle finding so may be buggy
  for(auto a_ty : this->types_->GetList()){
    tenv->Enter(a_ty->name_, new type::NameTy(a_ty->name_,nullptr));
  }

  for(auto a_ty : this->types_->GetList()){
    type::Ty* act_ty = a_ty->ty_->Translate(tenv,errormsg);
    if(dynamic_cast<type::RecordTy*> (act_ty) != nullptr || dynamic_cast<type::ArrayTy*> (act_ty) != nullptr){
      static_cast<type::NameTy *>(tenv->Look(a_ty->name_))->ty_ = act_ty;
    } else {
      static_cast<type::NameTy *>(tenv->Look(a_ty->name_))->ty_ = act_ty;
    }
  }

  return new tr::ExExp(new tree::ConstExp(0));
  
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* name_ty = tenv->Look(this->name_);
  return new type::NameTy(this->name_,name_ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Field *> all_fie = this->record_->GetList();
  type::FieldList* act_fie = new type::FieldList();
  for(auto fie_ : all_fie){
    type::Ty* name_ty = tenv->Look(fie_->typ_);
    if(name_ty){
      act_fie->Append(new type::Field(fie_->name_,name_ty));
    } else{
      std::string err_msg = "undefined type " + fie_->typ_->Name();
      errormsg->Error(this->pos_,err_msg);
      return new type::RecordTy(act_fie);
    }
  }

  return new type::RecordTy(act_fie);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* arr_ty = tenv->Look(this->array_);
  if(arr_ty){
    return new type::ArrayTy(arr_ty);
  } else {
    errormsg->Error(this->pos_,"wrong in the array type\n");
    return nullptr;
  }
}

} // namespace absyn
