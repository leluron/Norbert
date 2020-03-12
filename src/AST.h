#pragma once

#include <memory>
#include <vector>
#include <map>

class File;
class Stat;
class Exp;
class Lexp;
class LexpSuffix;

using namespace std;

using statp = shared_ptr<Stat>;
using expp  = shared_ptr<Exp >;
using lexpsuffixp = shared_ptr<LexpSuffix>;

class LexpSuffix {
public:
    virtual ~LexpSuffix() {}
};

class IndexSuffix : public LexpSuffix {
public:
    IndexSuffix(expp i) : i(i) {}
    expp i;
};

class Lexp {

public:
    Lexp(string name, lexpsuffixp suffix) : name(name), suffix(suffix) {}
    string name;
    lexpsuffixp suffix;
};

class File {

public:
    vector<statp> stats;
};

class Stat {

public:
    virtual ~Stat() {}
};

class AssignStat : public Stat {

public:
    AssignStat(Lexp left, expp right) : left(left), right(right) {}
    Lexp left;
    expp right;
};

class FuncCallStat : public Stat {

public:
    FuncCallStat(expp func, vector<expp> args) : func(func), args(args) {}
    expp func;
    vector<expp> args;
};

class WhileStat : public Stat {

public:
    WhileStat(expp cond, statp body) : cond(cond), body(body) {}
    expp cond;
    statp body;
};

class IfStat : public Stat {

public:
    IfStat(expp cond, statp then, statp els) : cond(cond), then(then), els(els) {}
    expp cond;
    statp then;
    statp els;
};

class BlockStat : public Stat {

public:
    BlockStat(vector<statp> stats) : stats(stats) {}
    vector<statp> stats;
};

class ReturnStat : public Stat {

public:
    ReturnStat(expp ret) : ret(ret) {}
    expp ret;
};

class Exp {

public:
    virtual ~Exp() {}
};

class IntExp : public Exp {

public:
    IntExp(int value) : value(value) {}
    int value;
};

class FloatExp : public Exp {

public:
    FloatExp(float value) : value(value) {}
    float value;
};

class StringExp : public Exp {

public:
    StringExp(string value) : value(value) {}
    string value;
};

class IdExp : public Exp {

public:
    IdExp(string name) : name(name) {}
    string name;
};

class FuncCallExp : public Exp {

public:
    FuncCallExp(expp func, vector<expp> args) : func(func), args(args) {}
    expp func;
    vector<expp> args;
};

class TernaryExp : public Exp {

public:
    TernaryExp(expp cond, expp then, expp els) : cond(cond), then(then), els(els) {}
    expp cond, then, els;
};

class ListExp : public Exp {
public:
    ListExp(vector<expp> elements) : elements(elements) {}
    vector<expp> elements;
};

class IndexExp : public Exp {
public:
    IndexExp(expp left, expp index) : left(left), index(index) {}
    expp left;
    expp index;
};