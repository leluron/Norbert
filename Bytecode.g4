grammar Bytecode;

code: instr* EOF;
instr
    : label* op
    ;

label: name ':';
op
    : 'function' funcname=name numargs=intliteral
    | opcode intliteral
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
    | 'load_str'
    | 'load_var_addr'
    | 'load_var'
    | 'load_mem'
    | 'store_mem'
    | 'store_var'
    | 'call'
    | 'call_ext'
    | 'pop'
    | 'return'
    | 'ifjump'
    | 'ifnjump'
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
    | 'inc'
    | 'list_create'
    | 'list_concat'
    | 'list_add'
    | 'list_access_ptr'
    | 'list_access'
    | 'list_length'
    | 'tuple_create'
    | 'tuple_concat'
    | 'tuple_access_ptr'
    | 'tuple_access'
    | 'closure_create'
    | 'closure_call'
    | 'map_create'
    | 'map_add'
    | 'map_access_ptr'
    | 'map_access'
    );

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