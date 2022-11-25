#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : thinking : codegen every stm core
  tree::StmList *all_stm = this->traces_.get()->GetStmList();
  std::list<tree::Stm *> stm_list = all_stm->GetList();

  assem::InstrList* all_intr = new assem::InstrList();

  cg::AssemInstr *res = new cg::AssemInstr(all_intr);
  for(auto stm_ : stm_list){
    stm_->Munch(*(res->GetInstrList()),this->fs_);
  }

  this->assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(res->GetInstrList()));
  // this->assem_instr_.reset(res);
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->left_->Munch(instr_list,fs);
  this->right_->Munch(instr_list,fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(temp::LabelFactory::LabelString(this->label_),this->label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : use j0 , which may be buggy
  instr_list.Append(new assem::OperInstr("jmp  `j0\n",nullptr,nullptr,new assem::Targets(this->jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : that would not parse the false jump (because the basic block opt)
  switch (this->op_)
  {
  case tree::RelOp::EQ_OP:
  {
    /* code */
    // parse the condition 
    // TODO(wjl) : bug in Condition may be in translate or here(too conflict)
    temp::Temp* left_ = this->left_->Munch(instr_list,fs);
    temp::Temp* right_ = this->right_->Munch(instr_list,fs);

    // construct the compare statement
    instr_list.Append(new assem::OperInstr("cmpq  `s0,`d0\n",new temp::TempList(left_),new temp::TempList(right_),nullptr));
    instr_list.Append(new assem::OperInstr("je  `j0\n",nullptr,nullptr,new assem::Targets(new std::vector<temp::Label*>({this->true_label_}))));
    break;
  }
  case tree::RelOp::NE_OP:
  {
    /* code */
    // parse the condition 
    // TODO(wjl) : bug in Condition may be in translate or here(too conflict)
    temp::Temp* left_ = this->left_->Munch(instr_list,fs);
    temp::Temp* right_ = this->right_->Munch(instr_list,fs);

    // construct the compare statement
    instr_list.Append(new assem::OperInstr("cmpq  `s0,`d0\n",new temp::TempList(left_),new temp::TempList(right_),nullptr));
    instr_list.Append(new assem::OperInstr("jne  `j0\n",nullptr,nullptr,new assem::Targets(new std::vector<temp::Label*>({this->true_label_}))));
    break;
  }
  case tree::RelOp::GE_OP:
  {
    /* code */
    // parse the condition 
    // TODO(wjl) : bug in Condition may be in translate or here(too conflict)
    temp::Temp* left_ = this->left_->Munch(instr_list,fs);
    temp::Temp* right_ = this->right_->Munch(instr_list,fs);

    // construct the compare statement
    instr_list.Append(new assem::OperInstr("cmpq  `s0,`d0\n",new temp::TempList(left_),new temp::TempList(right_),nullptr));
    instr_list.Append(new assem::OperInstr("jge  `j0\n",nullptr,nullptr,new assem::Targets(new std::vector<temp::Label*>({this->true_label_}))));
    break;
  }
  case tree::RelOp::GT_OP:
  {
    /* code */
    // parse the condition 
    // TODO(wjl) : bug in Condition may be in translate or here(too conflict)
    temp::Temp* left_ = this->left_->Munch(instr_list,fs);
    temp::Temp* right_ = this->right_->Munch(instr_list,fs);

    // construct the compare statement
    instr_list.Append(new assem::OperInstr("cmpq  `s0,`d0\n",new temp::TempList(left_),new temp::TempList(right_),nullptr));
    instr_list.Append(new assem::OperInstr("jg  `j0\n",nullptr,nullptr,new assem::Targets(new std::vector<temp::Label*>({this->true_label_}))));
    break;
  }
  case tree::RelOp::LE_OP:
  {
    /* code */
    // parse the condition 
    // TODO(wjl) : bug in Condition may be in translate or here(too conflict)
    temp::Temp* left_ = this->left_->Munch(instr_list,fs);
    temp::Temp* right_ = this->right_->Munch(instr_list,fs);

    // construct the compare statement
    instr_list.Append(new assem::OperInstr("cmpq  `s0,`d0\n",new temp::TempList(left_),new temp::TempList(right_),nullptr));
    instr_list.Append(new assem::OperInstr("jle  `j0\n",nullptr,nullptr,new assem::Targets(new std::vector<temp::Label*>({this->true_label_}))));
    break;
  }
  case tree::RelOp::LT_OP:
  {
    /* code */
    // parse the condition 
    // TODO(wjl) : bug in Condition may be in translate or here(too conflict)
    temp::Temp* left_ = this->left_->Munch(instr_list,fs);
    temp::Temp* right_ = this->right_->Munch(instr_list,fs);

    // construct the compare statement
    instr_list.Append(new assem::OperInstr("cmpq  `s0,`d0\n",new temp::TempList(left_),new temp::TempList(right_),nullptr));
    instr_list.Append(new assem::OperInstr("jl  `j0\n",nullptr,nullptr,new assem::Targets(new std::vector<temp::Label*>({this->true_label_}))));
    break;
  }
  
  default:
    printf("error , can't find matched condition operation\n");
    break;
  }
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(dynamic_cast<tree::TempExp*>(this->dst_) != nullptr){
    // case 1 : store to the register
    if(dynamic_cast<tree::TempExp*>(this->src_) != nullptr){
      instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n", new temp::TempList(this->dst_->Munch(instr_list,fs)),
      new temp::TempList(this->src_->Munch(instr_list,fs))));
      return;
    } else if(dynamic_cast<tree::MemExp*>(this->src_) != nullptr){
      // load from memory
      // instr_list.Append(new assem::OperInstr("movq  s0, d0\n", new temp::TempList(this->src_->Munch(instr_list,fs)),
      // new temp::TempList(this->dst_->Munch(instr_list,fs)),nullptr));
      // if(dynamic_cast<tree::ConstExp*>(static_cast<tree::MemExp*>(this->src_)->exp_) != nullptr){

      // } else if(dynamic_cast<tree::TempExp*>(static_cast<tree::MemExp*>(this->src_)->exp_) != nullptr){

      // } else if((dynamic_cast<tree::BinopExp*>(static_cast<tree::MemExp*>(this->src_)->exp_) != nullptr)
      // && static_cast<tree::BinopExp*>(static_cast<tree::MemExp*>(this->src_)->exp_)->op_ == PLUS_OP){
        
      // } 
      // test easy case first
      temp::Temp* dst = this->dst_->Munch(instr_list,fs);
      if(dst == nullptr){
        printf("wrong seg\n");
      }
      instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n", new temp::TempList(dst),
      new temp::TempList(this->src_->Munch(instr_list,fs))));
      return;
    } else if(dynamic_cast<tree::ConstExp*>(this->src_) != nullptr){
      // the case only appear in the above
      instr_list.Append(new assem::OperInstr("movq  $" + std::to_string(static_cast<tree::ConstExp*>(this->src_)->consti_)
       + ",`d0\n", new temp::TempList(this->dst_->Munch(instr_list,fs)),nullptr,nullptr));
      return;
    } else {
      // test easy case first
      instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n", new temp::TempList(this->dst_->Munch(instr_list,fs)),
      new temp::TempList(this->src_->Munch(instr_list,fs))));
      return;
    }
  } else if(dynamic_cast<tree::TempExp*>(this->src_) != nullptr){
    // case 2 : load from the register
    if(dynamic_cast<tree::MemExp*>(this->dst_) != nullptr){
      // TODO(wjl) : buggy here in function call !!!
      instr_list.Append(new assem::OperInstr("movq  `s0,(`d0)\n",new temp::TempList(static_cast<tree::MemExp*>(this->dst_)->exp_->Munch(instr_list,fs)),
      new temp::TempList(this->src_->Munch(instr_list,fs)),nullptr));
      return;
    } else {
      // the case of reg to reg has been cared in the first condition branch
      printf("unexpected instr type here\n");
      return;
    }

  } else {
    instr_list.Append(new assem::OperInstr("movq  `s0,(`d0)\n",new temp::TempList(static_cast<tree::MemExp*>(this->dst_)->exp_->Munch(instr_list,fs)),
      new temp::TempList(this->src_->Munch(instr_list,fs)),nullptr));
      return;
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->exp_->Munch(instr_list,fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  switch (this->op_)
  {
  case tree::BinOp::PLUS_OP:
  {
    /* code */
    // src can be immediate or reg or mem
    // dst can be reg or mem
    // src and dst can't be mem at the same time
    if(dynamic_cast<tree::TempExp*>(this->right_) != nullptr){
      // left_ is a reg
      // despite mem or reg , they all can be handled by this case
      // TODO(wjl) : here just use a simple case , don't care the memory visit
      // TODO(wjl) : attention : be careful for not effect the value of op register
      if(dynamic_cast<tree::TempExp*>(this->left_) != nullptr){
        temp::TempList* left_op = new temp::TempList(this->left_->Munch(instr_list,fs));
        temp::Temp* right_reg = this->right_->Munch(instr_list,fs);
        
        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(res),
        new temp::TempList(right_reg)));

        left_op->Append(res);
        instr_list.Append(new assem::OperInstr("addq  `s0,`d0\n", new temp::TempList(res),
        left_op,nullptr));
        // right_reg is the register which is returned
        return res;

      } else if(dynamic_cast<tree::MemExp*>(this->left_) != nullptr){
        // TODO(wjl) : this case can be opted (can reduce the num of register used in program)
        temp::TempList* left_op = new temp::TempList(this->left_->Munch(instr_list,fs));
        temp::Temp* right_reg = this->right_->Munch(instr_list,fs);

        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(res),
        new temp::TempList(right_reg)));

        left_op->Append(res);
        instr_list.Append(new assem::OperInstr("addq  (`s0),`d0\n", new temp::TempList(res),
        left_op,nullptr));
        // right_reg is the register which is returned
        return res;

      } else if(dynamic_cast<tree::ConstExp*>(this->left_) != nullptr){
        temp::TempList* left_op = new temp::TempList(this->left_->Munch(instr_list,fs));
        temp::Temp* right_reg = this->right_->Munch(instr_list,fs);

        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(res),
        new temp::TempList(right_reg)));

        left_op->Append(res);
        instr_list.Append(new assem::OperInstr("addq  $" + std::to_string(static_cast<tree::ConstExp*>(this->left_)->consti_) 
        + ",`d0\n",new temp::TempList(res), left_op,nullptr));
        // right_reg is the register which is returned

        return res;
      } else {
        printf("unexpected type in binop\n");
      }
      return nullptr;
    } else if(dynamic_cast<tree::TempExp*>(this->left_) != nullptr){
      // in this case , left_ can't be mem
      // load data from memory to register
      temp::TempList* mem_list = new temp::TempList(this->right_->Munch(instr_list,fs));
      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::OperInstr("movq  `s0,`d0\n",new temp::TempList(res),
      mem_list,nullptr));

      instr_list.Append(new assem::OperInstr("addq  `s0,`d0\n",new temp::TempList(res),
      new temp::TempList(this->left_->Munch(instr_list,fs)),nullptr));
      return res;
    } else if(dynamic_cast<tree::MemExp*>(this->left_) != nullptr){
      temp::Temp* res = temp::TempFactory::NewTemp();
      temp::TempList* bin_op = new temp::TempList(this->right_->Munch(instr_list,fs));
      instr_list.Append(new assem::OperInstr("movq  `s0,`d0\n",new temp::TempList(res),
      bin_op,nullptr));

      instr_list.Append(new assem::OperInstr("addq  `s0,`d0\n",new temp::TempList(res),
      new temp::TempList(this->left_->Munch(instr_list,fs)),nullptr));

      return res;
    } else if(dynamic_cast<tree::ConstExp*>(this->left_) != nullptr){
      // load data from memory to register
      temp::TempList* mem_list = new temp::TempList(this->right_->Munch(instr_list,fs));
      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::OperInstr("movq  `s0,`d0\n",new temp::TempList(res),
      mem_list,nullptr));

      instr_list.Append(new assem::OperInstr("addq  $" + std::to_string(static_cast<tree::ConstExp*>(this->left_)->consti_) 
      + ",`d0\n",new temp::TempList(res),
      nullptr,nullptr));

      return res;
    } else {
      printf("unexpected type\n");
      return nullptr;
    }
    break;
  }

  case tree::BinOp::MINUS_OP:
  {
    /* code */
    // src can be immediate or reg or mem
    // dst can be reg or mem
    // src and dst can't be mem at the same time
    if(dynamic_cast<tree::TempExp*>(this->right_) != nullptr){
      // left_ is a reg
      // despite mem or reg , they all can be handled by this case
      // TODO(wjl) : here just use a simple case , don't care the memory visit
      // TODO(wjl) : attention : be careful for not effect the value of op register
      if(dynamic_cast<tree::TempExp*>(this->left_) != nullptr){
        temp::TempList* right_op = new temp::TempList(this->right_->Munch(instr_list,fs));
        temp::Temp* left_reg = this->left_->Munch(instr_list,fs);
        
        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(res),
        new temp::TempList(left_reg)));

        right_op->Append(res);
        instr_list.Append(new assem::OperInstr("subq  `s0,`d0\n", new temp::TempList(res),
        right_op,nullptr));
        // right_reg is the register which is returned
        return res;

      } else if(dynamic_cast<tree::MemExp*>(this->left_) != nullptr){
        // TODO(wjl) : this case can be opted (can reduce the num of register used in program)
        temp::TempList* right_op = new temp::TempList(this->right_->Munch(instr_list,fs));
        temp::Temp* left_reg = this->left_->Munch(instr_list,fs);

        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(res),
        new temp::TempList(left_reg)));

        right_op->Append(res);
        instr_list.Append(new assem::OperInstr("subq  `s0,`d0\n", new temp::TempList(res),
        right_op,nullptr));
        // right_reg is the register which is returned
        return res;

      } else if(dynamic_cast<tree::ConstExp*>(this->left_) != nullptr){
        temp::TempList* right_op = new temp::TempList(this->right_->Munch(instr_list,fs));
        temp::Temp* left_reg = this->left_->Munch(instr_list,fs);

        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(res),
        new temp::TempList(left_reg)));

        right_op->Append(res);
        instr_list.Append(new assem::OperInstr("subq  $" + std::to_string(static_cast<tree::ConstExp*>(this->left_)->consti_) 
        + ",`d0\n",new temp::TempList(res), right_op,nullptr));
        // right_reg is the register which is returned

        return res;
      } else {
        printf("unexpected type in binop\n");
      }
      return nullptr;
    } else if(dynamic_cast<tree::TempExp*>(this->left_) != nullptr){
      // in this case , left_ can't be mem
      // load data from memory to register
      temp::TempList* left_op = new temp::TempList(this->left_->Munch(instr_list,fs));
      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::OperInstr("movq  `s0,`d0\n",new temp::TempList(res),
      left_op,nullptr));

      instr_list.Append(new assem::OperInstr("subq  `s0,`d0\n",new temp::TempList(res),
      new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));
      return res;
    } else if(dynamic_cast<tree::MemExp*>(this->left_) != nullptr){
      temp::Temp* res = temp::TempFactory::NewTemp();
      temp::TempList* bin_op = new temp::TempList(this->left_->Munch(instr_list,fs));
      instr_list.Append(new assem::OperInstr("movq  `s0,`d0\n",new temp::TempList(res),
      bin_op,nullptr));

      instr_list.Append(new assem::OperInstr("subq  `s0,`d0\n",new temp::TempList(res),
      new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      return res;
    } else if(dynamic_cast<tree::ConstExp*>(this->left_) != nullptr){
      // load data from memory to register
      temp::TempList* left_op = new temp::TempList(this->left_->Munch(instr_list,fs));
      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::OperInstr("movq  `s0,`d0\n",new temp::TempList(res),
      left_op,nullptr));

      instr_list.Append(new assem::OperInstr("subq  `s0,`d0\n",new temp::TempList(res),
      new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));
      
      return res;
    } else {
      printf("unexpected type\n");
      return nullptr;
    }
    break;
  }

  // use untry operation
  case tree::BinOp::MUL_OP:
  {
        // this op is a little different
    // init the initial register (load data to %rax)
    // TODO(WJL) : not care the case of const
    if(dynamic_cast<tree::TempExp*>(this->left_) != nullptr){
      instr_list.Append(new assem::OperInstr("movq  `s0, %rax\n",nullptr,new temp::TempList(this->left_->Munch(instr_list,fs)),
      nullptr));
    } else if(dynamic_cast<tree::MemExp*>(this->left_) != nullptr){
      instr_list.Append(new assem::OperInstr("movq  `s0, %rax\n",nullptr,new temp::TempList(this->left_->Munch(instr_list,fs)),
      nullptr));
    } else if(dynamic_cast<tree::ConstExp*>(this->left_) != nullptr){
      instr_list.Append(new assem::OperInstr("movq  $" + std::to_string(
        static_cast<tree::ConstExp*>(this->left_)->consti_
      ) + ", %rax\n",nullptr,new temp::TempList(this->left_->Munch(instr_list,fs)),
      nullptr));
    }

    instr_list.Append(new assem::OperInstr("cqto",nullptr,nullptr,nullptr));

    if(dynamic_cast<tree::TempExp*>(this->right_) != nullptr){
      // TODO(wjl) : may be buggy because leaving the dst nullptr
      instr_list.Append(new assem::OperInstr("imulq s0\n" , nullptr, new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq  %rax, d0", new temp::TempList(res),nullptr));
      return res;
    } else if(dynamic_cast<tree::MemExp*>(this->right_) != nullptr){
      instr_list.Append(new assem::OperInstr("imulq `s0\n" , nullptr, new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq  %rax,`d0", new temp::TempList(res),nullptr));

      return res;
    } else if(dynamic_cast<tree::ConstExp*>(this->right_) != nullptr){
      instr_list.Append(new assem::OperInstr("imulq $" + std::to_string(
        static_cast<tree::ConstExp*>(this->right_)->consti_
      ) + "\n" , nullptr, new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq  %rax,`d0", new temp::TempList(res),nullptr));

      return res;
    } 

    break;

  }

  case tree::BinOp::DIV_OP:
  {
    // this op is a little different
    // init the initial register (load data to %rax)
    // TODO(WJL) : not care the case of const
    if(dynamic_cast<tree::TempExp*>(this->left_) != nullptr){
      instr_list.Append(new assem::OperInstr("movq  `s0, %rax\n",nullptr,new temp::TempList(this->left_->Munch(instr_list,fs)),
      nullptr));
    } else if(dynamic_cast<tree::MemExp*>(this->left_) != nullptr){
      instr_list.Append(new assem::OperInstr("movq  `s0, %rax\n",nullptr,new temp::TempList(this->left_->Munch(instr_list,fs)),
      nullptr));
    } else if(dynamic_cast<tree::ConstExp*>(this->left_) != nullptr){
      instr_list.Append(new assem::OperInstr("movq  $" + std::to_string(
        static_cast<tree::ConstExp*>(this->left_)->consti_
      ) + ", %rax\n",nullptr,new temp::TempList(this->left_->Munch(instr_list,fs)),
      nullptr));
    }

    instr_list.Append(new assem::OperInstr("cqto",nullptr,nullptr,nullptr));

    if(dynamic_cast<tree::TempExp*>(this->right_) != nullptr){
      // TODO(wjl) : may be buggy because leaving the dst nullptr
      instr_list.Append(new assem::OperInstr("idivq `s0\n" , nullptr, new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq  %rax,`d0", new temp::TempList(res),nullptr));
      return res;
    } else if(dynamic_cast<tree::MemExp*>(this->right_) != nullptr){
      instr_list.Append(new assem::OperInstr("idivq `s0\n" , nullptr, new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq  %rax,`d0", new temp::TempList(res),nullptr));

      return res;
    } else if(dynamic_cast<tree::ConstExp*>(this->right_) != nullptr){
      instr_list.Append(new assem::OperInstr("idivq $" + std::to_string(
        static_cast<tree::ConstExp*>(this->right_)->consti_
      ) + "\n" , nullptr, new temp::TempList(this->right_->Munch(instr_list,fs)),nullptr));

      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq  %rax,`d0", new temp::TempList(res),nullptr));

      return res;
    } 

    break;

  }
  default:
    break;
  }
}

// this function support the value in the memory
temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // just return the address of the mem back
  if(dynamic_cast<tree::BinopExp*>(this->exp_) != nullptr){
    // movq imm(reg), res
    if(static_cast<tree::BinopExp*>(this->exp_)->op_ == tree::PLUS_OP){
      if(dynamic_cast<tree::ConstExp*>(static_cast<tree::BinopExp*>(this->exp_)->left_)
      != nullptr){
        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::OperInstr("movq  " + std::to_string(
          static_cast<tree::ConstExp*>(static_cast<tree::BinopExp*>(this->exp_)->left_)->consti_
        ) + "(`s0),`d0",new temp::TempList(res),
        new temp::TempList(static_cast<tree::BinopExp*>(this->exp_)->right_->Munch(instr_list,fs)),nullptr));
        return res;
      } else {
        // normal case
        temp::Temp* res = temp::TempFactory::NewTemp();
        instr_list.Append(new assem::MoveInstr("movq  (`s0),`d0\n", new temp::TempList(res),
        new temp::TempList(this->exp_->Munch(instr_list, fs))));
        return res;
      }
    } else {
      // movq (reg),res
      temp::Temp* res = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::OperInstr("movq  (`s0),`d0\n",new temp::TempList(res),
      new temp::TempList(this->exp_->Munch(instr_list,fs)),nullptr));
      return res;
    }
  } else if(dynamic_cast<tree::ConstExp*>(this->exp_) != nullptr){
    // movq imm,res
    temp::Temp* res = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::OperInstr("movq  " + std::to_string(static_cast<tree::ConstExp*>(this->exp_)->consti_) + ",`d0\n"
    ,new temp::TempList(res),nullptr,nullptr));
    return res;
  } else {
    temp::Temp* res = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::OperInstr("movq  (`s0),`d0\n",new temp::TempList(res),
    new temp::TempList(this->exp_->Munch(instr_list,fs)),nullptr));
    return res;
  }
  
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(this->temp_->Int() == 106){
    // replace the register with rsp
    // it is not problem because rbp here is not used except for frame pointer
    temp::Temp* my_rbp = temp::TempFactory::NewTemp();
    instr_list.Append(new assem::OperInstr("leaq  " + std::string(fs) + "(%rsp),`d0",new temp::TempList(my_rbp),nullptr,nullptr));
    return my_rbp;
  }
  return this->temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->stm_->Munch(instr_list,fs);
  return this->exp_->Munch(instr_list,fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : attention : this code is relative to machine(may be buggy in actually x64 frame)
  std::string label_ = this->name_->Name();
  int length = label_.length();
  std::string num_str = label_.substr(1,length-1);
  int num = atoi(num_str.c_str());
  
  temp::Temp* name_tmp = temp::TempFactory::NewTemp();
  // std::string* name_str = new std::string(this->name_->Name());

  // TODO(wjl) : attention ! that is buggy code !!!
  int add_ = 0x400000 + num * 8;

  // reg_manager->temp_map_->Enter(name_tmp,name_str);
  instr_list.Append(new assem::MoveInstr("movq  $" + std::to_string(
    add_
  )  + ",`d0\n", new temp::TempList(name_tmp),
  nullptr));
  return name_tmp;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // TODO(wjl) : here just move the const value to a register , so may be wrong (this thinking can't match x86-64 structure)
  temp::Temp* res = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::OperInstr("movq $" + std::to_string(this->consti_) + ",`d0\n",
  new temp::TempList(res),nullptr,nullptr));
  return res;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // temp::Temp* fun = this->fun_->Munch(instr_list,fs);
  // TODO(wjl) : tiger don't exist function embedded
  printf("into call\n");
  temp::Temp* fun = temp::TempFactory::NewTemp();
  std::string *fun_name = new std::string(static_cast<tree::NameExp*>(this->fun_)->name_->Name());
  reg_manager->temp_map_->Enter(fun,fun_name);
  // put the out in core registers
  temp::TempList* args = this->args_->MunchArgs(instr_list,fs);
  temp::TempList *call_decs = reg_manager->CallerSaves();
  call_decs->Append(reg_manager->ReturnValue());

  temp::TempList* src_ = new temp::TempList(fun);
  for(auto item : args->GetList()){
    src_->Append(item);
  }
  instr_list.Append(new assem::OperInstr("callq  `s0\n", call_decs, src_ ,nullptr));
  return reg_manager->ReturnValue();
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // put the params to correct register
  temp::TempList *args = new temp::TempList();
  for(auto exp_ : this->exp_list_){
    temp::Temp* res = exp_->Munch(instr_list,fs);
    args->Append(res);
  }

  int counter = 0;
  temp::TempList* call_params = reg_manager->ArgRegs();
  auto iter = call_params->GetList().begin();
  for(auto item : args->GetList()){
    if(counter < 6){
      instr_list.Append(new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(*iter),new temp::TempList(item)));
      iter++;
    } else {
      // store to the stack
      // TODO(wjl) : may be buggy
      instr_list.Append(new assem::OperInstr("movq  `s0, "+ std::to_string((counter - 5) * 8) + "(%rsp)",nullptr,
      new temp::TempList(item),nullptr));
    }
    counter++;
  }

  // TODO(wjl) : may be buggy
  return args;
}

} // namespace tree
