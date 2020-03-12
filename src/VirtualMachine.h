#pragma once

#include <stack>
#include <map>
#include <vector>
#include <iostream>

enum Type : int32_t {
    Int = 0, Float, Pointer, Funcptr, List
};

enum Instruction : int32_t {
    Noop = 0,
    LoadInt, LoadFloat, LoadPtr,
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
    End
};

enum ReservedFuncs {
    Printf, 
    ListCreate, ListDelete, ListResize, ListAccessPtr
};

using vmcode = std::vector<uint64_t>;

class VirtualMachine {
public:
    VirtualMachine(std::ostream &o) : out(o) {}
    void setSize(size_t size) {
        this->memory.resize(size);
    }
    void load(vmcode program) {
        auto size = memory.size();
        memory.assign(program.begin(), program.end());
        if (size > program.size()) {
            this->memory.resize(size);
        }
        RAM = program.size();
    }
    
    void step();
    void run();
    
    const static uint64_t LOCAL_VARS_SIZE = 32;

private:
    uint32_t PC = 0;
    uint32_t RAM = 0;
    vmcode memory;
    std::stack<uint64_t> operandStack;
    std::stack<uint32_t> addressStack;
    std::stack<std::array<uint64_t, LOCAL_VARS_SIZE>> localVarStack;
    std::map<uint32_t, void (VirtualMachine::*)()> stdlib = {
        { Printf, &VirtualMachine::printf},
        { ListCreate, &VirtualMachine::list_create},
        { ListDelete, &VirtualMachine::list_delete},
        { ListResize, &VirtualMachine::list_resize},
        { ListAccessPtr, &VirtualMachine::list_access_ptr}
    };

    uint32_t alloc(int size);
    void vmfree(uint32_t addr);

    std::ostream &out;

    void printf();
    void list_create();
    void list_delete();
    void list_resize();
    void list_access_ptr();

};