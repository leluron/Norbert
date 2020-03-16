#pragma once

#include <stack>
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <functional>

#define WORD uint64_t

enum Type : int32_t {
    /* Name      desc */
    Nil = 0,
    Int,      
    Float,     
    String,    // ptr to str const in code or str in heap
    Pointer,   // ptr to anywhere
    Function,  // ptr to {code ptr, num_args}
    Closure,   // ptr to {code ptr, num_args, num_captured, value...}
    List,      // ptr to {num_elements, value...}
    Tuple,     // ptr to {num_elements, value...}
    Map        // ptr to {num_pairs, (value, value)...}
};

enum Instruction : int32_t {
    /* Name        i1           stack                     desc  */
    Noop = 0,   // 
    LoadInt,    // int       - () -> int
    LoadFloat,  // float     - () -> float
    LoadStr,    // strptr    - () -> string
    LoadVarAddr,// int       - () -> ptr 
    LoadVar,    // int       - () -> value
    LoadMem,    //           - (ptr) -> value
    StoreMem,   //           - (ptr, value) ->
    StoreVar,   // int       - (value) ->
    Call,       // ptr       - () ->
    CallExt,    // int       - () ->
    Pop,        //           - (value) ->
    Return,     //           - () ->
    IfJump,     // ptr       - (int) ->
    IfNJump,    // ptr       - (int) ->
    Jump,       // ptr       - () ->
    Not,        //           - (int) -> value
    And,        //           - (int, int) -> value
    Or,         //           - (int, int) -> value
    Usub,       //           - (value)        -> value
    Mul,        //           - (value, value) -> value 
    Div,        //           - (value, value) -> value
    Mod,        //           - (int, int) -> int
    Add,        //           - (value, value) -> value
    Sub,        //           - (value, value) -> value
    Lteq,       //           - (int, int) -> int
    Lt,         //           - (int, int) -> int
    Gt,         //           - (int, int) -> int
    Gteq,       //           - (int, int) -> int
    Eq,         //           - (int, int) -> int
    Neq,        //           - (int, int) -> int
    Inc,        // index     - (value)        ->

    ListCreate,    // size   - (value...)   -> list
    ListAccessPtr, //        - (list, int)  -> ptr
    ListAccess,    //        - (list, int)  -> value
    ListLength,    //        - (list)       -> int

    TupleCreate,    // size   - (value...)     -> tuple
    TupleConcat,    //        - (tuple, tuple) -> tuple
    TupleAccessPtr, // int    - (tuple) -> ptr
    TupleAccess,    // int    - (tuple) -> value

    FunctionCreate, // ptr, size - () -> function
    FunctionCall,   //           - (function, value...) -> value|closure 

    ClosureCreate,  // ptr       - (value...) -> closure
    ClosureCall,    //           - (closure, value...) -> value|closure

    MapCreate,      // size  - (value...) -> map
    MapAdd,         //       - (map, value, value) -> map
    MapAccessPtr,   //       - (map, value) -> ptr
    MapAccess,      //       - (map, value) -> value

};

enum ReservedFuncs : uint32_t {
    Printf, 
};

using vmcode = std::vector<WORD>;

class VirtualMachine {
public:
    VirtualMachine(std::ostream &o) : out(o) {}
    void load(vmcode program) {
        memcpy(memory, program.data(), program.size()*sizeof(WORD));
    }
    
    void step();
    void run();
    
    const static WORD CODE_SIZE          = 1 << 12;
    const static WORD LOCAL_VARS_SIZE    = 1 << 5;
    const static WORD MAX_STACK_SIZE     = 1 << 6;
    const static WORD MAX_OP_STACK_SIZE  = 1 << 8;
    const static WORD TOTAL_SIZE         = 1 << 20;

    const static WORD CODE_START         = 0;
    const static WORD STACK_START        = CODE_START + CODE_SIZE;
    const static WORD ADDR_STACK_START   = STACK_START + LOCAL_VARS_SIZE*MAX_STACK_SIZE;
    const static WORD OP_STACK_START     = ADDR_STACK_START + MAX_STACK_SIZE;
    const static WORD HEAP_START         = OP_STACK_START + MAX_OP_STACK_SIZE;

    const static WORD CODE_END           = STACK_START;
    const static WORD STACK_END          = ADDR_STACK_START;
    const static WORD ADDR_STACK_END     = OP_STACK_START;
    const static WORD OP_STACK_END       = HEAP_START;
    const static WORD HEAP_END           = TOTAL_SIZE;

private:
    uint32_t PC = CODE_START;
    uint32_t allocStart = HEAP_START;
    WORD* memory = new WORD[TOTAL_SIZE];
    std::map<uint32_t, void (VirtualMachine::*)()> stdlib = {
        { Printf, &VirtualMachine::printf},
    };
    int stackFrame = 0;
    int opStackFrame = 0;

    bool isPrim(Type t) { return t==Nil || t==Int || t==Float || t==String;}
    void newStack(WORD addr=-1);
    WORD popStack();
    void pushOpStack(WORD v);
    WORD popOpStack();
    WORD getStackPtr(int index);


    uint32_t alloc(int size);
    void vmfree(uint32_t addr);

    using binopint   = std::function<int32_t(int32_t, int32_t)>;
    using binopfloat = std::function<float(float, float)>;

    void binOp(binopint fi, binopfloat ff);
    void binOpRel(binopint fi, binopfloat ff);

    void printOpStack();
    void printStack();

    std::ostream &out;

    void printf();
    void list_create(int size);
    void list_access_ptr();
    void list_access();
    void list_length();

    void list_concat(int32_t d, int32_t d2);
    void list_add(int32_t d, uint64_t v);
};