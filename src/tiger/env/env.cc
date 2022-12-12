#include "tiger/env/env.h"
#include "tiger/translate/translate.h"
#include "tiger/semant/semant.h"

namespace sem {
void ProgSem::FillBaseTEnv() {
  tenv_->Enter(sym::Symbol::UniqueSymbol("int"), type::IntTy::Instance());
  tenv_->Enter(sym::Symbol::UniqueSymbol("string"), type::StringTy::Instance());
}

void ProgSem::FillBaseVEnv() {
  type::Ty *result;
  type::TyList *formals;

  venv_->Enter(sym::Symbol::UniqueSymbol("flush"),
               new env::FunEntry(new type::TyList(), type::VoidTy::Instance()));

  formals = new type::TyList(type::IntTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("exit"),
      new env::FunEntry(formals, type::VoidTy::Instance()));

  result = type::StringTy::Instance();

  venv_->Enter(sym::Symbol::UniqueSymbol("chr"),
               new env::FunEntry(formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("getchar"),
               new env::FunEntry(new type::TyList(), result));

  formals = new type::TyList(type::StringTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("print"),
      new env::FunEntry(formals, type::VoidTy::Instance()));
  venv_->Enter(sym::Symbol::UniqueSymbol("printi"),
               new env::FunEntry(new type::TyList(type::IntTy::Instance()),
                                 type::VoidTy::Instance()));

  result = type::IntTy::Instance();
  venv_->Enter(sym::Symbol::UniqueSymbol("ord"),
               new env::FunEntry(formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("size"),
               new env::FunEntry(formals, result));

  result = type::StringTy::Instance();
  formals = new type::TyList(
      {type::StringTy::Instance(), type::StringTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("concat"),
               new env::FunEntry(formals, result));

  formals =
      new type::TyList({type::StringTy::Instance(), type::IntTy::Instance(),
                        type::IntTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("substring"),
               new env::FunEntry(formals, result));

}

} // namespace sem

namespace tr {

void ProgTr::FillBaseTEnv() {
  tenv_->Enter(sym::Symbol::UniqueSymbol("int"), type::IntTy::Instance());
  tenv_->Enter(sym::Symbol::UniqueSymbol("string"), type::StringTy::Instance());
}

void ProgTr::FillBaseVEnv() {
  type::Ty *result;
  type::TyList *formals;

  temp::Label *label = nullptr;
  tr::Level *level = main_level_.get();

  venv_->Enter(sym::Symbol::UniqueSymbol("flush"),
               new env::FunEntry(level, label, new type::TyList(),
                                 type::VoidTy::Instance()));

  formals = new type::TyList(type::IntTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("exit"),
      new env::FunEntry(level, label, formals, type::VoidTy::Instance()));

  result = type::StringTy::Instance();

  venv_->Enter(sym::Symbol::UniqueSymbol("chr"),
               new env::FunEntry(level, label, formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("getchar"),
               new env::FunEntry(level, label, new type::TyList(), result));

  formals = new type::TyList(type::StringTy::Instance());

  venv_->Enter(
      sym::Symbol::UniqueSymbol("print"),
      new env::FunEntry(level, label, formals, type::VoidTy::Instance()));
  venv_->Enter(sym::Symbol::UniqueSymbol("printi"),
               new env::FunEntry(level, label,
                                 new type::TyList(type::IntTy::Instance()),
                                 type::VoidTy::Instance()));

  result = type::IntTy::Instance();
  venv_->Enter(sym::Symbol::UniqueSymbol("ord"),
               new env::FunEntry(level, label, formals, result));

  venv_->Enter(sym::Symbol::UniqueSymbol("size"),
               new env::FunEntry(level, label, formals, result));

  result = type::StringTy::Instance();
  formals = new type::TyList(
      {type::StringTy::Instance(), type::StringTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("concat"),
               new env::FunEntry(level, label, formals, result));

  formals =
      new type::TyList({type::StringTy::Instance(), type::IntTy::Instance(),
                        type::IntTy::Instance()});
  venv_->Enter(sym::Symbol::UniqueSymbol("substring"),
               new env::FunEntry(level, label, formals, result));

}

} // namespace tr

namespace frame{
    tree::Exp* externalCall(std::string_view s, tree::ExpList *args){
        // TODO(wjl) : there don't do any deal with
        return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),args);
    }

    tree::Stm* procEntryExit1(frame::Frame *frame, tree::Stm* stm){
        // concat the view shift code with the given code
        auto iter = frame->view_shift->end();
        // tree::Stm* new_stm = frame->view_shift->back();
        tree::Stm* new_stm = stm;
        --iter;
        // --iter;
        // do concat job
        int callee_num = reg_manager->CalleeSaves()->GetList().size();
        int counter = 0;
        if(frame->view_shift->size() == 0){
            return stm;
        } else  {
            // for(int i = 1;i < callee_num;++i){
            //     new_stm = new tree::SeqStm(*iter,new_stm);
            //     iter--;
            // }
            // new_stm = new tree::SeqStm(stm,new_stm);
            while(true){
                new_stm = new tree::SeqStm(*iter,new_stm);
                if(iter == frame->view_shift->begin()){
                    break;
                }
                counter++;
                --iter;
            }
        }

        return new_stm;

    }

    assem::InstrList* ProcEntryExit2(assem::InstrList* body){
        body->Append(new assem::OperInstr("", new temp::TempList(),
                                    reg_manager->ReturnSink(), nullptr));
        return body;

    }

    assem::Proc* ProcEntryExit3(frame::Frame *frame, assem::InstrList *body){
        char buf[100];
        std::string fs = frame->name_->Name() + "_framesize";
        std::string fs_num = std::to_string(frame->frame_size);
        std::string ahead = ".set " + fs +  ", " + fs_num + "\n%s:\nsubq  $" + std::to_string(frame->frame_size) + ",%rsp\n";
        sprintf(buf, 
        ahead.c_str(), 
        temp::LabelFactory::LabelString(frame->name_).data());
        return new assem::Proc(std::string(buf), body, "addq  $" + std::to_string(frame->frame_size) + ",%rsp\nretq\n");
    }



}