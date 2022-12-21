#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"
#include <algorithm>

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
void RegAllocator::RegAlloc(){
    LivenessAnalysis();
    Build();
    MakeWorkList();
    while(true){
        if(simplifyWorklist->GetList().size() != 0){
            Simplify();
        } else if(worklistMoves->GetList().size() != 0){
            Coalesce();
        } else if(freezeWorklist->GetList().size() != 0){
            Freeze();
        } else if(spillWorklist->GetList().size() != 0){
            SelectSpill();
        }

        if(simplifyWorklist->GetList().size() == 0 && worklistMoves->GetList().size() == 0
        && freezeWorklist->GetList().size() == 0 && spillWorklist->GetList().size() == 0){
            break;
        }
    }
    AssignColors();
    if(this->spilledNodes->GetList().size() != 0){
        RewriteProgram();
        
        RegAlloc();
    } else {
        // prepare for the output
        result->coloring_ = temp::Map::Empty();
        DeleteMove();
        result->il_ = this->assem_instr.get()->GetInstrList();
        // init color
        for(auto node : this->live_graph_factory->GetLiveGraph().interf_graph->Nodes()->GetList()){
            if(node->NodeInfo()->Int() == 107)
                continue;
            result->coloring_->Enter(node->NodeInfo(),reg_manager->getCoreString(color.at(node)));
        }
        return;
    }

}

void RegAllocator::DeleteMove()
{
    cg::AssemInstr* instr_re = new cg::AssemInstr(new assem::InstrList());
    for(auto instr : this->assem_instr.get()->GetInstrList()->GetList()){
        if(dynamic_cast<assem::MoveInstr*>(instr) != nullptr){
            // TODO(wjl) : print
            // temp::Map *color_ = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
            // instr->Print(stderr,color);
            // printf("move instr hit\n");
            // instr->Print(stderr,color_);
            if(instr->Def()){
                if(color[GetAlias(this->live_graph_factory->GetTempNodeMap()->Look(instr->Def()->GetList().front()))]
                 == color[GetAlias(this->live_graph_factory->GetTempNodeMap()->Look(instr->Use()->GetList().front()))]){

                 } else {
                    instr_re->GetInstrList()->Append(instr);
                 }
            }
            else{
                instr_re->GetInstrList()->Append(instr);
            }
        } else {
            instr_re->GetInstrList()->Append(instr);
        }
    }
    this->assem_instr.reset(instr_re);
}

void RegAllocator::LivenessAnalysis()
{
    fg::FlowGraphFactory f_ana(this->assem_instr->GetInstrList());
    f_ana.AssemFlowGraph();
    fg::FGraphPtr flower_graph = f_ana.GetFlowGraph();

    this->live_graph_factory = new live::LiveGraphFactory(flower_graph);
    this->live_graph_factory->Liveness();

    // TODO(wjl) : these may be buggy after rewrite
    // init the precolored node_list
    temp::TempList* all_regs = reg_manager->AllRegisters();
    for(auto reg : all_regs->GetList()){
        if(reg_manager->temp_map_->Look(reg)->compare("%rsp") == 0 || reg_manager->temp_map_->Look(reg)->compare("%rip") == 0){
            // ignore the rsp and rip register
            continue;
        }
        this->precolored->Append(this->live_graph_factory->GetTempNodeMap()->Look(reg));
    }

    // init the initial set
    live::INodeListPtr all_nodes = this->live_graph_factory->GetLiveGraph().interf_graph->Nodes();
    for(auto node : all_nodes->GetList()){
        // escape rsp
        if(this->precolored->Contain(node) || node->NodeInfo()->Int() == 107){
        // if(this->precolored->Contain(node)){
            // printf("t%d\n",node->NodeInfo()->Int());
            continue;
        } else {
            // if(node->NodeInfo()->Int() == 107){
            //     printf("hit\n");
            // }
            this->initial->Append(node);
        }
    }

    // init precolor vector
    int counter = 0;
    for(auto node : precolored->GetList()){
        this->color.insert(std::pair<live::INodePtr,int>{node,counter});
        counter++;
    }
    // this->color.insert(std::pair<live::INodePtr,int>{this->live_graph_factory->GetTempNodeMap()->Look(reg_manager->StackPointer()),counter});
}

void RegAllocator::Build()
{
    // traverse in reverse order
    // auto iter = this->assem_instr->GetInstrList()->GetList().end();
    // int size = this->assem_instr->GetInstrList()->GetList().size();
    // iter--;

    // ps. the edge has been finished in the InterfaceGraph() interface of liveness

    // TODO(wjl) : just update move associated instr (init the worklistMoves and moveList)
    this->worklistMoves = this->live_graph_factory->GetLiveGraph().moves;
    // update moveList
    for(auto instr : this->worklistMoves->GetList()){
        if(this->moveList.find(instr.first) != this->moveList.end()){
            // int prev = this->moveList.at(instr.first)->GetList().size();
            this->moveList.at(instr.first)->Append(instr.first,instr.second);
            // int after = this->moveList.at(instr.first)->GetList().size();
            // assert(prev + 1 == after);
        } else{
            this->moveList.insert(std::pair<live::INodePtr, MoveListPtr>(instr.first,new live::MoveList()));
            this->moveList.at(instr.first)->Append(instr.first,instr.second);
        }

        if(this->moveList.find(instr.second) != this->moveList.end()){
            this->moveList.at(instr.second)->Append(instr.first,instr.second);
        } else{
            this->moveList.insert(std::pair<live::INodePtr, MoveListPtr>(instr.second,new live::MoveList()));
            this->moveList.at(instr.second)->Append(instr.first,instr.second);
        }
    }

    for(auto node : this->live_graph_factory->GetLiveGraph().interf_graph->Nodes()->GetList()){
        // TODO(wjl) : use inDegree may be buggy
        this->degree[node] = node->InDegree();
        if(precolored->Contain(node)){
            this->degree[node] = INT32_MAX;
        }
    }
}

void RegAllocator::Coalesce()
{
    auto m_pair = this->worklistMoves->GetList().front();
    auto x = m_pair.first;
    auto y = m_pair.second;
    // TODO(wjl) : maybe buggy for the pointer computation
    live::INodePtr u,v;
    if(this->precolored->Contain(y)){
        u = y;
        v = x;
    } else {
        u = x;
        v = y;
    }

    this->worklistMoves->Delete(x,y);
    if(u == v){
        this->coalescedMoves->Append(x,y);
        AddWorkList(u);
    } else if(this->precolored->Contain(v) || u->Succ()->Contain(v)){
        this->constrainedMoves->Append(x,y);
        AddWorkList(u);
        AddWorkList(v);
    } else if((this->precolored->Contain(u) && George(v,u)) 
    || (!this->precolored->Contain(u) && Conservative(Adjacent(u)->Union(Adjacent(v))))){
        this->coalescedMoves->Append(x,y);
        Combine(u,v);
        AddWorkList(u);
    } else {
        this->activeMoves->Append(x,y);
    }
}

void RegAllocator::Combine(live::INodePtr u,live::INodePtr v)
{
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    if(this->freezeWorklist->Contain(v)){
        this->freezeWorklist->DeleteNode(v);
    } else {
        this->spillWorklist->DeleteNode(v);
    }
    this->coalescedNodes->Append(v);
    alias.insert(std::pair<live::INodePtr,live::INodePtr>{v,u});
    // a test code
    assert(alias.at(v) == u);

    for(auto move_v : this->moveList.at(v)->GetList()){
        this->moveList.at(u)->Append(move_v.first,move_v.second);
    }

    live::INodeListPtr temp = new live::INodeList();
    temp->Append(v);
    EnableMoves(temp);
    delete temp;

    for(auto t : Adjacent(v)->GetList()){
        // TODO(wjl) : pay attention to the pointer usage
        AddEdge(t,u);
        DecrementDegree(t);
    }
    if(degree.at(u) >= K && this->freezeWorklist->Contain(u)){
        this->freezeWorklist->DeleteNode(u);
        this->spillWorklist->Append(u);
    }
}

void RegAllocator::AddEdge(live::INodePtr t,live::INodePtr u)
{
    // update degree mainly
    if(t == u || t->Succ()->Contain(u)){
        return;
    } else {
        this->live_graph_factory->GetLiveGraph().interf_graph->AddEdge(t,u);
        this->live_graph_factory->GetLiveGraph().interf_graph->AddEdge(u,t);
        degree[t]++;
        degree[u]++;
    }
}

bool RegAllocator::George(live::INodePtr v,live::INodePtr u)
{
    bool res = true;
    for(auto t : Adjacent(v)->GetList()){
        if(!OK(t,u)){
            res = false;
            break;
        }
    }
    return res;
}

bool RegAllocator::Conservative(live::INodeListPtr nodes)
{
    int k = 0;
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    for(auto node : nodes->GetList()){
        if(this->degree.at(node) >= K){
            k++;
        }
    }
    return (k < K);
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr u)
{
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    return degree.at(t) < K || this->precolored->Contain(t) || t->Succ()->Contain(u);
}

void RegAllocator::AddWorkList(live::INodePtr node)
{
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    if(!this->precolored->Contain(node) && !MoveRelated(node) && degree.at(node) < K){
        this->freezeWorklist->DeleteNode(node);
        this->simplifyWorklist->Append(node);
    }
}

void RegAllocator::MakeWorkList()
{
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    std::list<live::INodePtr> delete_set;
    for(auto node : this->initial->GetList()){
        // this->initial->DeleteNode(node);
        delete_set.push_back(node);
        if(degree.at(node) >= K){
            spillWorklist->Append(node);
        } else if (MoveRelated(node)){
            freezeWorklist->Append(node);
        } else {
            simplifyWorklist->Append(node);
        }
    }

    for(auto node : delete_set){
        this->initial->DeleteNode(node);
    }
}

void RegAllocator::Simplify()
{
    // select a node from simplifyList and do simplify
    // get the first node in the list default
    auto node = this->simplifyWorklist->GetList().front();
    simplifyWorklist->DeleteNode(node);

    // use double list to module a stack using the rule that : 
    // 1, push from front
    // 2, pop from front
    this->selectStack->Prepend(node);
    for(auto m_node : Adjacent(node)->GetList()){
        DecrementDegree(m_node);
    }

}

void RegAllocator::DecrementDegree(live::INodePtr node)
{
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    auto d = this->degree.at(node);
    degree[node] = d - 1;
    if(d == K){
        auto union_pre = new live::INodeList();
        union_pre->Append(node);
        EnableMoves(Adjacent(node)->Union(union_pre));
        this->spillWorklist->DeleteNode(node);
        if(MoveRelated(node)){
            this->freezeWorklist->Append(node);
        } else {
            this->simplifyWorklist->Append(node);
        }
    }
}

void RegAllocator::EnableMoves(live::INodeListPtr nodes)
{
    for(auto node : nodes->GetList()){
        for(auto m_node : NodeMoves(node)->GetList()){
            if(this->activeMoves->Contain(m_node.first,m_node.second)){
                this->activeMoves->Delete(m_node.first,m_node.second);
                this->worklistMoves->Append(m_node.first,m_node.second);
            }
        }
    }
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr node)
{
    // TODO(wjl) ; only use succ the determine the neighbor mnmay be buggy
    auto neighbor = node->Succ();
    return neighbor->Diff(this->selectStack->Union(this->coalescedNodes));
}

bool RegAllocator::MoveRelated(live::INodePtr node)
{
    return NodeMoves(node)->GetList().size() != 0;
}

MoveListPtr RegAllocator::NodeMoves(live::INodePtr node){
    if(moveList.find(node) != moveList.end()){
       // can find core key
       return this->moveList.at(node)->Intersect(this->activeMoves->Union(this->worklistMoves));
    } else {
       this->moveList.insert(std::pair<live::INodePtr,MoveListPtr>{node,new live::MoveList()});
       return this->moveList.at(node);
    }
}

void RegAllocator::Freeze()
{
    auto u = this->freezeWorklist->GetList().front();
    this->freezeWorklist->DeleteNode(u);
    this->simplifyWorklist->Append(u);
    FreezeMoves(u);
}

void RegAllocator::FreezeMoves(live::INodePtr u)
{
    live::INodePtr v;
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    for(auto m : NodeMoves(u)->GetList()){
        if(GetAlias(m.second) == GetAlias(u)){
            v = GetAlias(m.first);
        } else {
            v = GetAlias(m.second);
        }
        this->activeMoves->Delete(m.first,m.second);
        this->frozenMoves->Append(m.first,m.second);

        if(NodeMoves(v)->GetList().size() == 0 && degree.at(v) < K){
            this->freezeWorklist->DeleteNode(v);
            this->simplifyWorklist->Append(v);
        }
    }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n)
{
    if(this->coalescedNodes->Contain(n)){
        assert(alias.at(n) != nullptr);
        return GetAlias(alias.at(n));
    } else {
        return n;
    }
}

void RegAllocator::SelectSpill()
{
    auto m = this->spillWorklist->GetList().front();
    this->spillWorklist->DeleteNode(m);
    this->simplifyWorklist->Append(m);
    FreezeMoves(m);
}

void RegAllocator::AssignColors()
{
    // except %rsp and %rip
    const int K = this->precolored->GetList().size();
    while (this->selectStack->GetList().size() != 0)
    {
        /* code */
        // TODO(wjl) : maybe buggy
        auto n = this->selectStack->GetList().front();
        this->selectStack->DeleteNode(n);
        // mode the pop operation

        std::map<int,bool> okColors;
        for(int i = 0; i < K; ++i){
            okColors.insert(std::pair<int,bool>{i,true});
        }

        for(auto w : n->Succ()->GetList()){
            if(this->coloredNodes->Union(precolored)->Contain(GetAlias(w))){
                okColors[color.at(GetAlias(w))] = false;
            }
        }

        int is_empty = true;
        int idx;
        for(int i = 0;i < K;++i){
            if(okColors.at(i) == true){
                is_empty = false;
                idx = i;
                break;
            }
        }
        if(is_empty){
            if(n->NodeInfo()->Int() == 107){
                
            } else {
                this->spilledNodes->Append(n);
            }
        } else {
            this->coloredNodes->Append(n);
            color[n] = idx;
        }
    }

    for(auto n : this->coalescedNodes->GetList()){
        color[n] = color.at(GetAlias(n));
    }
}

void RegAllocator::RewriteProgram()
{
    // TODO(wjl) : maybe buggy : I do not use the new temp
    cg::AssemInstr* instr_re = new cg::AssemInstr(new assem::InstrList());
    // allocate for the variable first
    std::map<temp::Temp*,frame::Access*> temp_map;
    for(auto v : this->spilledNodes->GetList()){
        if(v->NodeInfo()->Int() == 107){
            continue;
        }
        frame::Access* new_access = frame_->allocLocal(true);

        // TODO(wjl) : lab7 fixed code
        // new_access->is_pointer

        temp_map.insert(std::pair<temp::Temp*,frame::Access*>{v->NodeInfo(),new_access});
    }
    temp::Map *color = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
    
    temp::TempList* new_temps = new temp::TempList();

    for(auto instr : this->assem_instr->GetInstrList()->GetList()){
        // load assem need to be place in the front(load)
        temp::TempList **src_ = nullptr, **dst_ = nullptr;
        // TODO(wjl) : debug code
        // instr->Print(stderr,color);

        if(dynamic_cast<assem::LabelInstr*>(instr) != nullptr){
            instr_re->GetInstrList()->Append(instr);
            continue;
        } else if(dynamic_cast<assem::OperInstr*>(instr) != nullptr &&
        static_cast<assem::OperInstr*>(instr)->assem_.compare("") == 0){
            instr_re->GetInstrList()->Append(instr);
            continue;
        }
        if(instr->Use()){
            for(auto usage : instr->Use()->GetList()){
                if(this->spilledNodes->Contain(this->live_graph_factory->GetTempNodeMap()->Look(usage))){
                    frame::Access* new_access = temp_map.at(usage);

                    temp::TempList* src = new temp::TempList();
                    temp::Temp* new_temp = temp::TempFactory::NewTemp();
                    new_temps->Append(new_temp);
                    src->Append(new_temp);

                    // src->Append(usage);

                    assem::OperInstr* new_instr = new assem::OperInstr("movq  (" + frame_->name_->Name() + "_framesize-" +
                    std::to_string(static_cast<frame::InFrameAccess *>(new_access)->offset) + ")(%rsp),`d0",src,nullptr,nullptr);
                    instr_re->GetInstrList()->Append(new_instr);
                    // we can just replace the temp old to new

                    if(dynamic_cast<assem::OperInstr*>(instr) != nullptr){
                        auto src_ = static_cast<assem::OperInstr*>(instr)->src_;
                        temp::TempList* new_src = new temp::TempList();
                        for(auto src_pre : src_->GetList()){
                            if(src_pre == usage){
                                new_src->Append(new_temp);
                            } else {
                                new_src->Append(src_pre);
                            }
                        }
                        static_cast<assem::OperInstr*>(instr)->src_ = new_src;
                    } else if(dynamic_cast<assem::MoveInstr*>(instr) != nullptr){
                        auto src_ = static_cast<assem::MoveInstr*>(instr)->src_;
                        temp::TempList* new_src = new temp::TempList();
                        for(auto src_pre : src_->GetList()){
                            if(src_pre == usage){
                                new_src->Append(new_temp);
                            } else {
                                new_src->Append(src_pre);
                            }
                        }
                        static_cast<assem::OperInstr*>(instr)->src_ = new_src;
                    }

                    // assem::MoveInstr* move_new = new assem::MoveInstr("movq  `s0,`d0\n",new temp::TempList(usage),new temp::TempList(new_temp));
                    // instr_re->GetInstrList()->Append(move_new);
                }
            }
        }

        instr_re->GetInstrList()->Append(instr);

        // define and store
        if(instr->Def()){
            for(auto def : instr->Def()->GetList()){
                if(this->spilledNodes->Contain(this->live_graph_factory->GetTempNodeMap()->Look(def))){
                    // use and store
                    frame::Access* new_access = temp_map.at(def);

                    // construct the temp
                    temp::TempList* src = new temp::TempList();
                    temp::Temp* new_temp = temp::TempFactory::NewTemp();
                    new_temps->Append(new_temp);
                    src->Append(new_temp);
                    // src->Append(def);

                    if(dynamic_cast<assem::OperInstr*>(instr) != nullptr){
                        auto src_ = static_cast<assem::OperInstr*>(instr)->dst_;
                        temp::TempList* new_src = new temp::TempList();
                        for(auto src_pre : src_->GetList()){
                            if(src_pre == def){
                                new_src->Append(new_temp);
                            } else {
                                new_src->Append(src_pre);
                            }
                        }
                        static_cast<assem::OperInstr*>(instr)->dst_ = new_src;
                    } else if(dynamic_cast<assem::MoveInstr*>(instr) != nullptr){
                        auto src_ = static_cast<assem::MoveInstr*>(instr)->dst_;
                        temp::TempList* new_src = new temp::TempList();
                        for(auto src_pre : src_->GetList()){
                            if(src_pre == def){
                                new_src->Append(new_temp);
                            } else {
                                new_src->Append(src_pre);
                            }
                        }
                        static_cast<assem::OperInstr*>(instr)->dst_ = new_src;
                    }


                    // TODO(wjl) : here maybe buggy
                    // assem::MoveInstr* move_new = new assem::MoveInstr("movq  `s0,`d0\n", src, new temp::TempList(def));
                    // instr_re->GetInstrList()->Append(move_new);
                    assem::OperInstr* new_instr = new assem::OperInstr("movq  `s0,(" + frame_->name_->Name() + "_framesize-" + 
                    std::to_string(static_cast<frame::InFrameAccess *>(new_access)->offset) + ")(%rsp)",nullptr,src,nullptr);
                    instr_re->GetInstrList()->Append(new_instr);
                }
            }
        }
    }


    // some init job
    this->spilledNodes->Clear();
    this->initial = this->coloredNodes->Union(this->coalescedNodes);
    this->coloredNodes->Clear();
    this->coalescedNodes->Clear();
    this->assem_instr.reset(instr_re);
    this->simplifyWorklist->Clear();
    this->freezeWorklist->Clear();
    this->spillWorklist->Clear();
    this->selectStack->Clear();


    delete this->coalescedMoves;
    delete this->constrainedMoves;
    delete this->frozenMoves;
    delete this->activeMoves;
    this->coalescedMoves = new live::MoveList();
    this->constrainedMoves = new live::MoveList();
    this->frozenMoves = new live::MoveList();
    // this->worklistMoves = new live::MoveList();
    this->activeMoves = new live::MoveList();

    this->degree.clear();
    this->color.clear();
    this->alias.clear();
    this->moveList.clear();

    this->precolored->Clear();
    this->initial->Clear();

}


} // namespace ra