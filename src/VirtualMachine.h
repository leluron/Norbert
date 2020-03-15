#pragma once

#include <stack>
#include <map>
#include <vector>
#include <iostream>
#include <cstring>
#include <functional>

#define WORD uint64_t

enum Type : int32_t {
    Nil = 0, Int, Float, String, Pointer, Function, Closure, List, Tuple, Map
};

enum Instruction : int32_t {
    Noop = 0,
    LoadInt, LoadFloat, LoadStr,
    LoadVar, LoadMem,
    StoreVar, StoreMem,
    Alloc,
    Free,
    Call,
    CallExt,
    Return,
    IfJump,
    Jump,
    Pop,
    Not,
    And,
    Or,
    Usub,  
    Mul, 
    Div, 
    Mod,
    Add, 
    Sub, 
    Lteq, 
    Lt, 
    Gt, 
    Gteq, 
    Eq, 
    Neq, 
};

enum ReservedFuncs : uint32_t {
    Printf, 
    ListCreate, ListDelete, ListResize, ListAccessPtr
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
        { ListCreate, &VirtualMachine::list_create},
        { ListDelete, &VirtualMachine::list_delete},
        { ListResize, &VirtualMachine::list_resize},
        { ListAccessPtr, &VirtualMachine::list_access_ptr}
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

    std::ostream &out;

    void printf();
    void list_create();
    void list_delete();
    void list_resize();
    void list_access_ptr();

};