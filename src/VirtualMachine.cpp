#include "VirtualMachine.h"
#include <tuple>

using namespace std;

float asfloat(int32_t a) {
    return *(float*)(&a);
}

int32_t asint(float a) {
    return *(int32_t*)&a;
}

uint64_t makeValue(Type t, int32_t d) {
    return uint64_t(t) << 32 | d;
}

tuple<Type, int32_t> extract(uint64_t a) {
    return make_tuple(
        Type((a>>32) & 0xffffffff),
        int32_t((a >> 0) & 0xffffffff));
}

uint32_t asPtr(int32_t a) {
    return *(uint32_t*)&a;
}

tuple<Instruction, int32_t> decode(uint64_t i) {
    return make_tuple(
        Instruction((i>>32) & 0xffffffff),
        int32_t((i >> 0) & 0xffffffff));
}

void VirtualMachine::step() {
    if (PC >= memory.size()) return;

    Instruction i0; int32_t i1;
    tie(i0, i1) = decode(memory[PC]);
    PC++;

    uint64_t v;
    Type t,t2; int32_t d, d2;

    switch (i0) {
        case Noop: break;
        case LoadInt: {
            operandStack.push(makeValue(Int, i1)); break;
        }
        case LoadFloat: {
            operandStack.push(makeValue(Float, i1)); break;
        }
        case LoadPtr: {
            operandStack.push(makeValue(Pointer, i1)); break;
        }
        case LoadVar: {
            operandStack.push(localVarStack.top()[i1]); break;
        }
        case LoadMem : {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Pointer) throw runtime_error("Can't read from memory from a non-pointer");
            operandStack.push(memory[asPtr(d)]);
            break;
        }
        case StoreMem: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Pointer) throw runtime_error("Can't write to memory with a non-pointer");
            v = operandStack.top(); operandStack.pop();
            memory[asPtr(d)] = v;
            break;
        }
        case StoreVar: {
            v = operandStack.top(); operandStack.pop();
            localVarStack.top()[i1] = v;
            break;
        }
        case Alloc: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Int) throw runtime_error("Can't allocate with size of non-int");
            auto p = RAM;
            RAM += d;
            operandStack.push(makeValue(Pointer, p));
            break;
        }
        case Free : {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Pointer) throw runtime_error("Can't free from a non-pointer");
            break;
        }
        case Call: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Funcptr) throw runtime_error("Can't call a non-function pointer");

            addressStack.push(PC);
            PC = asPtr(d);
            localVarStack.push({});
            break;
        }
        case CallExt: {
            auto func = stdlib[asPtr(i1)];
            (this->*func)();
            break;
        }
        case Return: {
            PC = addressStack.top();
            addressStack.pop();
            localVarStack.push({});
            break;
        }
        case IfJump: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Int) throw runtime_error("Can't evaluate a non-int");

            if (d) PC = asPtr(i1);
            break;
        }
        case Jump: {
            PC = asPtr(i1); break;
        }
        case Not: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            if (t != Int) throw runtime_error("Can't `not` with non-int");
            operandStack.push(makeValue(Int, !d));
            break;
        }
        case And: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            if (t != Int || t2 != Int) throw runtime_error("Can't `and` with non-int");
            operandStack.push(makeValue(Int, d && d2));
            break;
        }
        case Or: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            if (t != Int || t2 != Int) throw runtime_error("Can't `and` with non-int");
            operandStack.push(makeValue(Int, d || d2));
            break;
        }
        case Usub: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            int32_t neg = -d;
            if (t == Float) neg = asint(-asfloat(d));
            operandStack.push(makeValue(t, neg));
            break;
        }
        case Mul: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)*asfloat(d2));
                else res = asint(asfloat(d)*(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d*asfloat(d2));
                else res = d*d2;
            }
            operandStack.push(makeValue((t==Int && t2==Int)?Int:Float, res));
            break;
        }
        case Div: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)/asfloat(d2));
                else res = asint(asfloat(d)/(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d/asfloat(d2));
                else res = d/d2;
            }
            operandStack.push(makeValue((t==Int && t2==Int)?Int:Float, res));
            break;
        }
        case Mod: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            if (t2 == Float && t == Float) throw runtime_error("Cant' `mod` with non-int");
            operandStack.push(makeValue(t, d%d2));
            break;
        }
        case Add: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)+asfloat(d2));
                else res = asint(asfloat(d)+(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d+asfloat(d2));
                else res = d+d2;
            }
            operandStack.push(makeValue((t==Int && t2==Int)?Int:Float, res));
            break;
        }
        case Sub: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)-asfloat(d2));
                else res = asint(asfloat(d)-(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d-asfloat(d2));
                else res = d-d2;
            }
            operandStack.push(makeValue((t==Int && t2==Int)?Int:Float, res));
            break;
        }
        case Lteq: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)<=asfloat(d2));
                else res = asint(asfloat(d)<=(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d<=asfloat(d2));
                else res = d<=d2;
            }
            operandStack.push(makeValue(Int, res));
            break;
        }
        case Lt: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)<asfloat(d2));
                else res = asint(asfloat(d)<(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d<asfloat(d2));
                else res = d<d2;
            }
            operandStack.push(makeValue(Int, res));
            break;
        }
        case Gt: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)>asfloat(d2));
                else res = asint(asfloat(d)>(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d>asfloat(d2));
                else res = d>d2;
            }
            operandStack.push(makeValue(Int, res));
            break;
        }
        case Gteq: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)>=asfloat(d2));
                else res = asint(asfloat(d)>=(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d>=asfloat(d2));
                else res = d>=d2;
            }
            operandStack.push(makeValue(Int, res));
            break;
        }
        case Eq: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)==asfloat(d2));
                else res = asint(asfloat(d)==(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d==asfloat(d2));
                else res = d==d2;
            }
            operandStack.push(makeValue(Int, res));
            break;
        }
        case Neq: {
            tie(t,d) = extract(operandStack.top()); operandStack.pop();
            tie(t2,d2) = extract(operandStack.top()); operandStack.pop();
            int32_t res;
            if (t == Float) {
                if (t2 == Float) res = asint(asfloat(d)!=asfloat(d2));
                else res = asint(asfloat(d)!=(float)d2);
            } else {
                if (t2 == Float) res = asint((float)d!=asfloat(d2));
                else res = d!=d2;
            }
            operandStack.push(makeValue(Int, res));
            break;
        }
        default: break;
    }
}

void VirtualMachine::run() {

    localVarStack.push({});
    while (true) {
        if (PC >= memory.size()) return;
        auto instr0 = memory[PC];
        if (instr0 == End) return;
        step();
    }
}

void VirtualMachine::printf() {
    Type t; int32_t v; tie(t,v) = extract(operandStack.top());
    operandStack.pop();

    if (t != Pointer) throw "Invalid argument 0 for printf";

    auto addr = asPtr(v);

    while (true) {
        char c = (char)memory[addr];
        if (c == '\0') break;
        if (c == '%') {
            addr++;
            char c1 = (char)memory[addr];
            if (c1 == 'd' || c1 == 'i') {
                tie(t,v) = extract(operandStack.top()); operandStack.pop();
                if (t == Int || t == Pointer || t == Funcptr) out << v;
                else if (t == Float) out << (int)asfloat(v);
            } else if (c1 == 'f' || c1 == 'g') {
                tie(t,v) = extract(operandStack.top()); operandStack.pop();
                if (t == Int || t == Pointer || t == Funcptr) out << (float)v;
                else if (t == Float) out << asfloat(v);
            }
        } else {
            out << c;
        }
        addr++;
    }
}