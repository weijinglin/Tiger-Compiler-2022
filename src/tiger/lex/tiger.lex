%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE

digits [0-9]+
digit  [0-9]

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */

"while" {adjust(); return Parser::WHILE;}

"for" {adjust(); return Parser::FOR;}

"to" {adjust(); return Parser::TO;}

"break" {adjust(); return Parser::BREAK;}

"let" {adjust(); return Parser::LET;}

"in" {adjust(); return Parser::IN;}

"end" {adjust(); return Parser::END;}

"function" {adjust();return Parser::FUNCTION;}

"var" {adjust(); return Parser::VAR;}

"type" {adjust(); return Parser::TYPE;}

"if" {adjust(); return Parser::IF;}

"then" {adjust();return Parser::THEN;}

"else" {adjust(); return Parser::ELSE;}

"do" {adjust(); return Parser::DO;}

"of" {adjust(); return Parser::OF;}

"nil" {adjust(); return Parser::NIL;}

"," {adjust(); return Parser::COMMA;}

":" {adjust(); return Parser::COLON;}

";" {adjust(); return Parser::SEMICOLON;}

"(" {adjust(); return Parser::LPAREN;}

")" {adjust(); return Parser::RPAREN;}

"[" {adjust(); return Parser::LBRACK;}

"]" {adjust(); return Parser::RBRACK;}

"{" {adjust(); return Parser::LBRACE;}

"}" {adjust(); return Parser::RBRACE;}

"." {adjust(); return Parser::DOT;}

"+" {adjust(); return Parser::PLUS;}

"-" {adjust(); return Parser::MINUS;}

"*" {adjust(); return Parser::TIMES;}

"/" {adjust(); return Parser::DIVIDE;}

"=" {adjust(); return Parser::EQ;}

"<>" {adjust(); return Parser::NEQ;}

"<" {adjust(); return Parser::LT;}

"<=" {adjust(); return Parser::LE;}

">" {adjust(); return Parser::GT;}

">=" {adjust(); return Parser::GE;}

"&" {adjust(); return Parser::AND;}

"|" {adjust(); return Parser::OR;}

":=" {adjust(); return Parser::ASSIGN;}

{digits} {adjust(); return Parser::INT;}

\"              {
                    more();
                    begin(StartCondition__::STR);
                }

<STR>{
    \"          {
                    begin(StartCondition__::INITIAL);
                    adjust();
                    add_Pos();
                    un_count = 0;
                    if(matched() == "\n"){
                        add_Pos();
                    }
                    if(matched().length() == 2){
                        setMatched("");
                    }
                    else{
                        string_buf_ = matched().substr(1,matched().length()-2);
                        if(string_buf_.compare("\\n") == 0){
                            setMatched("\n");
                        }else{
                            setMatched(string_buf_);
                        }
                    }
                    return Parser::STRING;
                }

    "\\n"     {
                std::string char_list = matched().substr(0,matched().length()-2);
                std::string new_line = "\n";
                char_list = char_list + new_line;
                setMatched(char_list);
                more();
                un_count++;
            }
    
    "\\t"   {
                std::string char_list = matched().substr(0,matched().length()-2);
                std::string tew_line = "\t";
                char_list = char_list + tew_line;
                setMatched(char_list);
                more();
                un_count++;
            }

    "\\\"" {
                std::string char_list = matched().substr(0,matched().length()-2);
                std::string tew_line = "\"";
                char_list = char_list + tew_line;
                setMatched(char_list);
                more();
                un_count++;
            }

    "\\\\"  {
                std::string char_list = matched().substr(0,matched().length()-2);
                std::string tew_line = "\\";
                char_list = char_list + tew_line;
                setMatched(char_list);
                more();
                un_count++;
            } 

    "\\"{digit}{digit}{digit}  {
                                    std::string char_list = matched().substr(matched().length() - 3, matched().length());
                                    
                                    char new_char = 'a' - 97 + str_to_int(char_list);
                                    std::string all_string = matched();
                                    all_string[matched().length() - 4] = new_char;
                                    all_string = all_string.substr(0,matched().length() - 3);
                                    setMatched(all_string);
                                    more();
                                    un_count += 3;
                                }

    "\\^".      {
                    std::string char_list = matched().substr(matched().length() - 1, matched().length());

                    char new_char = char_list[0];

                    new_char = new_char - 64;

                    std::string mat_string = matched().substr(0, matched().length() - 2);

                    mat_string[matched().length() - 3] = new_char;
                    setMatched(mat_string);
                    more();
                    un_count+=2;
                }

    "\\"        {
                    more();
                    std::string mat_string = matched().substr(0,matched().length()-1);
                    setMatched(mat_string);
                    un_count++;
                    begin(StartCondition__::IGNORE);
                }

    .       more();
}

<IGNORE>{
    [ \t\n] {
        more();
        std::string mat_string = matched().substr(0,matched().length()-1);
        un_count++;
        setMatched(mat_string);
    } 

    "\\" {
        more();
        std::string mat_string = matched().substr(0,matched().length()-1);
        setMatched(mat_string);
        un_count++;
        begin(StartCondition__::STR);
    }
}

"/*"            {
                    adjust();
                    comment_level_ += 1;
                    begin(StartCondition__::COMMENT);
                }

<COMMENT>{
    "/*"        {
                    adjust();
                    comment_level_ += 1;
                }

    "*/"        {
                    adjust();
                    comment_level_ -= 1;
                    if(comment_level_ == 1){
                        begin(StartCondition__::INITIAL);
                    }
                }

    .       {adjust();}
    "\n"    {adjust();}
}

[a-zA-Z][a-zA-Z0-9_]* {adjust(); return Parser::ID;}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
