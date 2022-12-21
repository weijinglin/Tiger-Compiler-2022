#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include<iostream>

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
    // tr::PatchList *trues = new tr::PatchList();
    // tr::PatchList *falses = new tr::PatchList();
    // Cx* res =  new tr::Cx(*trues,*falses,this->UnNx());
    // return *res;
    tree::CjumpStm *stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    std::list<temp::Label **> tr_pa;
    std::list<temp::Label **> fa_pa;
    tr_pa.push_back(&(stm->true_label_));
    fa_pa.push_back(&(stm->false_label_));
    tr::PatchList *trues = new tr::PatchList(tr_pa);
    tr::PatchList *falses = new tr::PatchList(fa_pa);

    Cx* res =  new tr::Cx(*trues,*falses,stm);
    return *res;
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

  frags->PushBack(new frame::ProcFrag(frame::procEntryExit1(main_level_.get()->frame_,res->exp_->UnNx()),this->main_level_.get()->frame_));

  // debug code 
  // for(auto frag : frags->GetList()){
  //   if(dynamic_cast<frame::ProcFrag*>(frag) != nullptr){
  //     static_cast<frame::ProcFrag*>(frag)->body_->Print(stderr,1);
  //   }
  // }
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* res =  this->root_->Translate(venv,tenv,level,label,errormsg);
  // debug code
  // FILE* test_file = fopen("test_file","a");
  // res->exp_->UnEx()->Print(stderr,1);
  return res;
}

// used to compute for the static link
tree::Exp* static_link_com(tr::Level *def_level,tr::Level *use_level){
  // it's needed only use_level is in the def_level
  // first , get the FP of the use_level

  // some debug code
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
    if(now_level->frame_->formals_->front() == nullptr){
      printf("hit!\n");
    }
    iter = now_level->frame_->formals_->front()->ToExp(iter);
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

  if(sim_var == nullptr){
    printf("simple err be nullptr in venv\n");
  }

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
  if(mid_res->ty_ == nullptr){
    printf("wrong\n");
  }
  // use type to compute the offset of each field
  type::Ty* record_ty = mid_res->ty_->ActualTy();


  // TODO(wjl): there may be buggy because of the type convert
  type::RecordTy* con_record_ty = static_cast<type::RecordTy*>(record_ty);
  std::list<type::Field *> fie_list = con_record_ty->fields_->GetList();
  int counter = 0;
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
  // TODO(wjl) : fix some fetch code because of the modify of lab7
  tree::BinopExp *rec_off = new tree::BinopExp(tree::BinOp::MUL_OP,new tree::ConstExp(counter + 1),new tree::ConstExp(reg_manager->WordSize()));
  // construct the base
  tree::Exp *base_ = mid_res->exp_->UnEx();

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

  tree::Exp* arr_fir = e->exp_;
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
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::NilTy::Instance());
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
  // TODO(wjl): deal with a string_Frag , but the instr is not care here and the label is random (may be wrong)
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
  env::EnvEntry *fun_entry = venv->Look(this->func_);
  // TODO(wjl) : find a bug (external function should not pass static link)
  if(static_cast<env::FunEntry*>(fun_entry)->label_ == nullptr){
    // external function
    // TODO(wjl) : preallocate the pos for the outbound params
    int counter = 0;
    for(auto arg : this->args_->GetList()){
      counter++;
      tr::ExpAndTy* mid_res = arg->Translate(venv,tenv,level,label,errormsg);
      args->Append(mid_res->exp_->UnEx());
    }
    // TODO(wjl) : deal it in codegen
    // if(counter > 6){
    //   level->frame_->frame_size += (counter - 6) * 8;
    // }
  } else {
    if(level->frame_->name_ == this->func_ || static_cast<env::FunEntry*>(fun_entry)->level_->depth <= level->depth){
      // transfer static link
      // TODO(wjl) : preallocate the pos for the outbound params
      int counter = 0;
      frame::Access* static_acc = level->frame_->formals_->front();
      tree::Exp* static_exp = static_link_com(static_cast<env::FunEntry*>(fun_entry)->level_->parent_,level);

      args->Append(static_exp);
      for(auto arg : this->args_->GetList()){
        counter++;
        tr::ExpAndTy* mid_res = arg->Translate(venv,tenv,level,label,errormsg);
        args->Append(mid_res->exp_->UnEx());
      }

      // TODO(wjl) : deal it in codegen
      // if(counter >= 6){
      //   // count the static link
      //   level->frame_->frame_size += (counter - 5) * 8;
      // }
    } else {
      // transfer the frame pointer
      // TODO(wjl) : preallocate the pos for the outbound params
      int counter = 0;
      tree::Exp* static_exp = new tree::TempExp(reg_manager->FramePointer());
      args->Append(static_exp);
      for(auto arg : this->args_->GetList()){
        counter++;
        tr::ExpAndTy* mid_res = arg->Translate(venv,tenv,level,label,errormsg);
        args->Append(mid_res->exp_->UnEx());
      }

      // TODO(wjl) : deal it in codegen
      // if(counter >= 6){
      //   // count the static link
      //   level->frame_->frame_size += (counter - 5) * 8;
      // }
    }
  }

  type::Ty* last_ty = type::VoidTy::Instance();
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
  if(left_res == nullptr || left_res->exp_ == nullptr || right_res == nullptr || right_res->exp_ == nullptr){
    printf("hit left\n");
  }
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
    // TODO(wjl) : add the model to deal with the string compare
    if(dynamic_cast<type::StringTy*>(left_res->ty_) != nullptr){
      tree::ExpList* args = new tree::ExpList({left_res->exp_->UnEx(),right_res->exp_->UnEx()});

      tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::EQ_OP,
      frame::externalCall("string_equal",args),new tree::ConstExp(1),nullptr,nullptr);
      std::list<temp::Label **> pat_trues;
      std::list<temp::Label **> pat_falses;
      pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
      pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
      tr::PatchList *trues = new tr::PatchList(pat_trues);
      tr::PatchList *falses = new tr::PatchList(pat_falses);
      return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
      break;
    } else {
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
    // TODO(wjl) : add the model to deal with the string compare
    if(dynamic_cast<type::StringTy*>(left_res->ty_) != nullptr){
      tree::ExpList* args = new tree::ExpList({left_res->exp_->UnEx(),right_res->exp_->UnEx()});

      tree::Stm* last_ = new tree::CjumpStm(tree::RelOp::NE_OP,
      frame::externalCall("string_equal",args),new tree::ConstExp(1),nullptr,nullptr);
      std::list<temp::Label **> pat_trues;
      std::list<temp::Label **> pat_falses;
      pat_trues.push_back(&(static_cast<tree::CjumpStm*>(last_)->true_label_));
      pat_falses.push_back(&(static_cast<tree::CjumpStm*>(last_)->false_label_));
      tr::PatchList *trues = new tr::PatchList(pat_trues);
      tr::PatchList *falses = new tr::PatchList(pat_falses);
      return new tr::ExpAndTy(new tr::CxExp(*trues,*falses,last_),type::IntTy::Instance());
      break;
    } else {
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
  } 
  default:
    printf("don't take care of all case in OpExp\n");
    break;
  }

}

bool is_pointer(type::Ty* ty_){
  // TODO : here we assume pointer only appear in the case that record or array type
  if((dynamic_cast<type::RecordTy*>(ty_) != nullptr) && (dynamic_cast<type::ArrayTy*>(ty_) != nullptr)){
    return true;
  } else {
    return false;
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : !!! here try to add description for record and may cause buggy(doing in lab7)
  std::list<absyn::EField*> eFie_list = this->fields_->GetList();
  std::list<tr::ExpAndTy*> *exp_list = new std::list<tr::ExpAndTy*>();
  for(auto efie : eFie_list){
    exp_list->push_back(efie->exp_->Translate(venv,tenv,level,label,errormsg));
  }

  // call the external function
  // first , prepare for the params
  int counter = exp_list->size();
  // tree::ConstExp* _size = new tree::ConstExp(counter * reg_manager->WordSize());
  tree::ConstExp* _size = new tree::ConstExp((counter + 1) * reg_manager->WordSize());

  tree::Exp* _call = frame::externalCall("alloc_record",new tree::ExpList({_size}));

  // store the Record address and act as the result of this exp
  temp::Temp *res = temp::TempFactory::NewTemp();
  tree::Stm* ini_stm = new tree::MoveStm(new tree::TempExp(res),_call);

  // TODO(wjl) : construct a string to descript this record and store in the zero field(so it causes we need to fix the method we get record's member)
  std::string des_word = "";
  if(dynamic_cast<type::RecordTy*>(tenv->Look(this->typ_)->ActualTy()) != nullptr){
    for(auto fie : (static_cast<type::RecordTy*>(tenv->Look(this->typ_)->ActualTy())->fields_->GetList())){
      auto ty_ = fie->ty_;
      // check for pointer
      if(is_pointer(ty_->ActualTy())){
        des_word += "1";
      } else {
        des_word += "0";
      }
    }
  } else {
    printf("what wrong in the record type\n");
  }

  // construct the string exp
  temp::Label *str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label,des_word));
  auto str_mes = new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str_label)),type::StringTy::Instance());
  exp_list->push_front(str_mes);

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
  if(move_list.size() == 1){

  } else {
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

  // TODO(wjl) : using another method to code
  // build the stm first
  auto iter = res_list.end();
  iter--;
  iter--;
  if(res_list.size() > 1){
    tree::Stm* all_stm = (*iter)->exp_->UnNx();
    iter--;
    if(res_list.size() > 2){
      while(true){
        all_stm = new tree::SeqStm((*iter)->exp_->UnNx(),all_stm);
        if(iter == res_list.begin()){
          break;
        }
        iter--;
      }
    }

    // build the res
    tree::EseqExp* res = new tree::EseqExp(all_stm,res_list.back()->exp_->UnEx());
    return new tr::ExpAndTy(new tr::ExExp(res),res_list.back()->ty_);
  } else if(res_list.size() == 1){
    return new tr::ExpAndTy(new tr::ExExp(res_list.back()->exp_->UnEx()),res_list.back()->ty_);
  } else {
    printf("error let body is 0\n");
    return nullptr;
  }


  // construct the seq_stm
  // tree::Exp* con_exp = res_list.back()->exp_->UnEx();
  // auto iter = res_list.end();
  // iter--;
  // iter--;
  // int counter = 0;
  // if(res_list.size() > 1){
  //   while(true){
  //     con_exp = new tree::EseqExp(new tree::ExpStm((*iter)->exp_->UnEx()),con_exp);
  //     if(iter == res_list.begin()){
  //       break;
  //     }
  //     iter--;
  //   }
  //   return new tr::ExpAndTy(new tr::ExExp(con_exp),res_list.back()->ty_);
  // } else if(res_list.size() == 1){
  //   return new tr::ExpAndTy(new tr::ExExp(res_list.back()->exp_->UnEx()),res_list.back()->ty_);
  // } else {
  //   printf("error let body is 0\n");
  //   return nullptr;
  // }
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
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

  tr::Cx test_im = test_->exp_->UnCx(errormsg);

  temp::Label* _true = temp::LabelFactory::NewLabel();
  temp::Label* _false = temp::LabelFactory::NewLabel();

  temp::Label* joint_ = temp::LabelFactory::NewLabel();

  temp::Temp* res = temp::TempFactory::NewTemp();

  // build the true branch
  bool is_void = false;
  tr::ExpAndTy* true_exp = this->then_->Translate(venv,tenv,level,label,errormsg);
  // TODO(wjl) : take care of the VOIDTYPE of function
  if(dynamic_cast<type::VoidTy*>(true_exp->ty_) != nullptr){
    is_void = true;
  }
  tree::MoveStm* true_mov = new tree::MoveStm(new tree::TempExp(res),true_exp->exp_->UnEx());

  // build the false branch
  tree::Stm* all_;
  if(this->elsee_){
    tr::ExpAndTy* false_exp = this->elsee_->Translate(venv,tenv,level,label,errormsg);
    tree::MoveStm* false_mov = new tree::MoveStm(new tree::TempExp(res),false_exp->exp_->UnEx());
    test_im.falses_.DoPatch(_false);
    test_im.trues_.DoPatch(_true);
    if(!is_void){
      all_ = new tree::SeqStm(
        test_im.stm_,
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
      all_ = new tree::SeqStm(
        test_im.stm_,
        new tree::SeqStm(
          new tree::LabelStm(_true),
          new tree::SeqStm(
            true_exp->exp_->UnNx(),
            new tree::SeqStm(
              new tree::JumpStm(new tree::NameExp(joint_),new std::vector<temp::Label*>({joint_})),
              new tree::SeqStm(
                new tree::LabelStm(_false),
                new tree::SeqStm(
                  false_exp->exp_->UnNx(),
                  new tree::SeqStm(
                    new tree::JumpStm(new tree::NameExp(joint_),new std::vector<temp::Label*>({joint_})),
                    new tree::LabelStm(joint_)
                  )
                )
              )
            )
          )
        ));
    }
  } else {
    // TODO(wjl) : here may be too redundancy
    test_im.falses_.DoPatch(_false);
    test_im.trues_.DoPatch(_true);
    if(is_void){
      all_ = new tree::SeqStm(
        test_im.stm_,
        new tree::SeqStm(
          new tree::LabelStm(_true),
          new tree::SeqStm(
            true_exp->exp_->UnNx(),
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
    } else {
      all_ = new tree::SeqStm(
        test_im.stm_,
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
  tr::Cx test_cx = _test->exp_->UnCx(errormsg);
  test_cx.falses_.DoPatch(false_);
  test_cx.trues_.DoPatch(true_);
  tree::Stm* while_stm = new tree::SeqStm(
    new tree::LabelStm(begin_),
    new tree::SeqStm(
      test_cx.stm_,
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
  venv->BeginScope();
  venv->Enter(this->var_,new env::VarEntry(tr::Access::AllocLocal(level,this->escape_),type::IntTy::Instance(),true));
  env::EnvEntry* iter = venv->Look(this->var_);
  tree::Exp* iter_val = static_cast<env::VarEntry*>(iter)->access_->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));

  // define some jump label
  temp::Label* test_ = temp::LabelFactory::NewLabel();
  temp::Label* true_ = temp::LabelFactory::NewLabel();
  temp::Label* done_  = temp::LabelFactory::NewLabel();

  tree::Exp* limit_ = this->hi_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx(); 
  tree::Exp* low_lim = this->lo_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx(); 
  // predefine some condition jump
  tree::CjumpStm* cx_ = new tree::CjumpStm(tree::RelOp::LE_OP,
  iter_val, limit_, true_, done_);

  // goto done_
  tree::Stm* goto_done = new tree::JumpStm(new tree::NameExp(done_), new std::vector<temp::Label*>({done_}));

  // body
  tree::Stm* body_fix = this->body_->Translate(venv,tenv,level,done_,errormsg)->exp_->UnNx();

  tree::Stm* inner_loop = new tree::SeqStm(
    new tree::LabelStm(test_),new tree::SeqStm(
      cx_,new tree::SeqStm(
        new tree::LabelStm(true_),new tree::SeqStm(
          body_fix,new tree::SeqStm(
            new tree::MoveStm(iter_val, new tree::BinopExp(tree::BinOp::PLUS_OP,iter_val,new tree::ConstExp(1))),
            new tree::SeqStm(
              new tree::JumpStm(new tree::NameExp(test_), new std::vector<temp::Label*>({test_})),
              new tree::LabelStm(done_)
            )
          )
        )
      )
    )
  );

  // tree::Stm* inner_loop = new tree::SeqStm(
  //   cx_1,
  //   new tree::SeqStm(
  //     new tree::LabelStm(tr_fir),
  //     new tree::SeqStm(
  //       goto_done,
  //       new tree::SeqStm(
  //         new tree::LabelStm(fal_fir),
  //         new tree::SeqStm(
  //           body_fix,
  //           new tree::SeqStm(
  //             cx_2,
  //             new tree::SeqStm(
  //               new tree::LabelStm(tr_sec),
  //               new tree::SeqStm(
  //                 goto_done,
  //                 new tree::SeqStm(
  //                   new tree::LabelStm(Loop),
  //                   new tree::SeqStm(
  //                     new tree::MoveStm(iter_val, new tree::BinopExp(tree::BinOp::PLUS_OP,iter_val,new tree::ConstExp(1))),
  //                     new tree::SeqStm(
  //                       body_fix,
  //                       new tree::SeqStm(
  //                         cx_3,
  //                         new tree::SeqStm(
  //                           new tree::LabelStm(tr_thi),
  //                           new tree::SeqStm(
  //                             new tree::JumpStm(new tree::NameExp(Loop), new std::vector<temp::Label*>({Loop})),
  //                             new tree::LabelStm(done_)
  //                           )
  //                         )
  //                       )
  //                     )
  //                   )
  //                 )
  //               )
  //             )
  //           )
  //         )
  //       )
  //     )
  //   )
  // );

  tree::Stm* last_ = new tree::SeqStm(
    new  tree::MoveStm(iter_val,low_lim),
    inner_loop
  );

  venv->EndScope();

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
  venv->BeginScope();
  tenv->BeginScope();
  for(auto dec : this->decs_->GetList()){
    dec_list.push_back(dec->Translate(venv,tenv,level,label,errormsg));
  }

  // link all the exp together
  tr::ExpAndTy* res;
  if(dec_list.size() != 0){
    auto iter = dec_list.end();
    iter--;
    iter--;
    // tree::Stm* left_stm = new tree::ExpStm(dec_list.back()->UnEx());
    tree::Stm* left_stm = dec_list.back()->UnNx();
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

  venv->EndScope();
  tenv->EndScope();

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
  // TODO(wjl) : may be buggy ! can be replaced bt toExp() (because it doesn't store in register actually)
  // TODO(wjl) : unnecessary , because the variable has been allocated in the assign exp
  tree::Stm* move_stm = new tree::MoveStm(new tree::TempExp(ans),_call);

  // TODO(wjl) : array type or one primitive type
  type::Ty* res_ty = tenv->Look(this->typ_)->ActualTy();

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
  std::vector<tr::Level*> all_new_level;
  for(auto fun_dec : this->functions_->GetList()){
    // construct the function Label
    temp::Label* fun_label = temp::LabelFactory::NamedLabel(fun_dec->name_->Name());
    all_lab.push_back(new tree::LabelStm(fun_label));
    
    // change the venv
    std::list<absyn::Field *> fie_list = fun_dec->params_->GetList();
    type::TyList* ty_list = new type::TyList();
    // push static link to the function first
    ty_list->Append(type::IntTy::Instance());
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
    all_new_level.push_back(new_level);

    if(fun_dec == nullptr){
      printf("error on fun_dec is nullptr\n");
    }
    type::Ty* re_ty = type::VoidTy::Instance();
    if(fun_dec->result_ != nullptr){
      re_ty = tenv->Look(fun_dec->result_);
    }
    env::FunEntry* new_fun = new env::FunEntry(new_level, fun_label, ty_list, re_ty);

    venv->Enter(fun_dec->name_,new_fun);
  }


  // parse the function body
  // prologue and epilogue is not care in translate stage
  int glo_count = 0;
  for(auto fun_bo : this->functions_->GetList()){
    // prepare for the params of a function
    std::list<absyn::Field*> fie_list = fun_bo->params_->GetList();
    auto iter = fie_list.begin();

    venv->BeginScope();

    auto formal_list = all_new_level.at(glo_count)->frame_->formals_;
    auto iter_fo = formal_list->begin();
    // skip static link
    iter_fo++;

    for(;iter != fie_list.end();++iter){
      venv->Enter((*iter)->name_, new env::VarEntry(new tr::Access(all_new_level.at(glo_count),(*iter_fo)),tenv->Look((*iter)->typ_)->ActualTy(),false));
      iter_fo++;
    }

    // get the core fun entry
    env::EnvEntry* _fun = venv->Look(fun_bo->name_);

    // translate body
    tr::ExpAndTy* fun_res = fun_bo->body_->Translate(venv,tenv,static_cast<env::FunEntry*>(_fun)->level_,static_cast<env::FunEntry*>(_fun)->label_,errormsg);
    tree::Stm* mov_stm;
    if(dynamic_cast<type::VoidTy*>(fun_res->ty_) != nullptr){
      mov_stm = fun_res->exp_->UnNx();
    } else {
      mov_stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),fun_res->exp_->UnEx());
    }

    tree::Stm* all_stm = frame::procEntryExit1(static_cast<env::FunEntry*>(_fun)->level_->frame_,mov_stm);

    frame::ProcFrag* _frag = new frame::ProcFrag(all_stm,static_cast<env::FunEntry*>(_fun)->level_->frame_);
    // add the function to the frags
    frags->PushBack(_frag);
    venv->EndScope();

    glo_count++;
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // assign the core frame
  if(this->typ_){
    tr::Access* var_acc = tr::Access::AllocLocal(level, this->escape_);

    tree::Exp* var_exp = var_acc->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));

    tree::Stm* out_stm = new tree::MoveStm(var_exp, this->init_->Translate(venv,tenv,level,label,errormsg)->exp_->UnEx());

    type::Ty* act_ty = tenv->Look(this->typ_)->ActualTy();
    // DONE(wjl) : a bug : using the interface defined by lab4 (fixed)
    venv->Enter(this->var_, new env::VarEntry(var_acc,act_ty,false));
    return new tr::NxExp(out_stm);
  } else{
    tr::Access* var_acc = tr::Access::AllocLocal(level, this->escape_);
    tree::Exp* var_exp = var_acc->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));

    tr::ExpAndTy* res =  this->init_->Translate(venv,tenv,level,label,errormsg);

    // move the output of the function to the core pos(may be stack or register)
    tree::Stm* out_stm = new tree::MoveStm(var_exp, res->exp_->UnEx());

    venv->Enter(this->var_, new env::VarEntry(var_acc,res->ty_,false));
    return new tr::NxExp(out_stm);
  }
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
