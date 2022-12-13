#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"


namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  [[nodiscard]] virtual temp::TempList* AllRegisters() = 0;

  [[nodiscard]] virtual std::string* getCoreString(int idx) = 0;

  [[nodiscard]] virtual temp::Temp* Rdx() = 0;

  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */

  virtual tree::Exp *ToExp(tree::Exp *framePtr) const = 0;

  virtual ~Access() = default;
  
};

class Frame {
  /* TODO: Put your lab5 code here */
  
public:  
  // all formals used by a function and their location is stored in the Access
  std::list<frame::Access *> *formals_;
  // represent for the function label
  temp::Label* name_;
  // some instruction used for imple view shift
  std::list<tree::Stm *> *view_shift;
  // the size of frame assigned so far
  int frame_size;

  // 8 is represented for the inital 8 bytes for the return address
  // other case such as local var and saved register is not determined
  // this time.
  // usage : construct a new frame
  Frame(temp::Label* fun_name,std::list<bool> formals):frame_size(0),name_(fun_name)
  {
    // don't need to deal with the formals(because escape analysis)
    this->formals_ = new std::list<frame::Access *>;
    this->view_shift = new std::list<tree::Stm*>;
  }

  Frame() = default;

  virtual frame::Access* allocLocal(bool escape) = 0;

  ~Frame(){ delete this->formals_; }

  std::string GetLabel(){
    return this->name_->Name();
  }
  
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */
tree::Exp* externalCall(std::string_view s, tree::ExpList *args);

assem::Proc* ProcEntryExit3(frame::Frame *frame, assem::InstrList *body);

tree::Stm* procEntryExit1(frame::Frame *frame, tree::Stm* stm);

assem::InstrList* ProcEntryExit2(assem::InstrList* body);

} // namespace frame

#endif