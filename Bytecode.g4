grammar Bytecode;

code: instr* EOF;
instr
    : label* op
    ;

label: name ':';
op
    : opcode intliteral
    | opcode floatliteral
    | opcode name
    | opcode
    | stringarray
    | intl=intliteral
    | floatl=floatliteral
    ;

intliteral: INT | HEX;
floatliteral: FLOAT;
name: ID;
stringarray: STRING;

opcode
    : o=('noop'
    | 'load_int'
    | 'load_float'
    | 'load_ptr'
    | 'load_var'
    | 'load_mem'
    | 'store_var'
    | 'store_mem'
    | 'alloc'
    | 'free'
    | 'call_ext'
    | 'call'
    | 'pop'
    | 'return'
    | 'ifjump'
    | 'jump'
    | 'not'
    | 'and'
    | 'or'
    | 'usub'
    | 'mul'
    | 'div'
    | 'mod'
    | 'add'
    | 'sub'
    | 'lteq'
    | 'lt'
    | 'gt'
    | 'gteq'
    | 'eq'
    | 'neq'
    | 'end')
    ;

ID
    : [a-zA-Z] [a-zA-Z_0-9]*
    ;

INT
    : [0-9]+
    ;

FLOAT
    : [0-9]+ '.' [0-9]* 
    | '.' [0-9]+ 
    ;

STRING
  : '"' (~('"'))* '"'
  | '\'' (~('\''))* '\''
  ;

HEX
  : '0' [xX] [0-9a-fA-F]+;

SPACE
    : [ \t\r\n] -> skip
    ;

OTHER
    : . 
    ;