#include "VirtualMachine.h"

#include <tuple>

using namespace std;

float asfloat(WORD a) {
    return *(float*)(&a);
}

WORD asint(float a) {
    return *(WORD*)&a;
}

DWORD makeValue(Type t, WORD d) {
    return DWORD(t) << 32 | d;
}

tuple<Type, int32_t> extract(DWORD a) {
    WORD b = (a >> 0) & 0xffffffff;
    return make_tuple(
        Type((a>>32) & 0xffffffff),
        *(int32_t*)&b);
}

PTR asPtr(int32_t a) {
    return a & 0xffffff;
}

tuple<Instruction, uint32_t> decode(WORD i) {
    return make_tuple(
        Instruction((i>>24) & 0xff),
        uint32_t((i >> 0) & 0xffffff));
}

void VirtualMachine::step() {
    Instruction i0; uint32_t i1;
    tie(i0, i1) = decode(memory[PC]);
    PC++;

    DWORD v;
    Type t,t2; int32_t d, d2;

    switch (i0) {
        case Noop: break;
        case LoadInt: pushOpStack(makeValue(Int, memory[i1])); break;
        case LoadFloat: pushOpStack(makeValue(Float, memory[i1])); break;
        case LoadStr: pushOpStack(makeValue(String, i1)); break;
        case LoadVarAddr: 
            pushOpStack(makeValue(Pointer, getStackPtr(i1))); break;
        case LoadVar:
            pushOpStack(getDword(getStackPtr(i1))); break;
        case LoadMem :
            tie(t,d) = extract(popOpStack());
            if (t != Pointer) throw runtime_error("Can't read from memory from a non-pointer");
            pushOpStack(getDword(asPtr(d)));
            break;
        case StoreMem:
            v = popOpStack();
            tie(t,d) = extract(popOpStack());
            if (t != Pointer) throw runtime_error("Can't write to memory with a non-pointer");
            deepFree(getDword(asPtr(d)));
            setDword(asPtr(d), v);
            break;
        case StoreVar:
            deepFree(getStackPtr(i1));
            setDword(getStackPtr(i1), popOpStack()); break;
        case Call: {
            auto addr = asPtr(i1);
            if (addr >= CODE_START && addr < CODE_END) { 
                newStack(PC);
                PC = addr;
            }
            break;
        }
        case CallExt: {
            auto func = stdlib[(ReservedFuncs)i1];
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
            tie(t,d) = extract(getDword(getStackPtr(i1)));
            tie(t2,d2) = extract(popOpStack());
            if (t == Int && t2 == Int) {
                setDword(getStackPtr(i1),makeValue(Int, d+d2));
            } else throw runtime_error("inc supported only for ints");
            break;
        case ListCreate: list_create(i1); break;
        case ListAccessPtr: list_access_ptr(); break;
        case ListAccess: list_access(); break;
        case ListLength: list_length(); break;
        default: throw runtime_error("Unsupported opcode");
    }
}

void VirtualMachine::run() {
    newStack();
    while (PC != ENDPC && PC < CODE_END) {
        step();
    }
}

void VirtualMachine::binOp(binopint fi, binopfloat ff) {
    Type t,t2; int32_t d,d2;
    tie(t,d) = extract(popOpStack());
    tie(t2,d2) = extract(popOpStack());
    int32_t res;
    if (t == Nil || t2 == Nil) throw runtime_error("Can't binop with nil");
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
    if (t == Nil || t2 == Nil) throw runtime_error("Can't binop with nil");
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

void VirtualMachine::newStack(PTR addr) {
    memory[ADDR_STACK_START + stackFrame] = addr;
    stackFrame += 1;
    if (stackFrame == MAX_STACK_SIZE) throw runtime_error("Stack overflow");
}

PTR VirtualMachine::popStack() {
    if (stackFrame == 0) throw runtime_error("Stack underflow");
    stackFrame -= 1;
    return memory[ADDR_STACK_START + stackFrame];
}

void VirtualMachine::pushOpStack(DWORD v) {
    if (opStackFrame == MAX_OP_STACK_SIZE-1) throw runtime_error("Operand stack overflow");
    for (int i=0;i<LOCAL_VARS_SIZE;i++) deepFree(getStackPtr(i));

    setDword(OP_STACK_START+2*(opStackFrame++), v);
}

DWORD VirtualMachine::popOpStack() {
    if (opStackFrame == 0) throw runtime_error("Operand stack underflow");
    return getDword(OP_STACK_START+2*(--opStackFrame));
}

PTR VirtualMachine::getStackPtr(int index) {
    if (index < 0 || index >= LOCAL_VARS_SIZE) throw runtime_error("Invalid stack slot");
    return STACK_START + 2*((stackFrame-1)*LOCAL_VARS_SIZE+index);
}

void VirtualMachine::list_create(int size) {
    auto addr = alloc(1+size*2);
    memory[addr] = size;
    for (int i=0;i<size;i++)
        setDword(addr+1+i*2, popOpStack());
    pushOpStack(makeValue(List, addr));
}

void VirtualMachine::list_concat(PTR d, PTR d2) {
    int len1 = memory[d];
    int len2 = memory[d2];
    int new_size = len1+len2;
    auto addr = alloc(1+new_size*2);
    memory[addr] = new_size;
    memcpy(&memory[addr+1]       , &memory[d +1], len1*sizeof(DWORD));
    memcpy(&memory[addr+1+len1*2], &memory[d2+1], len2*sizeof(DWORD));
    pushOpStack(makeValue(List, addr));
}

void VirtualMachine::list_add(PTR d, DWORD v) {
    int len1 = memory[d];
    int new_size = len1+1;
    auto addr = alloc(1+new_size*2);
    memory[addr] = new_size;
    memcpy(&memory[addr+1], &memory[d+1], len1*sizeof(DWORD));
    setDword(addr+1+len1*2, v);
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
    pushOpStack(makeValue(Pointer, d+1+d2*2));
}

void VirtualMachine::list_access() {
    list_access_ptr();
    int32_t d;
    tie (ignore,d) = extract(popOpStack());
    pushOpStack(getDword(d));
}

void VirtualMachine::list_length() {
    Type t; int32_t d;
    tie(t,d) = extract(popOpStack());
    pushOpStack(makeValue(Int, memory[d]));
}


void printValue(DWORD v) {
    Type t; int32_t d;
    tie(t,d) = extract(v);
    if (t == Nil) cout << "nil";
    else if (t == Int) cout << "int";
    else if (t == Float) cout << "float";
    else if (t == String) cout << "string";
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
        cout << "\t"; printValue(getDword(OP_STACK_START+i*2)); cout << endl;
    }
    cout << "]" << endl;
}

void VirtualMachine::printStack() {
    cout << "[" << endl;
    for (int i=0;i<LOCAL_VARS_SIZE;i++) {
        cout << "\t"; printValue(getDword(STACK_START+2*(LOCAL_VARS_SIZE*(stackFrame-1)+i))); cout << endl;
    }
    cout << "]" << endl;
}
  
void VirtualMachine::setDword(PTR addr, DWORD v) {
    memcpy(&memory[addr], &v, sizeof(DWORD));
}

DWORD VirtualMachine::getDword(PTR addr) {
    DWORD v;
    memcpy(&v, &memory[addr], sizeof(DWORD));
    return v;
}

// ALLOC

PTR VirtualMachine::alloc(int size) {
    PTR a = alloc(&heaproot, size);
    if (a == NULLPTR) throw runtime_error("Memory full, can't allocate");
    return a;
}

PTR VirtualMachine::alloc(HeapTree *tree, int size) {
    if (tree->allocated) return NULLPTR;
    if (!tree->left) {
        if (size > tree->size/2 || size < SMALLEST_ALLOC) {
            tree->allocated = true;
            return tree->start;
        } else {
            tree->left  = shared_ptr<HeapTree>(new HeapTree{tree->start             , tree->size/2, false, nullptr, nullptr, tree});
            tree->right = shared_ptr<HeapTree>(new HeapTree{tree->start+tree->size/2, tree->size/2, false, nullptr, nullptr, tree});
        }
    }
    PTR a = alloc(tree->left.get(), size);
    if (a == NULLPTR) a = alloc(tree->right.get(), size);
    return a;
}

// FREE

void VirtualMachine::vmfree(PTR ptr) {
    auto tree = find(&heaproot, ptr);
    if (tree->start == tree->parent->start) tree->parent->left = nullptr;
    else tree->parent->right = nullptr;
    merge(tree->parent);
}

void VirtualMachine::merge(HeapTree *tree) {
    if (!tree) return;
    if (!tree->left && !tree->right) {
        auto l = tree->parent;
        if (tree->start == tree->parent->start)
            l->left = nullptr;
        else 
            l->right = nullptr;
        return merge(l);
    }
}

VirtualMachine::HeapTree* VirtualMachine::find(HeapTree *tree, PTR ptr) {
    if (tree->allocated) return tree;
    if (ptr < tree->start + tree->size/2) 
        return find(tree->left.get(), ptr);
    else
        return find(tree->right.get(), ptr);
}

// memory management
// deep free on values
// free previous value on assign (store_mem, store_var)
// free all values on stack when popping the stack

void VirtualMachine::deepFree(DWORD value) {
    Type t; int32_t d;
    tie(t,d) = extract(value);
    PTR p = asPtr(d);
    if (isPrim(t)) return;
    if (t == Pointer) {
        if (d > HEAP_START) {
            deepFree(getDword(p));
        }
    } else if (t == List || t == Tuple) {
        for (int i=0;i<memory[p];i++) {
            deepFree(getDword(p+i*2));
        }
    }
    // TODO implem for other types
    if (p >= HEAP_START) vmfree(p);
}