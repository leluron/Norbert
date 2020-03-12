#pragma once

#include <memory>
#include <vector>
#include <map>

struct File;
struct Stat;
struct Exp;
struct Lexp;

using namespace std;

using statp = shared_ptr<Stat>;
using expp  = shared_ptr<Exp >;

struct Lexp {
    string name;
};

struct File {
    vector<statp> stats;
};

struct Stat {
    ~Stat() {}
};

struct AssignStat : public Stat {
    AssignStat(Lexp left, expp right) : left(left), right(right) {}
    Lexp left;
    expp right;
};

struct FuncCallStat : public Stat {
    FuncCallStat(expp func, vector<expp> args) : func(func), args(args) {}
    expp func;
    vector<expp> args;
};

struct WhileStat : public Stat {
    WhileStat(expp cond, statp body) : cond(cond), body(body) {}
    expp cond;
    statp body;
};

struct IfStat : public Stat {
    IfStat(expp cond, statp then, statp els) : cond(cond), then(then), els(els) {}
    expp cond;
    statp then;
    statp els;
};

struct BlockStat : public Stat {
    BlockStat(vector<statp> stats) : stats(stats) {}
    vector<statp> stats;
};

struct BreakStat : public Stat {};

struct ReturnStat : public Stat {
    ReturnStat(expp ret) : ret(ret) {}
    expp ret;
};

struct Exp {
    ~Exp() {}
};

struct IntExp : public Exp {
    IntExp(int value) : value(value) {}
    int value;
};

struct FloatExp : public Exp {
    FloatExp(float value) : value(value) {}
    float value;
};

struct StringExp : public Exp {
    StringExp(string value) : value(value) {}
    string value;
};

struct IdExp : public Exp {
    IdExp(string name) : name(name) {}
    string name;
};

struct FuncCallExp : public Exp {
    FuncCallExp(expp func, vector<expp> args) : func(func), args(args) {}
    expp func;
    vector<expp> args;
};

struct TernaryExp : public Exp {
    TernaryExp(expp cond, expp then, expp els) : cond(cond), then(then), els(els) {}
    expp cond, then, els;
};