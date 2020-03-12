#pragma once

#include <stack>
#include <map>
#include <vector>
#include <iostream>

enum Type : int32_t {
    Int = 0, Float, Pointer, Funcptr
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
    uint64_t PC = 0;
    uint64_t RAM = 0;
    vmcode memory;
    std::stack<uint64_t> operandStack;
    std::stack<uint32_t> addressStack;
    std::stack<std::array<uint64_t, LOCAL_VARS_SIZE>> localVarStack;
    std::map<uint32_t, void (VirtualMachine::*)()> stdlib = {
        { Printf, &VirtualMachine::printf},
    };

    std::ostream &out;

    void printf();

};