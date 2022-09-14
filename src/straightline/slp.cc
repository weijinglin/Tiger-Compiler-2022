#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int res = (this->stm1->MaxArgs() > this->stm2->MaxArgs())
                ? this->stm1->MaxArgs()
                : this->stm2->MaxArgs();
  return res;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  Table *mid_res = this->stm1->Interp(t);

  // std::cout << "Debug" << std::endl;
  // mid_res->Debug();
  // std::cout << std::endl;

  return this->stm2->Interp(mid_res);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int res = this->exp->MaxArgs();
  return res;
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *mid_res = this->exp->Interp(t);
  if (mid_res->t == nullptr) {
    return new Table(this->id, mid_res->i, nullptr);
  } else {
    return mid_res->t->Update(this->id, mid_res->i);
  }
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int res = (this->exps->NumExps() > this->exps->MaxArgs())
                ? this->exps->NumExps()
                : this->exps->MaxArgs();
  return res;
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  // the work of print need to done by this->exps->Interp(t)
  IntAndTable *mid_res = this->exps->Interp(t);

  return mid_res->t;
}

int A::IdExp::MaxArgs() const { return 0; }

int A::NumExp::MaxArgs() const { return 0; }

int A::OpExp::MaxArgs() const {
  int res = (this->left->MaxArgs() > this->right->MaxArgs())
                ? this->left->MaxArgs()
                : this->right->MaxArgs();
  return res;
}

int A::EseqExp::MaxArgs() const {
  int res = (this->exp->MaxArgs() > this->stm->MaxArgs())
                ? this->exp->MaxArgs()
                : this->stm->MaxArgs();
  return res;
}

int A::PairExpList::MaxArgs() const {
  int res = (this->exp->MaxArgs() > this->tail->MaxArgs())
                ? this->exp->MaxArgs()
                : this->tail->MaxArgs();
  return res;
}

int A::PairExpList::NumExps() const {
  if (this->tail == nullptr) {
    return 1;
  } else {
    int res = 1 + this->tail->NumExps();
    return res;
  }
}

IntAndTable *A::IdExp::Interp(Table *t) const {
  return new IntAndTable(t->Lookup(this->id), t);
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  return new IntAndTable(this->num, t);
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  IntAndTable *mid_res = this->left->Interp(t);
  IntAndTable *mid_res_2 = this->right->Interp(t);

  switch (this->oper) {
  case PLUS:
    /* code */
    return new IntAndTable(mid_res->i + mid_res_2->i, mid_res_2->t);
    break;

  case MINUS:
    /* code */
    return new IntAndTable(mid_res->i - mid_res_2->i, mid_res_2->t);
    break;

  case TIMES:
    /* code */
    return new IntAndTable(mid_res->i * mid_res_2->i, mid_res_2->t);
    break;

  case DIV:
    /* code */
    return new IntAndTable(mid_res->i / mid_res_2->i, mid_res_2->t);
    break;

  default:
    break;
  }
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  Table *mid_res = this->stm->Interp(t);
  IntAndTable *res = this->exp->Interp(mid_res);
  return res;
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  if (this->tail == nullptr) {
    IntAndTable *res = this->exp->Interp(t);
    std::cout << res->i << std::endl;
    return res;
  } else {
    IntAndTable *mid_res = this->exp->Interp(t);
    std::cout << mid_res->i << " ";
    return this->tail->Interp(mid_res->t);
  }
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  IntAndTable *mid_res = this->exp->Interp(t);
  std::cout << mid_res->i << std::endl;
  return mid_res;
}

int A::LastExpList::MaxArgs() const { return this->exp->MaxArgs(); }

int A::LastExpList::NumExps() const { return 1; }

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}

// a function used for debug
void Table::Debug() const {
  if (this->tail != nullptr) {
    std::cout << this->id << "  value : " << this->value << " ";
    this->tail->Debug();
  } else {
    std::cout << this->id << "  value : " << this->value << " ";
  }
  return;
}

} // namespace A
