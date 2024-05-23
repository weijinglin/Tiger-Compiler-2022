#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

#include "tiger/frame/x64frame.h"

#include <map>

using MoveListPtr = live::MoveList*;
namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() {}
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
  private:
    // a function frag input by program prog 
    frame::Frame* frame_;
    // core code prog
    std::unique_ptr<cg::AssemInstr> assem_instr;
    // the Result needed to be output
    std::unique_ptr<ra::Result> result;

    // live_graph
    live::LiveGraphFactory* live_graph_factory;

    // some useful set store the node and move instr
    live::INodeListPtr precolored;
    live::INodeListPtr initial;
    live::INodeListPtr simplifyWorklist;
    live::INodeListPtr freezeWorklist;
    live::INodeListPtr spillWorklist;
    live::INodeListPtr spilledNodes;
    live::INodeListPtr coalescedNodes;
    live::INodeListPtr coloredNodes;
    live::INodeListPtr selectStack;

    MoveListPtr coalescedMoves;
    MoveListPtr constrainedMoves;
    MoveListPtr frozenMoves;
    MoveListPtr worklistMoves;
    MoveListPtr activeMoves;

    std::list<std::pair<live::INodePtr,live::INodePtr>> adjSet;
    tab::Table<live::INodePtr,live::INodeListPtr> adjList;
    std::map<live::INodePtr,int> degree;
    std::map<live::INodePtr,MoveListPtr> moveList;
    std::map<live::INodePtr,live::INodePtr> alias;

    std::map<live::INodePtr,int> color;

    // left color need to be init


    void LivenessAnalysis();

    void Build();

    void AddEdge();

    void MakeWorkList();

    bool MoveRelated(live::INodePtr node);

    MoveListPtr NodeMoves(live::INodePtr node);

    void Simplify();

    live::INodeListPtr Adjacent(live::INodePtr node);

    void DecrementDegree(live::INodePtr node);

    void EnableMoves(live::INodeListPtr nodes);

    void Coalesce();

    void AddWorkList(live::INodePtr node);

    bool George(live::INodePtr v,live::INodePtr u);

    bool OK(live::INodePtr t, live::INodePtr u);

    bool Conservative(live::INodeListPtr nodes);

    void Combine(live::INodePtr u,live::INodePtr v);

    void AddEdge(live::INodePtr u,live::INodePtr v);

    void Freeze();

    void FreezeMoves(live::INodePtr u);

    live::INodePtr GetAlias(live::INodePtr n);

    void SelectSpill();

    void AssignColors();

    void RewriteProgram();

    void DeleteMove();

    // TODO : lab7
    void genMap();

    bool is_callee_saved(temp::Temp* reg);

  public:
    RegAllocator(frame::Frame* frame,std::unique_ptr<cg::AssemInstr> assem_):frame_(frame),assem_instr(std::move(assem_)),result(std::make_unique<ra::Result>())
    {
      live_graph_factory = nullptr;
      // init core set
      this->precolored = new live::INodeList();
      this->initial = new live::INodeList();
      this->simplifyWorklist = new live::INodeList();
      this->freezeWorklist = new live::INodeList();
      this->spilledNodes = new live::INodeList();
      this->spillWorklist = new live::INodeList();
      this->coalescedNodes = new live::INodeList();
      this->coloredNodes = new live::INodeList();
      this->selectStack = new live::INodeList();

      this->coalescedMoves = new live::MoveList();
      this->constrainedMoves = new live::MoveList();
      this->frozenMoves = new live::MoveList();
      // this->worklistMoves = new live::MoveList();
      this->activeMoves = new live::MoveList();

      // init precolored and initial

    }

    void RegAlloc();

    std::unique_ptr<ra::Result> TransferResult(){ return std::move(this->result); }


};

} // namespace ra

#endif