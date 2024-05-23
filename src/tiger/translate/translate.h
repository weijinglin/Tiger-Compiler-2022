#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

#include "tiger/frame/x64frame.h"

namespace tr {

class Exp;
class ExpAndTy;
class Level;

class PatchList {
public:
  void DoPatch(temp::Label *label) {
    for(auto &patch : patch_list_) *patch = label;
  }

  static PatchList JoinPatch(const PatchList &first, const PatchList &second) {
    PatchList ret(first.GetList());
    for(auto &patch : second.patch_list_) {
      ret.patch_list_.push_back(patch);
    }
    return ret;
  }

  explicit PatchList(std::list<temp::Label **> patch_list) : patch_list_(patch_list) {}
  PatchList() = default;

  [[nodiscard]] const std::list<temp::Label **> &GetList() const {
    return patch_list_;
  }

private:
  std::list<temp::Label **> patch_list_;
};

class Access {
public:
  Level *level_;
  frame::Access *access_;

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}
  static Access *AllocLocal(Level *level, bool escape);
};

class Level {
public:
  frame::Frame *frame_;
  Level *parent_;

  // may be used in the static link to reduce the computation
  int depth;

  /* TODO: Put your lab5 code here */
  Level(Level* parent, frame::Frame* frame, int depth) : frame_(frame),parent_(parent),depth(depth){}

  // generation a new level (used when call a new function)
  static Level* NewLevel(Level* par_lev,temp::Label *fun_, std::list<bool> formals){
    // push front a static link (true)
    formals.push_front(true);

    frame::Frame * new_frame = new frame::X64Frame(fun_, formals);
    if(par_lev){
      return new Level(par_lev,new_frame,par_lev->depth + 1);
    } else {
      // init for the main_level
      return new Level(par_lev,new_frame,0);
    }
  }

  ~Level(){
  }

};

class ProgTr {
public:
  // TODO: Put your lab5 code here */
  ProgTr(std::unique_ptr<absyn::AbsynTree> Ast, std::unique_ptr<err::ErrorMsg> error_msg):absyn_tree_(std::move(Ast)),
        errormsg_(std::move(error_msg)),
        tenv_(std::make_unique<env::TEnv>()),
        venv_(std::make_unique<env::VEnv>())
  {
    // init the main level , assume the default global function name is main
    temp::Label* main_label = temp::LabelFactory::NamedLabel("tigermain");
    std::list<bool> formals;
    frame::Frame* main_frame = new frame::X64Frame(main_label,formals);
    main_level_.reset(new tr::Level(nullptr,main_frame,0));


    // do some init job
    this->FillBaseTEnv();
    this->FillBaseVEnv();
  };

  /**
   * Translate IR tree
   */
  void Translate();

  /**
   * Transfer the ownership of errormsg to outer scope
   * @return unique pointer to errormsg
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

  ~ProgTr(){
  }


private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<err::ErrorMsg> errormsg_;
  std::unique_ptr<Level> main_level_;
  std::unique_ptr<env::TEnv> tenv_;
  std::unique_ptr<env::VEnv> venv_;

  // Fill base symbol for var env and type env
  void FillBaseVEnv();
  void FillBaseTEnv();

};

} // namespace tr

#endif
