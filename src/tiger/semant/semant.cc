#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */

  // my design : labelcount = 0 represent that the out_space is not in loop
  // and labelcount = 1 is reverse 
  this->root_->SemAnalyze(venv,tenv,0,errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry* sim_var = venv->Look(this->sym_);
  if(sim_var && ((dynamic_cast< env::VarEntry* >( sim_var )) != nullptr)){
    // use a pointer of base class to get the core ty_ 
    return static_cast<env::VarEntry *>(sim_var)->ty_->ActualTy();
  } else {
    std::string err_msg = "undefined variable " + this->sym_->Name();
    errormsg->Error(this->pos_,err_msg);
    // see it as a IntTy
    return type::VoidTy::Instance();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* re_ty = this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::FieldList *all_fie = static_cast<type::RecordTy *>(re_ty)->fields_;

  if(dynamic_cast<type::RecordTy*>(re_ty) == nullptr){
    errormsg->Error(this->pos_,"not a record type");
    return nullptr;
  }

  std::list<type::Field *> field_list = all_fie->GetList();

  for(auto a_fie : field_list){
    if(a_fie->name_ == this->sym_){
      type::Ty* res = a_fie->ty_;
      return res;
    }
  }

  std::string err_msg = "field " + this->sym_->Name() + " doesn't exist";

  errormsg->Error(this->pos_,err_msg);
  return type::VoidTy::Instance();

}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* arr_ty = this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty* is_Int = this->subscript_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(dynamic_cast<type::ArrayTy*>(arr_ty) == nullptr){
    errormsg->Error(this->pos_,"array type required");
    return nullptr;
  }
  if(is_Int != type::IntTy::Instance()){
    // err handler
    errormsg->Error(this->pos_,"the subscript should be int type\n");
    return type::VoidTy::Instance();
  } else {
    return static_cast<type::ArrayTy*>(arr_ty)->ty_->ActualTy();
  }
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // check the var is in the venv or not
  // there is not handle the error passed by the inner SemAnalyze
  return this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // first , check the correctness of function declare
  env::EnvEntry* fun_var = venv->Look(this->func_);
  if(fun_var && ((dynamic_cast< env::FunEntry *>( fun_var )) != nullptr)){
    // check for the formals
    std::list<Exp *> act_list = this->args_->GetList();
    std::list<type::Ty*> for_list = static_cast<env::FunEntry *>(fun_var)->formals_->GetList();
    if(act_list.size() != for_list.size()){
      if(act_list.size() > for_list.size()){
        std::string err_msg = "too many params in function " + this->func_->Name();
        errormsg->Error(this->pos_,err_msg);
      } else {
        errormsg->Error(this->pos_,"para type mismatch");
      }
      return nullptr;
    }
    bool is_normal = true;
    auto iter = for_list.begin();
    // check each formal in the calling list
    for(auto exp : act_list){
      type::Ty *res_ty = exp->SemAnalyze(venv,tenv,labelcount,errormsg);
      if(res_ty && res_ty->IsSameType(*iter)){
        iter++;
      } else {
        is_normal = false;
        iter++;
        errormsg->Error(this->pos_,"para type mismatch");
      }
    }
    if(!is_normal){
      return nullptr;
    } else {
      return static_cast<env::FunEntry *>(fun_var)->result_;
    }
    
  } else {
    std::string err_msg = "undefined function " + this->func_->Name();
    errormsg->Error(this->pos_,err_msg);
    return type::VoidTy::Instance();
  }
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(this->oper_ == absyn::PLUS_OP || this->oper_ == absyn::MINUS_OP ||
  this->oper_ == absyn::DIVIDE_OP || this->oper_ == absyn::TIMES_OP ||
  this->oper_ == absyn::AND_OP || this->oper_ == absyn::OR_OP){
    type::Ty* lef_ty =  this->left_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty* rig_ty =  this->right_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(lef_ty && ((dynamic_cast< type::IntTy* >(lef_ty)) != nullptr)
    && rig_ty && ((dynamic_cast< type::IntTy* >(rig_ty)) != nullptr)){
      return type::IntTy::Instance();
    } else if(lef_ty && ((dynamic_cast< type::IntTy* >(lef_ty)) != nullptr)){
      errormsg->Error(this->right_->pos_,"integer required");
      return type::IntTy::Instance();
    } else {
      errormsg->Error(this->left_->pos_,"integer required");
      return type::IntTy::Instance();
    }
  }

  if(this->oper_ == absyn::EQ_OP || this->oper_ == absyn::NEQ_OP || 
  this->oper_ == absyn::LT_OP || this->oper_ == absyn::LE_OP || 
  this->oper_ == absyn::GT_OP || this->oper_ == absyn::GE_OP) {
    type::Ty* lef_ty =  this->left_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty* rig_ty =  this->right_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(lef_ty && rig_ty && rig_ty->IsSameType(lef_ty)){
      return type::IntTy::Instance();
    } else if(lef_ty && ((dynamic_cast< type::IntTy* >(lef_ty)) != nullptr)){
      errormsg->Error(this->right_->pos_,"same type required");
      return nullptr;
    } else {
      errormsg->Error(this->left_->pos_,"same type required");
      return nullptr;
    }
  }
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* rec_ty = tenv->Look(this->typ_);
  if(rec_ty){
    if(((dynamic_cast<type::RecordTy*> (rec_ty)) != nullptr)){
      // pass the type check

      // get the input field of defined 
      std::list<EField *> act_list = this->fields_->GetList();
      // get the formal field
      std::list<type::Field *> fie_list = static_cast<type::RecordTy *>(rec_ty)->fields_->GetList();

      // check the symbol first
      if(fie_list.size() != act_list.size()){
        errormsg->Error(this->pos_,"wrong num of field\n");
        return nullptr;
      }
      if(fie_list.size() != 0){
        auto iter = fie_list.begin();
        // compare the name in the field first
        for(auto act_ : act_list){
          if(act_->name_ != (*iter)->name_){
            errormsg->Error(this->pos_,"wrong input in field\n");
          }
          iter++;
        }
      }

      // check the type
      auto iter = fie_list.begin();
      for(auto act_ : act_list){
        type::Ty* exp_ty = act_->exp_->SemAnalyze(venv,tenv,labelcount,errormsg);
        type::Ty* cor_ty = (*iter)->ty_;
        // try to compare the type
        if(exp_ty->IsSameType(cor_ty)){
          iter++;
        } else {
          errormsg->Error(this->pos_,"unmatched type\n");
          return nullptr;
        }
      }

      return rec_ty;
    } else if(((dynamic_cast<type::NameTy*> (rec_ty)) != nullptr) &&
    (dynamic_cast<type::RecordTy *>(static_cast<type::NameTy *>(rec_ty)->ty_)) != nullptr){
      // the case of recur case

      // get the input field of defined 
      std::list<EField *> act_list = this->fields_->GetList();
      // get the formal field
      std::list<type::Field *> fie_list = static_cast<type::RecordTy *>(static_cast<type::NameTy *>(rec_ty)->ty_)->fields_->GetList();

      // check the symbol first
      if(fie_list.size() != act_list.size()){
        errormsg->Error(this->pos_,"wrong num of field\n");
        return nullptr;
      }
      if(fie_list.size() != 0){
        auto iter = fie_list.begin();
        // compare the name in the field first
        for(auto act_ : act_list){
          if(act_->name_ != (*iter)->name_){
            errormsg->Error(this->pos_,"wrong input in field\n");
          }
          iter++;
        }
      }

      // check the type
      auto iter = fie_list.begin();
      for(auto act_ : act_list){
        type::Ty* exp_ty = act_->exp_->SemAnalyze(venv,tenv,labelcount,errormsg);
        type::Ty* cor_ty = (*iter)->ty_;
        // try to compare the type
        if(exp_ty->IsSameType(cor_ty)){
          iter++;
        } else {
          errormsg->Error(this->pos_,"unmatched type\n");
          return nullptr;
        }
      }

      return rec_ty;
    }
    

  } else {
    errormsg->Error(this->pos_,"undefined type rectype");
    return nullptr;
  }
  
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp *> all_exp = this->seq_->GetList();
  type::Ty *res;
  for(auto exp : all_exp){
    // need fault handler ?
    res = exp->SemAnalyze(venv,tenv,labelcount,errormsg);
  }
  return res;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *var_ty = this->var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(dynamic_cast<absyn::SimpleVar *>(this->var_) != nullptr){
    // simple var case
    if(venv->Look(static_cast<absyn::SimpleVar*>(this->var_)->sym_)->readonly_){
      errormsg->Error(this->pos_,"loop variable can't be assigned");
      return nullptr;
    }
  }
  if(var_ty){
    // check the exp
    type::Ty* exp_ty = this->exp_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(!exp_ty){
      errormsg->Error(this->exp_->pos_,"err in exp when assign\n");
      return nullptr;
    }
    if(!exp_ty->IsSameType(var_ty)){
      if(dynamic_cast<type::VoidTy*>(var_ty) != nullptr){

      }
      else
        errormsg->Error(this->pos_,"unmatched assign exp");
      return nullptr;
    }
    // return exp_ty;
    return type::VoidTy::Instance();
  } else {
    errormsg->Error(this->pos_,"undefined variable\n");
    return nullptr;
  }
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(this->elsee_){
    // else is not empty
    type::Ty* if_ty = this->test_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(if_ty && ((dynamic_cast<type::IntTy*> (if_ty)) != nullptr)){
      type::Ty* then_ty = this->then_->SemAnalyze(venv,tenv,labelcount,errormsg);
      type::Ty* else_ty = this->elsee_->SemAnalyze(venv,tenv,labelcount,errormsg);
      // check the equal of then_ty and else_ty
      if(then_ty && then_ty->IsSameType(else_ty))
      {
        // return the last type
        return else_ty;
      } else {
        errormsg->Error(this->pos_,"then exp and else exp type mismatch");
      }
    } else {
      return nullptr;
    }
  }
  else{
    type::Ty* if_ty = this->test_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(if_ty && ((dynamic_cast<type::IntTy*> (if_ty)) != nullptr)){
      type::Ty* then_ty = this->then_->SemAnalyze(venv,tenv,labelcount,errormsg);
      if(dynamic_cast<type::VoidTy*>(then_ty) != nullptr){
        return type::VoidTy::Instance();
      } else {
        errormsg->Error(this->then_->pos_,"if-then exp's body must produce no value");
      }
      
    } else {
      return nullptr;
    }
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* test_ty = this->test_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(test_ty && ((dynamic_cast<type::IntTy*> (test_ty)) != nullptr)){
    type::Ty* res = this->body_->SemAnalyze(venv,tenv,1,errormsg);
    if(res->IsSameType(type::VoidTy::Instance())){
      return type::VoidTy::Instance();
    } else {
      errormsg->Error(this->pos_,"while body must produce no value");
    }
  } else {
    errormsg->Error(this->pos_,"type err in test condition\n");
    return nullptr;
  }
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  type::Ty* lo_ty = this->lo_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty* hi_ty = this->hi_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(!lo_ty || ((dynamic_cast<type::IntTy*>(lo_ty)) == nullptr)){
    errormsg->Error(this->lo_->pos_,"for exp's range type is not integer");
  }
  if(!hi_ty || ((dynamic_cast<type::IntTy*>(hi_ty)) == nullptr)){
    errormsg->Error(this->hi_->pos_,"for exp's range type is not integer");
  }
  venv->Enter(this->var_,new env::VarEntry(type::IntTy::Instance(),true));
  type::Ty* res = this->body_->SemAnalyze(venv,tenv,1,errormsg);
  venv->EndScope();
  return res;
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(labelcount == 1){
    return type::VoidTy::Instance();
  } else {
    errormsg->Error(this->pos_,"break is not inside any loop");
    return nullptr;
  }
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Dec *> dec_list = this->decs_->GetList();
  venv->BeginScope();
  tenv->BeginScope();

  for(auto dec_ : dec_list){
    dec_->SemAnalyze(venv,tenv,labelcount,errormsg);
  }

  type::Ty* res = this->body_->SemAnalyze(venv,tenv,labelcount,errormsg);
  tenv->EndScope();
  venv->EndScope();
  return res;

}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // check for type first
  type::Ty* ini_ty = tenv->Look(this->typ_);
  if(!ini_ty){
    errormsg->Error(this->pos_,"undefined typee");
    return nullptr;
  }

  type::Ty* num_ty = this->size_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty* val_init_ty = this->init_->SemAnalyze(venv,tenv,labelcount,errormsg);

  if(!num_ty || ((dynamic_cast<type::IntTy*>(num_ty)) == nullptr)){
    errormsg->Error(this->pos_,"wrong in num of array");
    return nullptr;
  }
  if(!val_init_ty || !val_init_ty->IsSameType(static_cast<type::ArrayTy *>(ini_ty->ActualTy())->ty_)){
    errormsg->Error(this->pos_,"type mismatch");
    return nullptr;
  }
  return ini_ty;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // need to handle recur case
  std::list<FunDec *> all_fun_list = this->functions_->GetList();
  type::Ty* ret;
  bool is_pro = false;

  // check for the repeat define of function
  for(auto iter = all_fun_list.begin(); iter != all_fun_list.end();++iter){
    for(auto it = all_fun_list.begin();it != iter;++it){
      if((*iter)->name_ == (*it)->name_){
        errormsg->Error(this->pos_, "two functions have the same name");
      }
    }
  }

  for(auto a_func : all_fun_list){

    if(a_func->result_ == nullptr){
      is_pro = true;
      ret = type::VoidTy::Instance();
    } else {
      ret = tenv->Look(a_func->result_);
    }

    if(!ret){
      errormsg->Error(this->pos_,"wrong ret type inf func\n");
      continue;
    }

    std::list<Field *> params = a_func->params_->GetList();
    type::TyList* ty_params = new type::TyList();
    for(auto param : params){
      type::Ty *formal_ = tenv->Look(param->typ_);
      if(formal_){
        ty_params->Append(formal_);
      } else {
        errormsg->Error(this->pos_,"wrong type in the formal\n");
      }
    } 

    venv->Enter(a_func->name_,new env::FunEntry(ty_params,ret));
  }

  // check the function body one by one
  for(auto a_func : all_fun_list){
    // add the param to the env
    venv->BeginScope();
    std::list<Field *> params = a_func->params_->GetList();
    for(auto param : params){
      type::Ty *formal_ = tenv->Look(param->typ_);
      venv->Enter(param->name_,new env::VarEntry(formal_));
    }
    type::Ty* ret_ = a_func->body_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(a_func->result_ == nullptr && dynamic_cast<type::VoidTy*>(ret_) == nullptr){
      errormsg->Error(this->pos_,"procedure returns value");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(this->typ_ == nullptr){
    // without type
    type::Ty *x_ty = this->init_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(dynamic_cast<type::NilTy*>(x_ty) != nullptr){
      errormsg->Error(this->pos_,"init should not be nil without type specified");
    }
    venv->Enter(this->var_,new env::VarEntry(x_ty));
    return;
  } else {
    type::Ty *x_ty = this->init_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty *corr_type = tenv->Look(this->typ_);
    if(x_ty){
      if(x_ty->IsSameType(corr_type)){

      } else {
        errormsg->Error(this->pos_,"type mismatch");
        venv->Enter(this->var_,new env::VarEntry(corr_type));
      }
      venv->Enter(this->var_,new env::VarEntry(x_ty));
      return;
    } else {
      errormsg->Error(this->pos_,"undefined type in the var dec\n");
      return;
    }
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // need to handle the recur case
  // add the dec first
  // here left a case don't care(recur is all symbol type)
  std::list<NameAndTy *> all_ty_list = this->types_->GetList();

  // check for the type repeat define
  for(auto iter = all_ty_list.begin();iter != all_ty_list.end();++iter){
    for(auto it = all_ty_list.begin();it != iter;++it){
      if((*it)->name_ == (*iter)->name_ && iter != it){
        errormsg->Error(this->pos_,"two types have the same name");
        
      }
    }
  }

  for(auto a_ty : all_ty_list){
    // add to the tenv
    // check the type is exist or not first
    // type::Ty* find_ty = tenv->Look(a_ty->name_);
    // if(!find_ty){
    //   errormsg->Error(this->pos_,"two types have the same name");
    // }
    tenv->Enter(a_ty->name_,new type::NameTy(a_ty->name_,nullptr));
  }


  // check for recur
  for(auto a_ty : all_ty_list){
    // tenv->Set(a_ty->name_,a_ty->ty_->SemAnalyze(tenv,errormsg));
    type::Ty* inner_ty = a_ty->ty_->SemAnalyze(tenv,errormsg);
    if(dynamic_cast<type::RecordTy*> (inner_ty) != nullptr || dynamic_cast<type::ArrayTy*> (inner_ty) != nullptr){
      static_cast<type::NameTy *>(tenv->Look(a_ty->name_))->ty_ = inner_ty;
    } else {
      static_cast<type::NameTy *>(tenv->Look(a_ty->name_))->ty_ = inner_ty;
    }

    // tenv->Set(a_ty->name_,inner_ty);
  }

  // check for the cycle 
  for(auto a_ty : all_ty_list){
    type::Ty* inner_ty;
    auto iter_ele = tenv->Look(a_ty->name_);
    iter_ele = iter_ele = static_cast<type::NameTy*>(iter_ele)->ty_;
    while(true){
      if(dynamic_cast<type::NameTy*>(iter_ele) == nullptr){
        break;
      }
      
      if(static_cast<type::NameTy*>(iter_ele)->sym_ == a_ty->name_){
        errormsg->Error(this->pos_,"illegal type cycle");
        return;
      }

      iter_ele = static_cast<type::NameTy*>(iter_ele)->ty_;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* name_ty = tenv->Look(this->name_);
  return new type::NameTy(this->name_,name_ty);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
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

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* arr_ty = tenv->Look(this->array_);
  if(arr_ty){
    return new type::ArrayTy(arr_ty);
  } else {
    errormsg->Error(this->pos_,"wrong in the array type\n");
    return nullptr;
  }

}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
