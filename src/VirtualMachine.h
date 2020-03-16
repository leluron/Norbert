#pragma once

#include <stack>
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <functional>

#define WORD uint32_t
#define DWORD uint64_t
#define PTR uint32_t

#define ENDPC 0xffffff

enum Type : int8_t {
    /* Name      desc */
    Nil = 0,
    Int,      
    Float,     
    String,    // ptr to str const in code or str in heap
    Pointer,   // ptr to anywhere
    Closure,   // ptr to {code ptr, num_captured, value...}
    List,      // ptr to {num_elements, type, value...}
    Tuple,     // ptr to {num_elements, value...}
    Map        // ptr to {num_pairs, (value, value)...}
};

enum Instruction : int8_t {
    /* Name        i1           stack                     desc  */
    Noop = 0,   // 
    LoadInt,    // ptr       - () -> int
    LoadFloat,  // ptr       - () -> float
    LoadStr,    // ptr       - () -> string
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
    const static WORD ADDR_STACK_START   = STACK_START + LOCAL_VARS_SIZE*MAX_STACK_SIZE*2;
    const static WORD OP_STACK_START     = ADDR_STACK_START + MAX_STACK_SIZE;
    const static WORD HEAP_START         = OP_STACK_START + MAX_OP_STACK_SIZE*2;

    const static WORD CODE_END           = STACK_START;
    const static WORD STACK_END          = ADDR_STACK_START;
    const static WORD ADDR_STACK_END     = OP_STACK_START;
    const static WORD OP_STACK_END       = HEAP_START;
    const static WORD HEAP_END           = TOTAL_SIZE;

private:
    PTR PC = CODE_START;
    PTR allocStart = HEAP_START;
    WORD* memory = new WORD[TOTAL_SIZE];
    std::map<ReservedFuncs, void (VirtualMachine::*)()> stdlib = {
        { Printf, &VirtualMachine::printf},
    };
    int stackFrame = 0;
    int opStackFrame = 0;

    bool isPrim(Type t) { return t==Nil || t==Int || t==Float || t==String;}
    void newStack(PTR addr=ENDPC);
    PTR popStack();
    void pushOpStack(DWORD v);
    DWORD popOpStack();
    PTR getStackPtr(int index);

    void setDword(PTR addr, DWORD v);
    DWORD getDword(PTR addr);

    PTR alloc(int size);
    void vmfree(PTR addr);

    using binopint   = std::function<int32_t(int32_t, int32_t)>;
    using binopfloat = std::function<float(float, float)>;

    void binOp(binopint fi, binopfloat ff);
    void binOpRel(binopint fi, binopfloat ff);

    void printOpStack();
    void printStack();

    void gc();

    std::ostream &out;

    void printf();
    void list_create(int size);
    void list_access_ptr();
    void list_access();
    void list_length();

    void list_concat(PTR d, PTR d2);
    void list_add(PTR d, DWORD v);
};