#pragma once

#include <stack>
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <functional>
#include <memory>

#define WORD uint32_t
#define DWORD uint64_t
#define PTR uint32_t

#define ENDPC 0xffffff

using vmcode = std::vector<WORD>;

struct vmunit {
    vmcode code;
    std::map<std::string, std::pair<PTR, int>> funcs;
};

enum Type : int8_t {
    /* Name      desc */
    Nil = 0,
    Int,      
    Float,     
    String,    // ptr to str const in code or str in heap
    Pointer,   // ptr to anywhere
    Closure,   // ptr to {code ptr, num_args, num_captured, value...}
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
    ClosureCall,    // numargs   - (closure, value...) -> value|closure

    MapCreate,      // size  - (value...) -> map
    MapAdd,         //       - (map, value, value) -> map
    MapAccessPtr,   //       - (map, value) -> ptr
    MapAccess,      //       - (map, value) -> value

};

enum ReservedFuncs : uint32_t {
    Printf, 
};

class VirtualMachine {
public:
    VirtualMachine(std::ostream &o) : out(o) {}
    void load(vmunit program) {
        memcpy(memory, program.code.data(), program.code.size()*sizeof(WORD));
        for (auto f : program.funcs) {
            funcNames[f.first] = f.second.first;
            funcNumArgs[f.second.first] = f.second.second;
        }
    }
    
    void step();
    void run(std::string funcname);
    
    const static int CODE_SIZE          = 1 << 14;
    const static int LOCAL_VARS_SIZE    = 1 << 5;
    const static int MAX_STACK_SIZE     = 1 << 6;
    const static int MAX_OP_STACK_SIZE  = 1 << 8;
    const static int HEAP_SIZE          = 1 << 16;

    const static PTR CODE_START         = 0;
    const static PTR STACK_START        = CODE_START + CODE_SIZE;
    const static PTR ADDR_STACK_START   = STACK_START + LOCAL_VARS_SIZE*MAX_STACK_SIZE*2;
    const static PTR OP_STACK_START     = ADDR_STACK_START + MAX_STACK_SIZE;
    const static PTR HEAP_START         = OP_STACK_START + MAX_OP_STACK_SIZE*2;

    const static PTR CODE_END           = STACK_START;
    const static PTR STACK_END          = ADDR_STACK_START;
    const static PTR ADDR_STACK_END     = OP_STACK_START;
    const static PTR OP_STACK_END       = HEAP_START;
    const static PTR HEAP_END           = HEAP_START+HEAP_SIZE;
    const static PTR TOTAL_SIZE         = HEAP_END;

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

    void list_concat(PTR d, PTR d2);
    void list_add(PTR d, DWORD v);

    const int SMALLEST_ALLOC = 2;

    const int NULLPTR = 0xffffff;

    struct HeapTree {
        PTR start = HEAP_START;
        int size = HEAP_SIZE;
        bool allocated = false;
        std::shared_ptr<HeapTree> left = nullptr;
        std::shared_ptr<HeapTree> right = nullptr;
        HeapTree *parent = nullptr;
    };

    HeapTree heaproot;

    // ALLOC
    PTR alloc(int size);
    PTR alloc(HeapTree *tree, int size);

    // FREE
    void vmfree(PTR ptr);
    void merge(HeapTree *tree);
    HeapTree* find(HeapTree *tree, PTR ptr);

    // memory management
    // deep free on values
    // free previous value on assign (store_mem, store_var)
    // free all values on stack when popping the stack

    void deepFree(DWORD value);

    // functions

    std::map<std::string, PTR> funcNames;
    std::map<PTR, int> funcNumArgs;
};