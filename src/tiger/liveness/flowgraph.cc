#include "tiger/liveness/flowgraph.h"

extern frame::RegManager *reg_manager;

namespace fg {

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
      last_node = curr_node;
      continue;
    } else if(dynamic_cast<assem::OperInstr*>(curr_node->NodeInfo()) != nullptr 
    && static_cast<assem::OperInstr*>(curr_node->NodeInfo())->jumps_ != nullptr){
      // jump instr case(jump source)
      jump_list.push_back(curr_node);
      this->flowgraph_->AddEdge(last_node,curr_node);

      // there is not edge from jump to next instr (for condition jump case will deal here)
      // judge
      if(static_cast<assem::OperInstr*>(curr_node->NodeInfo())->assem_.find("jmp") != std::string::npos){
        last_node = curr_node;
      } else {
        last_node = nullptr;
      }
    } else if(dynamic_cast<assem::LabelInstr*>(curr_node->NodeInfo()) != nullptr){
      // label case
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
      fg::FNodePtr jump_node = this->label_map_.get()->Look(label_);
      this->flowgraph_->AddEdge(jump_,jump_node);
    }
  }

  printf("flowgraph is below\n");
  // this->flowgraph_->Show(stderr,)
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
