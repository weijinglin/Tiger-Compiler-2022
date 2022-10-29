# 文档

## 1, how to handle comments?

答：首先，识别第一个标志着注释开始的字符(/\*)，然后进入对应的StartCondition__(COMMENT)，在COMMENT状态中，如果还遇到(/\*)，则把comment_level_加1，遇到其他字符就跳过，遇到(\*/)的话就把对应的comment_level_减1，如果comment_level_等于1的话就退出COMMENT状态进入基础的INITIAL状态。

## 2， how to handle strings?

答：首先，遇到（\"）标志着字符串的开始，进入到STR的状态，在这个状态对于（\"）进行匹配，如果匹配，标志着字符串的结束，退出STR状态，进入INITIAL状态，当处于STR状态时，对于不同的转义字符进行匹配，匹配成功后先适用matched()函数获得匹配的子串，根据不同的匹配情况对子串进行修改（把被转义的\\去掉，和后面的字符组合成对应的转义字符）。由于在adjust调用之前setMatched会导致字符串的计数出错，于是在Scanner.h中定义了un_count变量，来记录没有被算入的position，这些变量会在最后匹配到(\")后调用add_Pos来用于调整到正确的位置，结束后将un_count赋值为0。来实现正确的计数。

## 3，error handling?

答：把错误匹配的规则放到最后，表示如果前面所有的规则都没有匹配，则准备报错，这条规则会匹配所有字符串。当匹配到这条规则代表着前面所有的规则都没有成功匹配。则报错。

## 4，end-of-file handling?

答：对于EOF采取的策略是不进行匹配，之前尝试过匹配，但是由于EOF后面也都是EOF，所以会导致无止境的匹配，而导致程序无法终止，所以采用不匹配的策略。