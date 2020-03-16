#include "VirtualMachine.h"

#include <tuple>

using namespace std;

float asfloat(int32_t a) {
    return *(float*)(&a);
}

int32_t asint(float a) {
    return *(int32_t*)&a;
}

WORD makeValue(Type t, int32_t d) {
    return WORD(t) << 32 | d;
}

tuple<Type, int32_t> extract(WORD a) {
    return make_tuple(
        Type((a>>32) & 0xffffffff),
        int32_t((a >> 0) & 0xffffffff));
}

uint32_t asPtr(int32_t a) {
    return *(uint32_t*)&a;
}

tuple<Instruction, int32_t> decode(WORD i) {
    return make_tuple(
        Instruction((i>>32) & 0xffffffff),
        int32_t((i >> 0) & 0xffffffff));
}

void VirtualMachine::step() {
    Instruction i0; int32_t i1;
    tie(i0, i1) = decode(memory[PC]);
    PC++;

    WORD v;
    Type t,t2; int32_t d, d2;

    switch (i0) {
        case Noop: break;
        case LoadInt: pushOpStack(makeValue(Int, i1)); break;
        case LoadFloat: pushOpStack(makeValue(Float, i1)); break;
        case LoadStr: pushOpStack(makeValue(String, i1)); break;
        case LoadVarAddr: 
            pushOpStack(makeValue(Pointer, getStackPtr(i1))); break;
        case LoadVar:
            pushOpStack(memory[getStackPtr(i1)]); break;
        case LoadMem :
            tie(t,d) = extract(popOpStack());
            if (t != Pointer) throw runtime_error("Can't read from memory from a non-pointer");
            pushOpStack(memory[asPtr(d)]);
            break;
        case StoreMem:
            v = popOpStack();
            tie(t,d) = extract(popOpStack());
            if (t != Pointer) throw runtime_error("Can't write to memory with a non-pointer");
            memory[asPtr(d)] = v;
            break;
        case StoreVar:
            memory[getStackPtr(i1)] = popOpStack(); break;
        case Call: {
            auto addr = asPtr(i1);
            if (addr >= CODE_START && addr < CODE_END) { 
                newStack(PC);
                PC = addr;
            }
            break;
        }
        case CallExt: {
            auto func = stdlib[asPtr(i1)];
            (this->*func)();
            break;
        }
        case Return: PC = popStack(); break;
        case Pop: popOpStack(); break;
        case IfJump: {
            tie(t,d) = extract(popOpStack());
            if (t != Int) throw runtime_error("Can't evaluate a non-int");
            if (d) PC = asPtr(i1);
            break;
        }
        case IfNJump: {
            tie(t,d) = extract(popOpStack());
            if (t != Int) throw runtime_error("Can't evaluate a non-int");
            if (!d) PC = asPtr(i1);
            break;
        }
        case Jump: PC = asPtr(i1); break;
        case Not: {
            tie(t,d) = extract(popOpStack());
            if (t != Int) throw runtime_error("Can't `not` with non-int");
            pushOpStack(makeValue(Int, !d));
            break;
        }
        case And: {
            tie(t,d) = extract(popOpStack());
            tie(t2,d2) = extract(popOpStack());
            if (t != Int || t2 != Int) throw runtime_error("Can't `and` with non-int");
            pushOpStack(makeValue(Int, d && d2));
            break;
        }
        case Or: {
            tie(t,d) = extract(popOpStack());
            tie(t2,d2) = extract(popOpStack());
            if (t != Int || t2 != Int) throw runtime_error("Can't `and` with non-int");
            pushOpStack(makeValue(Int, d || d2));
            break;
        }
        case Usub: {
            tie(t,d) = extract(popOpStack());
            int32_t neg = -d;
            if (t == Float) neg = asint(-asfloat(d));
            pushOpStack(makeValue(t, neg));
            break;
        }
        case Mul:
            binOp([](float a, float b){return a*b;},[](float a, float b){return a*b;});break;
        case Div:
            binOp([](float a, float b){return a/b;},[](float a, float b){return a/b;});break;
        case Mod: {
            tie(t,d) = extract(popOpStack());
            tie(t2,d2) = extract(popOpStack());
            if (t2 == Float && t == Float) throw runtime_error("Cant' `mod` with non-int");
            pushOpStack(makeValue(t, d%d2));
            break;
        }
        case Add: {
            tie(t,d) = extract(popOpStack());
            auto v = popOpStack();
            tie(t2,d2) = extract(v); 

            if (t == List) {
                if (t2 == List) list_concat(d,d2);
                else list_add(d, v);
            } else {
                int32_t res;
                if (t == Float) {
                    if (t2 == Float) res = asint(asfloat(d)+asfloat(d2));
                    else res = asint(asfloat(d)+(float)d2);
                } else {
                    if (t2 == Float) res = asint((float)d+asfloat(d2));
                    else res = d+d2;
                }
                pushOpStack(makeValue((t==Int && t2==Int)?Int:Float, res));
            }
            break;
        }
        case Sub:
            binOp([](int32_t a, int32_t b){return a-b;},[](float a, float b){return a-b;});break;
        case Lteq:
            binOpRel([](int32_t a, int32_t b){return a<=b;},[](float a, float b){return a<=b;});break;
        case Lt:
            binOpRel([](int32_t a, int32_t b){return a<b;},[](float a, float b){return a<b;});break;
        case Gt:
            binOpRel([](int32_t a, int32_t b){return a>b;},[](float a, float b){return a>b;});break;
        case Gteq:
            binOpRel([](int32_t a, int32_t b){return a>=b;},[](float a, float b){return a>=b;});break;
        case Eq:
            binOpRel([](int32_t a, int32_t b){return a==b;},[](float a, float b){return a==b;});break;
        case Neq:
            binOpRel([](int32_t a, int32_t b){return a!=b;},[](float a, float b){return a!=b;}); break;
        case Inc:
            tie(t,d) = extract(memory[getStackPtr(i1)]);
            tie(t2,d2) = extract(popOpStack());
            if (t == Int && t2 == Int) {
                memory[getStackPtr(i1)] = d+d2;
            } else throw runtime_error("inc supported only for ints");
        case ListCreate: list_create(i1); break;
        case ListAccessPtr: list_access_ptr(); break;
        case ListAccess: list_access(); break;
        case ListLength: list_length(); break;
        default: throw runtime_error("Unsupported opcode");
    }
}

void VirtualMachine::run() {
    newStack();
    while (PC != -1 && PC < CODE_END) {
        step();
    }
}

uint32_t VirtualMachine::alloc(int size) {
    auto l = allocStart;
    allocStart += size;
    if (allocStart > TOTAL_SIZE) {
        throw runtime_error("Memory overflow");
    }
    return l;
}

void VirtualMachine::vmfree(uint32_t addr) {
    if (addr < HEAP_START || addr >= HEAP_END)
        throw runtime_error("Can't free invalid pointer");
}

void VirtualMachine::binOp(binopint fi, binopfloat ff) {
    Type t,t2; int32_t d,d2;
    tie(t,d) = extract(popOpStack());
    tie(t2,d2) = extract(popOpStack());
    int32_t res;
    if (t == Float) {
        if (t2 == Float) res = asint(ff(asfloat(d),asfloat(d2)));
        else res = asint(ff(asfloat(d),(float)d2));
    } else {
        if (t2 == Float) res = asint(ff((float)d,asfloat(d2)));
        else res = fi(d,d2);
    }
    pushOpStack(makeValue((t==Int && t2==Int)?Int:Float, res));
}

void VirtualMachine::binOpRel(binopint fi, binopfloat ff) {
    Type t,t2; int32_t d,d2;
    tie(t,d) = extract(popOpStack());
    tie(t2,d2) = extract(popOpStack());
    int32_t res;
    if (t == Float) {
        if (t2 == Float) res = asint(ff(asfloat(d),asfloat(d2)));
        else res = asint(ff(asfloat(d),(float)d2));
    } else {
        if (t2 == Float) res = asint(ff((float)d,asfloat(d2)));
        else res = fi(d,d2);
    }
    pushOpStack(makeValue(Int, res));
}

void VirtualMachine::printf() {
    Type t; int32_t v; tie(t,v) = extract(popOpStack());
    if (t != String) throw "Invalid argument 0 for printf";

    auto addr = asPtr(v);

    while (true) {
        char c = (char)memory[addr];
        if (c == '\0') break;
        if (c == '%') {
            addr++;
            char c1 = (char)memory[addr];
            if (c1 == 'd' || c1 == 'i') {
                tie(t,v) = extract(popOpStack());
                if (t == Float) out << (int)asfloat(v);
                else  out << v;
            } else if (c1 == 'f' || c1 == 'g') {
                tie(t,v) = extract(popOpStack());
                if (t == Float) out << asfloat(v);
                else out << (float)v;
            }
        } else {
            out << c;
        }
        addr++;
    }
}

void VirtualMachine::newStack(WORD addr) {
    memory[ADDR_STACK_START + stackFrame] = addr;
    stackFrame += 1;
    if (stackFrame == MAX_STACK_SIZE) throw runtime_error("Stack overflow");
}

WORD VirtualMachine::popStack() {
    if (stackFrame == 0) throw runtime_error("Stack underflow");
    stackFrame -= 1;
    return memory[ADDR_STACK_START + stackFrame];
}

void VirtualMachine::pushOpStack(WORD v) {
    if (opStackFrame == MAX_OP_STACK_SIZE-1) throw runtime_error("Operand stack overflow");
    memory[OP_STACK_START+(opStackFrame++)] = v;
}

WORD VirtualMachine::popOpStack() {
    if (opStackFrame == 0) throw runtime_error("Operand stack underflow");
    return memory[OP_STACK_START+(--opStackFrame)];
}

WORD VirtualMachine::getStackPtr(int index) {
    if (index < 0 || index >= LOCAL_VARS_SIZE) throw runtime_error("Invalid stack slot");
    return STACK_START + (stackFrame-1)*LOCAL_VARS_SIZE+index;
}

void VirtualMachine::list_create(int size) {
    auto addr = alloc(1+size);
    memory[addr] = size;
    for (int i=0;i<size;i++)
        memory[addr+1+i] = popOpStack();
    pushOpStack(makeValue(List, addr));
}

void VirtualMachine::list_concat(int32_t d, int32_t d2) {
    int len1 = memory[d];
    int len2 = memory[d2];
    int new_size = len1+len2;
    auto addr = alloc(1+new_size);
    memory[addr] = new_size;
    memcpy(&memory[addr+1]     , &memory[d +1], len1*sizeof(uint64_t));
    memcpy(&memory[addr+1+len1], &memory[d2+1], len1*sizeof(uint64_t));
    pushOpStack(makeValue(List, addr));
}

void VirtualMachine::list_add(int32_t d, uint64_t v) {
    int len1 = memory[d];
    int new_size = len1+1;
    auto addr = alloc(1+new_size);
    memory[addr] = new_size;
    memcpy(&memory[addr+1], &memory[d+1], len1*sizeof(uint64_t));
    memory[addr+1+len1] = v;
    pushOpStack(makeValue(List, addr));
}

void VirtualMachine::list_access_ptr() {
    Type t,t2; int32_t d,d2;
    tie(t2,d2) = extract(popOpStack());
    tie(t,d) = extract(popOpStack());
    if (t != List) throw runtime_error("Can't access from non-list");
    if (t2 != Int) throw runtime_error("Can't index into list with non-int");
    int len = memory[d];

    if (d2 < 0 || d2 >= len) throw runtime_error("Access out of bounds");
    pushOpStack(makeValue(Pointer, d+1+d2));
}

void VirtualMachine::list_access() {
    list_access_ptr();
    int32_t d;
    tie (ignore,d) = extract(popOpStack());
    pushOpStack(memory[d]);
}

void VirtualMachine::list_length() {
    Type t; int32_t d;
    tie(t,d) = extract(popOpStack());
    pushOpStack(makeValue(Int, memory[d]));
}


void printValue(WORD v) {
    Type t; int32_t d;
    tie(t,d) = extract(v);
    if (t == Nil) cout << "nil";
    else if (t == Int) cout << "int";
    else if (t == Float) cout << "float";
    else if (t == Function) cout << "func";
    else if (t == Pointer) cout << "ptr";
    else if (t == Closure) cout << "closure";
    else if (t == List) cout << "list";
    else if (t == Tuple) cout << "tuple";
    else if (t == Map) cout << "map";

    cout << " ";
    if (t == Float) cout << asfloat(d);
    else cout << d;
    cout << endl;
}

void VirtualMachine::printOpStack() {
    cout << "[" << endl;
    for (int i=opStackFrame-1;i>=0;i--) {
        cout << "\t"; printValue(memory[OP_STACK_START+i]); cout << endl;
    }
    cout << "]" << endl;
}

void VirtualMachine::printStack() {
    cout << "[" << endl;
    for (int i=0;i<LOCAL_VARS_SIZE;i++) {
        cout << "\t"; printValue(memory[STACK_START+LOCAL_VARS_SIZE*(stackFrame-1)+i]); cout << endl;
    }
    cout << "]" << endl;
}