#include "tiger/liveness/flowgraph.h"

extern frame::RegManager *reg_manager;

namespace fg {

void PrintAssem(assem::Instr* info)
{ 
  if(dynamic_cast<assem::OperInstr*>(info) != nullptr){
    printf(":%s  ",static_cast<assem::OperInstr*>(info)->assem_.c_str());
  } else if(dynamic_cast<assem::MoveInstr*>(info) != nullptr){
    printf(":%s  ",static_cast<assem::MoveInstr*>(info)->assem_.c_str());
  } else if(dynamic_cast<assem::LabelInstr*>(info) != nullptr){
    printf(":%s  ",static_cast<assem::LabelInstr*>(info)->assem_.c_str());
  } 
}

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  // AssemFlowGraph() will construct the flow graph and store into flowgraph_
  std::list<assem::Instr *> instr_list = this->instr_list_->GetList();
  printf("assem flow size is %d\n",instr_list.size());
  std::list<FNodePtr> jump_list;

  FNodePtr last_node = nullptr;
  FNodePtr curr_node = nullptr;
  int counter = 1;
  for(auto instr : instr_list){
    temp::Map *color = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
    counter++;
    if(counter == instr_list.size() + 1){
      break;
    }
    instr->Print(stderr,color);
    curr_node = this->flowgraph_->NewNode(instr);
    if(!last_node){
      // first instr case and the jump case(jumped dst)
      // last_node = curr_node;
      if(dynamic_cast<assem::OperInstr*>(curr_node->NodeInfo()) != nullptr 
      && static_cast<assem::OperInstr*>(curr_node->NodeInfo())->jumps_ != nullptr){
        // jump instr case(jump source)
        jump_list.push_back(curr_node);
        // there is not edge from jump to next instr (for condition jump case will deal here)
        // judge
        if(static_cast<assem::OperInstr*>(curr_node->NodeInfo())->assem_.find("jmp") == std::string::npos){
          last_node = curr_node;
        } else {
          last_node = nullptr;
        }
      } else if(dynamic_cast<assem::LabelInstr*>(curr_node->NodeInfo()) != nullptr){
        // label case
        printf("find label case is %s\n",static_cast<assem::LabelInstr*>(instr)->label_->Name().c_str());
        this->label_map_.get()->Enter(static_cast<assem::LabelInstr*>(instr)->label_,curr_node);
        last_node = curr_node;
      } else {
        last_node = curr_node;
      }
      continue;
    } else if(dynamic_cast<assem::OperInstr*>(curr_node->NodeInfo()) != nullptr 
    && static_cast<assem::OperInstr*>(curr_node->NodeInfo())->jumps_ != nullptr){
      // jump instr case(jump source)
      jump_list.push_back(curr_node);
      this->flowgraph_->AddEdge(last_node,curr_node);

      // there is not edge from jump to next instr (for condition jump case will deal here)
      // judge
      if(static_cast<assem::OperInstr*>(curr_node->NodeInfo())->assem_.find("jmp") == std::string::npos){
        last_node = curr_node;
      } else {
        last_node = nullptr;
      }
    } else if(dynamic_cast<assem::LabelInstr*>(curr_node->NodeInfo()) != nullptr){
      // label case
      printf("find label case is %s\n",static_cast<assem::LabelInstr*>(instr)->label_->Name().c_str());
      this->label_map_.get()->Enter(static_cast<assem::LabelInstr*>(instr)->label_,curr_node);
      this->flowgraph_->AddEdge(last_node,curr_node);
      last_node = curr_node;
    } else {
      this->flowgraph_->AddEdge(last_node,curr_node);
      last_node = curr_node;
    }
  }

  // deal with the jump instr
  for(auto jump_ : jump_list){
    auto jump_instr = jump_->NodeInfo();
    for(auto label_ : *(static_cast<assem::OperInstr*>(jump_instr)->jumps_->labels_)){
      printf("prog is %s and label is %s\n",static_cast<assem::OperInstr*>(jump_instr)->assem_.c_str(),label_->Name().c_str());
      fg::FNodePtr jump_node = this->label_map_.get()->Look(label_);
      this->flowgraph_->AddEdge(jump_,jump_node);
    }
  }

  // some code to ensure the flowgraph's correctness
  if(false){
    for(auto node : this->flowgraph_->Nodes()->GetList()){
      if(dynamic_cast<assem::OperInstr*>(node->NodeInfo()) != nullptr){
        printf("curr : %s  ",static_cast<assem::OperInstr*>(node->NodeInfo())->assem_.c_str());
        for(auto succ_ : node->Succ()->GetList()){
          PrintAssem(succ_->NodeInfo());
          printf("  ");
        }
        printf("\n");
      } else if(dynamic_cast<assem::MoveInstr*>(node->NodeInfo()) != nullptr){
        printf("curr : %s  ",static_cast<assem::MoveInstr*>(node->NodeInfo())->assem_.c_str());
        for(auto succ_ : node->Succ()->GetList()){
          PrintAssem(succ_->NodeInfo());
          printf("  ");
        }
        printf("\n");
      } else if(dynamic_cast<assem::LabelInstr*>(node->NodeInfo()) != nullptr){
        printf("curr : %s  ",static_cast<assem::LabelInstr*>(node->NodeInfo())->assem_.c_str());
        for(auto succ_ : node->Succ()->GetList()){
          PrintAssem(succ_->NodeInfo());
          printf("  ");
        }
        printf("\n");
      } 
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return this->dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return this->dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return this->src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return this->src_;
}
} // namespace assem
