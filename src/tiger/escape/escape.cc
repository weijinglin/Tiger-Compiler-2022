#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */

  // init the depth to one
  env->BeginScope();
  this->root_->Traverse(env,1);
  env->EndScope();
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  
  // get the core esc::EscapeEntry
  esc::EscapeEntry* esc_entry = env->Look(this->sym_);
  // compare the depth, depth : used depth , esc's depth : defined depth
  if(depth > esc_entry->depth_){
    *(esc_entry->escape_) = true;
  }

}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore the field used in Record
  this->var_->Traverse(env,depth);

}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore the var used in Array

  // parse the using in the exp
  // don't need to add the depth
  this->subscript_->Traverse(env,depth);

  // analysis the array
  this->var_->Traverse(env,depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // parse by the core Var
  this->var_->Traverse(env,depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore~~

}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore ~~

}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore~~
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // get all exps in the explist and parse then core
  std::list<Exp *> exp_list = this->args_->GetList();
  for(auto exp : exp_list){
    exp->Traverse(env,depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  
  // parse core by the core exp
  this->left_->Traverse(env,depth);
  this->right_->Traverse(env,depth);

}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // review , such as Record_ty var = {a = exp1,b = exp2}
  // just analysis the exp is ok
  // get the std::list<EField *>
  std::list<EField *> efield_list = this->fields_->GetList();
  // analysis core
  for(auto efield : efield_list){
    efield->exp_->Traverse(env,depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // get the core std::list<Exp *>
  std::list<Exp *> exp_list = this->seq_->GetList();
  for(auto exp : exp_list){
    exp->Traverse(env,depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  
  // analysis the var and exp
  this->var_->Traverse(env,depth);
  this->exp_->Traverse(env,depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  this->test_->Traverse(env,depth);
  this->then_->Traverse(env,depth);
  if(this->elsee_)
    this->elsee_->Traverse(env,depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env,depth);
  this->body_->Traverse(env,depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  // add the readonly variable in the iteration
  env->Enter(this->var_,new esc::EscapeEntry(depth,&(this->escape_)));

  this->lo_->Traverse(env,depth);
  this->hi_->Traverse(env,depth);
  this->body_->Traverse(env,depth);
  env->EndScope();
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore~~

}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  
  // get all dec
  env->BeginScope();
  std::list<Dec *> dec_list = this->decs_->GetList();
  for(auto dec : dec_list){
    dec->Traverse(env,depth);
  }

  // analysis the body
  this->body_->Traverse(env,depth);
  env->EndScope();
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->size_->Traverse(env,depth);
  this->init_->Traverse(env,depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // ignore~~
  
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */

  // get the core dec_list
  std::list<FunDec *> fun_dec_list = this->functions_->GetList();
  for(auto fun : fun_dec_list){
    // analysis the parms
    std::list<Field *> fie_list = fun->params_->GetList();
    env->BeginScope();
    for(auto fie : fie_list){
      env->Enter(fie->name_,new esc::EscapeEntry(depth + 1,&(fie->escape_)));
    }
    fun->body_->Traverse(env,depth + 1);
    env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->init_->Traverse(env,depth);
  env->Enter(this->var_,new esc::EscapeEntry(depth,&(this->escape_)));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  
  // ignore~~
  std::list<NameAndTy *> name_ty_list = this->types_->GetList();
  for(auto ty_list : name_ty_list){
    if(typeid(*(ty_list->ty_)) == typeid(absyn::RecordTy)){
      std::list<Field *> fie_list = static_cast<absyn::RecordTy *>(ty_list->ty_)->record_->GetList();
      for(auto fie : fie_list){
        fie->escape_ = true;
      }
    }
  }
}

} // namespace absyn
