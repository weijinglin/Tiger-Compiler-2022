#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

bool Contain(temp::TempList* a, temp::Temp* ele){
  bool res = false;
  if(!a){
    printf("error in contain\n");
    return false;
  } else {
    for(auto element : a->GetList()){
      if(element == ele){
        res = true;
        break;
      }
    }
  }
  return res;
}

temp::TempList* Union(temp::TempList* a , temp::TempList* b){
  if(!a && !b){
    printf("seg mentation in Union\n");
    return nullptr;
  } else if(!a){
    return b;  
  } else if(!b){
    return a; 
  } else {
    temp::TempList* res = new temp::TempList(*a);

    for(auto ele : b->GetList()){
      if(Contain(res,ele)){
        continue;
      } else {
        res->Append(ele);
      }
    }
    return res;
  }
}

temp::TempList* Minus(temp::TempList* a, temp::TempList* b){
  if(!a && !b){
    printf("seg mentation in Minus\n");
    return nullptr;
  } else if(!a){
    return b;  
  } else if(!b){
    return a; 
  } else {
    temp::TempList* res = new temp::TempList();

    for(auto ele : b->GetList()){
      if(Contain(a,ele)){
        res->Append(ele);
      }
    }
    return res;
  }
}

bool Equal(temp::TempList* a, temp::TempList* b){
  bool res = true;
  if(!a && !b){
    printf("seg mentation in Minus\n");
    return true;
  } else if(!a){
    return false;  
  } else if(!b){
    return false; 
  } else {
    for(auto ele : a->GetList()){
      if(!Contain(b,ele)){
        res = false;
        break;
      }
    }
  }
  return res;
}


void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  // construct the node set which is active in the instr node
  // store the result in the in_ and out_
  bool is_fix = true;
  auto instr_list = this->flowgraph_->Nodes();
  auto iter = instr_list->GetList().end();
  iter--;
  while(true){
    for(int i = 0;i < instr_list->GetList().size();++i){
      auto prev_in = this->in_->Look(*iter);
      auto prev_out = this->out_->Look(*iter);
      if(prev_in == nullptr){
        this->in_->Enter(*iter,new temp::TempList());
        prev_in = this->in_->Look(*iter);
      }
      if(prev_out == nullptr){
        this->out_->Enter(*iter,new temp::TempList());
        prev_out = this->out_->Look(*iter);
      }

      // and produce new in and out and compare
      auto succ_ = (*iter)->Succ()->GetList();
      auto node_in = Union((*iter)->NodeInfo()->Use(),Minus(prev_out,(*iter)->NodeInfo()->Def()));
      temp::TempList* node_out = new temp::TempList();
      if(succ_.size() > 0)
        node_out = this->in_->Look(succ_.front());
      for(auto succ : succ_){
        node_out = Union(node_out,this->in_->Look(succ));
      }

      if(is_fix && (!Equal(node_in,prev_in) || !Equal(node_out,prev_out))){
        is_fix = false;
      }

      // TODO(wjl) : use set maybe buggy 
      this->in_->Set(*iter,node_in);
      this->out_->Set(*iter,node_out);

      iter--;
    }

    if(is_fix){
      break;
    } else {
      is_fix = true;
      iter = instr_list->GetList().end();
      iter--;
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  // construct the interface_graph and take care of move instr
  // after LiveMap , the in_ and out_ have been filled with the variable.
  auto instr_list = this->flowgraph_->Nodes();
  printf("the size of interfacegraph is %d\n",instr_list->GetList().size());

  for(auto instr : instr_list->GetList()){
    auto node_out = this->out_->Look(instr);

    auto def_list = instr->NodeInfo()->Def();
    if(dynamic_cast<assem::OperInstr*>(instr->NodeInfo()) != nullptr){
      if(def_list){
        for(auto def : def_list->GetList()){
          // this->live_graph_.interf_graph->NewNode();
          auto def_tab = this->temp_node_map_->Look(def);
          if(def_tab == nullptr){
            def_tab = this->live_graph_.interf_graph->NewNode(def);
            this->temp_node_map_->Enter(def,def_tab);
          }

          // normal operation
          for(auto node : node_out->GetList()){
            auto node_tab = this->temp_node_map_->Look(node);
            if(node_tab == nullptr){
              node_tab = this->live_graph_.interf_graph->NewNode(node);
              this->temp_node_map_->Enter(node,node_tab);
            }

            this->live_graph_.interf_graph->AddEdge(def_tab,node_tab);
            this->live_graph_.interf_graph->AddEdge(node_tab,def_tab);
          }
        }
      }
    } else if(dynamic_cast<assem::MoveInstr*>(instr->NodeInfo()) != nullptr){
      auto src_list = instr->NodeInfo()->Use();
      for(auto def : def_list->GetList()){
        // this->live_graph_.interf_graph->NewNode();
        auto def_tab = this->temp_node_map_->Look(def);
        if(def_tab == nullptr){
          def_tab = this->live_graph_.interf_graph->NewNode(def);
          this->temp_node_map_->Enter(def,def_tab);
        }

        // move operation
        for(auto node : node_out->GetList()){
          auto node_tab = this->temp_node_map_->Look(node);
          if(node_tab == nullptr){
            node_tab = this->live_graph_.interf_graph->NewNode(node);
            this->temp_node_map_->Enter(node,node_tab);
          }

          if(!Contain(src_list,node)){
            this->live_graph_.interf_graph->AddEdge(def_tab,node_tab);
            this->live_graph_.interf_graph->AddEdge(node_tab,def_tab);
          } else {
            this->live_graph_.moves->Append(def_tab,node_tab);
          }
        }
      }
    }
  }

}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
