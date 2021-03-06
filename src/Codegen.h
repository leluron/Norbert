#pragma once

#include "VirtualMachine.h"
#include "Assembler.h"

#include <map>

using namespace std;

class CodeGen {
public:
    vmunit gen(File f) {
        for (auto s : f.functions) {
            visit(s.first, s.second);
        }
        code << "return" << endl;
        /* DEBUG */
        cout << str.str() << endl;
        cout << constantsstr.str() << endl;
        /*       */
        return assemble(str.str() + constantsstr.str());
    }

    void visit(string name, Function f) {
        string lbl = name + "entry";
        funclbls[name] = lbl;

        locals.clear();
        localId = 0;
        for (auto a : f.args) {
            locals[a] = localId;
            localId += 1;
        }

        code << lbl << ": function " << name << " " << f.args.size() << endl;
        if (f.body) visit(f.body);
        else {
            visit(f.e);
            code << "return" << endl;
        }
    }

    void visit(statp sb) {
        if (auto s = dynamic_pointer_cast<AssignStat>(sb)) {
            if (auto l = dynamic_pointer_cast<LexpId>(s->left)) {
                auto name = l->name;
                auto it = locals.find(name);
                int id;
                if (it == locals.end()) {
                    locals[name] = localId;
                    id = localId;
                    localId += 1;
                } else id = it->second;
                visit(s->right);
                code << "store_var " << id << endl;
            } else {
                visit(s->left);
                visit(s->right);
                code << "store_mem" << endl;
            }
        } else if (auto s = dynamic_pointer_cast<FuncCallStat>(sb)) {
            visit(expp(new FuncCallExp(s->func, s->args)));
        } else if (auto s = dynamic_pointer_cast<WhileStat>(sb)) {
            auto startlbl = newlabel();
            auto endlbl = newlabel();
            code << startlbl << ":" << endl;
            visit(s->cond);
            code << "ifnjump " << endlbl << endl;
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

    void visit(lexpp lexp) {
        if (auto l = dynamic_pointer_cast<LexpId>(lexp)) {
            visitLexpId(l);
        } else if (auto l = dynamic_pointer_cast<LexpIndex>(lexp)) {
            visit(l->l);
            code << "load_mem" << endl;
            visit(l->e);
            code << "list_access_ptr" << endl;
        }
    }

    void visitLexpId(shared_ptr<LexpId> l) {
        auto name = l->name;
        auto it = locals.find(name);
        int id;
        if (it == locals.end()) {
            throw runtime_error("Can't find local");
        } else id = it->second;
        code << "load_var_addr " << id << endl;
    }

    void visit(expp eb) {
        if (auto e = dynamic_pointer_cast<IntExp>(eb)) {
            auto it = intToLbl.find(e->value);
            string lbl;
            if (it == intToLbl.end()) {
                lbl = newlabel();
                intToLbl[e->value] = lbl;
                constants << lbl << ": " << e->value << endl;
            } else {
                lbl = intToLbl[e->value];
            }
            code << "load_int " << lbl << endl;
        } else if (auto e = dynamic_pointer_cast<FloatExp>(eb)) {
            auto it = floatToLbl.find(e->value);
            string lbl;
            if (it == floatToLbl.end()) {
                lbl = newlabel();
                floatToLbl[e->value] = lbl;
                constants << lbl << ": " << fixed << e->value << endl;
            } else {
                lbl = floatToLbl[e->value];
            }
            code << "load_float " << lbl << endl;
        } else if (auto e = dynamic_pointer_cast<StringExp>(eb)) {
            auto it = strToLbl.find(e->value);
            string lbl;
            if (it == strToLbl.end()) {
                lbl = newlabel();
                strToLbl[e->value] = lbl;
                constants << lbl << ": " << e->value << endl;
            } else {
                lbl = strToLbl[e->value];
            }
            code << "load_str " << lbl << endl;
        } else if (auto e = dynamic_pointer_cast<IdExp>(eb)) {
            auto it = locals.find(e->name);
            if (it == locals.end()) throw runtime_error("Can't find local or function");
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
                else if (n=="len") code << "list_length" << endl;
                // stdlib
                else {
                    auto it = funclbls.find(n);
                    if (it == funclbls.end()) code << "call_ext " << n << endl;
                    else code << "call " << it->second << endl;
                }
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
        } else if (auto e = dynamic_pointer_cast<ListExp>(eb)) {
            for (int i=e->elements.size()-1;i>=0;i--) visit(e->elements[i]);
            code << "list_create " << e->elements.size() << endl;
        } else if (auto e = dynamic_pointer_cast<IndexExp>(eb)) {
            visit(e->left);
            visit(e->index);
            code << "list_access" << endl;
        }
    }


private:

    string newlabel() {
        stringstream ss;
        ss << "lbl" << (lblId++);
        return ss.str();
    }

    int lblId = 0;

    map<string, string> funclbls;
    map<string, int32_t> locals;
    int localId = 0;
    stringbuf str;
    stringbuf constantsstr;
    ostream code{&str};
    ostream constants{&constantsstr};

    map<string, string> strToLbl;
    map<int, string> intToLbl;
    map<float, string> floatToLbl;
};