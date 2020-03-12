grammar Norbert;

file: stat* EOF;

stat
  : lexp '=' exp #assignstat
  | exp '(' explist? ')' #funccallstat
  | 'while' exp stat    #whilestat
  | 'if' exp stat ('elseif' exp stat)* ('else' els=stat)?       #ifstat
  | '{' stat* '}' #blockstat
  | 'return' exp? #returnstat
  ;

lexp : ID;

exp
  : 'true'                            #trueexp
  | 'false'                           #falseexp
  | inte                              #intexp
  | FLOAT                             #floatexp
  | STRING                            #stringexp
  | ID                                #idexp
  | exp '(' explist? ')'             #funccallexp
  | op=('-' | 'not') exp              #unaryexp
  | exp op=('*' | '/' | '%') exp      #multiplicativeexp
  | exp op=('+' | '-') exp            #additiveexp
  | exp op=('<=' | '<' | '>' | '>=') exp #relationexp
  | exp op=('==' | '!=' ) exp            #comparisonexp
  | exp op='and' exp                     #andexp
  | exp op='or' exp                      #orexp
  | exp 'if' exp 'else' exp           #ternaryexp
  | '(' exp ')'                       #parenexp
  ;

inte : INT | HEX;

explist: exp (',' exp)*;


ID
    : [a-zA-Z_] [a-zA-Z_0-9]*
    ;

INT
    : [0-9]+
    ;

FLOAT
    : [0-9]+ '.' [0-9]* 
    | '.' [0-9]+ 
    ;

HEX
  : '0' [xX] [0-9a-fA-F]+;

STRING
  : '"' (~('"'))* '"'
  | '\'' (~('\''))* '\''
  ;

COMMENT
    : '//' ~[\r\n]* -> skip
    ;

SPACE
    : [ \t\r\n] -> skip
    ;

OTHER
    : . 
    ;