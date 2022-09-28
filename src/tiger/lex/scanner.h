#ifndef TIGER_LEX_SCANNER_H_
#define TIGER_LEX_SCANNER_H_

#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <string>
#include <sstream>

#include "scannerbase.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/parse/parserbase.h"

class Scanner : public ScannerBase {
public:
  Scanner() = delete;
  explicit Scanner(std::string_view fname, std::ostream &out = std::cout)
      : ScannerBase(std::cin, out), comment_level_(1), char_pos_(1), un_count(0),
        errormsg_(std::make_unique<err::ErrorMsg>(fname)) {
    switchStreams(errormsg_->infile_, out);
  }

  /**
   * Output an error
   * @param message error message
   */
  void Error(int pos, std::string message, ...) {
    va_list ap;
    va_start(ap, message);
    errormsg_->Error(pos, message, ap);
    va_end(ap);
  }

  /**
   * Getter for `tok_pos_`
   */
  [[nodiscard]] int GetTokPos() const { return errormsg_->GetTokPos(); }

  /**
   * Transfer the ownership of `errormsg_` to the outer scope
   * @return unique pointer to errormsg
   */
  [[nodiscard]] std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

  int lex();

private:
  int comment_level_;
  std::string string_buf_;
  int char_pos_;
  int un_count;
  std::unique_ptr<err::ErrorMsg> errormsg_;

  /**
   * NOTE: do not change all the funtion signature below, which is used by
   * flexc++ internally
   */
  int lex__();
  int executeAction__(size_t ruleNr);

  void print();
  void preCode();
  void postCode(PostEnum__ type);
  void adjust();
  void adjustStr();
  void debug();
  void add_Pos();
  int str_to_int(std::string charlist);
  // void fix_string(std::string input);
};

inline int Scanner::str_to_int(std::string charlist){
  std::stringstream ss;
  ss << charlist;
  int res;
  ss >> res;
  return res;
}

inline void Scanner::debug(){
  std::cout << "debug" << std::endl;
}

// inline void Scanner::fix_string(std::string input){
//   int index = 0;
//   char *charlist = new char[input.length()+1];
//   for(int i = 0;i < input.length();++i){
//     if(input[i] != "\\"){
//       switch (input[i+1])
//       {
//         /* code */
//         case "n":
//         {
//           charlist[index] = '\n';
//           index++;
//           i++;
//           break;
//         }
//         case "t":
//         {
//           charlist[index] = '\t';
//           index++;
//           i++;
//           break;
//         }
//         case "^":
//         {
//           if(input[i+2] == 'C'){
//             charlist[index] = '\^C';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'O'){
//             charlist[index] = '\^O';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'M'){
//             charlist[index] = '\^M';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'P'){
//             charlist[index] = '\^P';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'I'){
//             charlist[index] = '\^I';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'L'){
//             charlist[index] = '\^L';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'E'){
//             charlist[index] = '\^E';
//             index++;
//             i+=2;
//           }else if(input[i+2] == 'R'){
//             charlist[index] = '\^R';
//             index++;
//             i+=2;
//           }
//           break;
//         }
//         case "\"":
//         {
//           charlist[index] = '\"';
//           index++;
//           i++;
//           break;
//         }

//         case "\\":{
//           charlist[index] = '\\';
//         }

      
//         default:
//           break;
//       }
//     }
//   }
// }

inline int Scanner::lex() { return lex__(); }

inline void Scanner::preCode() {
  // Optionally replace by your own code
}

inline void Scanner::postCode(PostEnum__ type) {
  // Optionally replace by your own code
}

inline void Scanner::print() { print__(); }

inline void Scanner::adjust() {
  errormsg_->tok_pos_ = char_pos_;
  char_pos_ += length();
}

inline void Scanner::add_Pos(){
  char_pos_+=un_count;
}

inline void Scanner::adjustStr() { char_pos_ += length(); }

#endif // TIGER_LEX_SCANNER_H_
