#pragma once

#include "VirtualMachine.h"
#include "Assembler.h"

#include <map>

using namespace std;

class CodeGen {
public:
    vmcode gen(File f) {
        for (auto s : f.stats) {
            visit(s);
        }
        code << "end" << endl;
        cout << str.str() << endl;
        cout << constantsstr.str() << endl;
        return assemble(str.str() + constantsstr.str());
    }

    void visit(statp sb) {
        if (auto s = dynamic_pointer_cast<AssignStat>(sb)) {
            auto name = s->left.name;
            auto it = locals.find(name);
            int id;
            if (it == locals.end()) {
                locals[name] = localId;
                id = localId;
                localId += 1;
            } else id = it->second;
            visit(s->right);
            code << "store_var " << id << endl;
        } else if (auto s = dynamic_pointer_cast<FuncCallStat>(sb)) {
            visit(expp(new FuncCallExp(s->func, s->args)));
        } else if (auto s = dynamic_pointer_cast<WhileStat>(sb)) {
            auto startlbl = newlabel();
            auto endlbl = newlabel();
            code << startlbl << ":" << endl;
            visit(s->cond);
            code << "not" << endl << "ifjump " << endlbl << endl;
            visit(s->body);
            code << "jump " << startlbl << endl;
            code << endlbl << ":" << endl;
        } else if (auto s = dynamic_pointer_cast<IfStat>(sb)) {
            auto condlbl = newlabel();
            auto endlbl = newlabel();
            visit(s->cond);
            code << "ifjump " << condlbl << endl;
            visit(s->els);
            code << "jump " << endlbl << endl;
            visit(s->then);
            code << endlbl << ":" << endl;
        } else if (auto s = dynamic_pointer_cast<BlockStat>(sb)) {
            for (auto s1 : s->stats) visit(s1);
        } else if (auto s = dynamic_pointer_cast<ReturnStat>(sb)) {
            visit(s->ret);
            code << "return" << endl;
        }
    }

    void visit(expp eb) {
        if (auto e = dynamic_pointer_cast<IntExp>(eb)) {
            code << "load_int " << e->value << endl;
        } else if (auto e = dynamic_pointer_cast<FloatExp>(eb)) {
            code << "load_float" << e->value << endl;
        } else if (auto e = dynamic_pointer_cast<StringExp>(eb)) {
            auto lbl = newlabel();
            constants << lbl << ": " << e->value << endl;
            code << "load_ptr " << lbl << endl;
        } else if (auto e = dynamic_pointer_cast<IdExp>(eb)) {
            auto it = locals.find(e->name);
            if (it == locals.end()) throw runtime_error("Can't find local");
            code << "load_var " << it->second << endl;
        } else if (auto e = dynamic_pointer_cast<FuncCallExp>(eb)) {
            for (int i=e->args.size()-1;i>=0;i--) {
                visit(e->args[i]);
            }
            if (auto n0 = dynamic_pointer_cast<IdExp>(e->func)) {
                auto n = n0->name;
                if (n=="-") {
                    if (e->args.size() == 1) code << "usub" << endl;
                    else code << "sub" << endl;
                } else if (n=="not" || n=="and" || n=="or") {
                    code << n << endl;
                } else if (n=="*") code << "mul" << endl;
                else if (n=="/") code << "div" << endl;
                else if (n=="%") code << "mod" << endl;
                else if (n=="+") code << "add" << endl;
                else if (n=="<=") code << "lteq" << endl;
                else if (n=="<") code << "lt" << endl;
                else if (n==">") code << "gt" << endl;
                else if (n==">=") code << "gteq" << endl;
                else if (n=="==") code << "eq" << endl;
                else if (n=="!=") code << "neq" << endl;
                // stdlib
                else code << "call_ext " << n << endl;
            } else {
                //TODO implement
                throw;
            }
        } else if (auto e = dynamic_pointer_cast<TernaryExp>(eb)) {
            auto condlbl = newlabel();
            auto endlbl = newlabel();
            visit(e->cond);
            code << "ifjump " << condlbl << endl;
            visit(e->els);
            code << "jump " << endlbl << endl;
            code << condlbl << ":" << endl;
            visit(e->then);
            code << endlbl << ":" << endl;
        }
    }


private:

    string newlabel() {
        stringstream ss;
        ss << "lbl" << (lblId++);
        return ss.str();
    }

    int lblId = 0;

    map<string, int32_t> locals;
    int localId = 0;
    stringbuf str;
    stringbuf constantsstr;
    ostream code{&str};
    ostream constants{&constantsstr};
};