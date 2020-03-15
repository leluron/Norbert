#include "Assembler.h"
#include <antlr4-runtime/antlr4-runtime.h>
#include "parser/BytecodeParser.h"
#include "parser/BytecodeLexer.h"
#include "parser/BytecodeBaseVisitor.h"

using namespace std;
using namespace antlr4;

using addressmap = std::map<std::string, uint64_t>;

addressmap stdlib = {
    {"printf", Printf},
    {"list_create", ListCreate},
    {"list_delete", ListDelete},
    {"list_access_ptr", ListAccessPtr},
    {"list_resize", ListResize},
};

vector<int64_t> stringArrayToCode(std::string str) {
    vector<int64_t> s;
    for (int i=1;i<str.size()-1;i++) {
        char c = str[i];
        if (c == '\\') {
            c = str[++i];
            if (c == 'n') s.push_back('\n');
            else if (c == '\\') s.push_back('\\');
            else if (c == '"') s.push_back('"');
            else if (c == 'r') s.push_back('\r');
            else if (c == 't') s.push_back('\t');
            else if (c == 'v') s.push_back('\v');
        } else {
            s.push_back(c);
        }
    }
    s.push_back('\0');
    if (s.size()%2 == 1) s.push_back('\0');
    return s;
}

class LabelResolve : BytecodeBaseVisitor {
public:
    virtual antlrcpp::Any visitCode(BytecodeParser::CodeContext *ctx) override {
        a = 0;
        labels.clear();
        visitChildren(ctx);
        return labels;
    }

    virtual antlrcpp::Any visitInstr(BytecodeParser::InstrContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitLabel(BytecodeParser::LabelContext *ctx) override {
        labels[ctx->name()->getText()] = a;
        return nullptr;
    }

    virtual antlrcpp::Any visitOp(BytecodeParser::OpContext *ctx) override {
        if (ctx->stringarray()) return visit(ctx->stringarray());
        else a += 1;
        return nullptr;
    }

    virtual antlrcpp::Any visitStringarray(BytecodeParser::StringarrayContext *ctx) override {
        auto str = ctx->STRING()->getText();
        a += stringArrayToCode(str).size();
        return nullptr;
    }
private:
    uint64_t a;
    addressmap labels;
};

uint64_t makeInstruction(Instruction i0, int32_t i1) {
    return uint64_t(i0) << 32 | i1;
}

class Assembler : BytecodeBaseVisitor {
public:
    vmcode run(std::string assembly) {
        ANTLRInputStream input(assembly);
        BytecodeLexer lexer(&input);
        CommonTokenStream tokens(&lexer);
        BytecodeParser parser(&tokens);    
        BytecodeParser::CodeContext* tree = parser.code();

        auto labels = LabelResolve().visitCode(tree).as<addressmap>();
        this->addresses.insert(labels.begin(), labels.end());
        this->addresses.insert(stdlib.begin(), stdlib.end());
        code.clear();
        visitCode(tree);

        return code;
    }

    virtual antlrcpp::Any visitCode(BytecodeParser::CodeContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitInstr(BytecodeParser::InstrContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitLabel(BytecodeParser::LabelContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitOp(BytecodeParser::OpContext *ctx) override {
        if (ctx->opcode()) {
            string op = visit(ctx->opcode());
            Instruction i0 = Noop;
            if (op == "noop") i0 = Noop;
            else if (op == "load_int") i0 = LoadInt;
            else if (op == "load_float") i0 = LoadFloat;
            else if (op == "load_str") i0 = LoadStr;
            else if (op == "load_var") i0 = LoadVar;
            else if (op == "load_mem") i0 = LoadMem;
            else if (op == "store_var") i0 = StoreVar;
            else if (op == "store_mem") i0 = StoreMem;
            else if (op == "alloc") i0 = Alloc;
            else if (op == "free") i0 = Free;
            else if (op == "call") i0 = Call;
            else if (op == "call_ext") i0 = CallExt;
            else if (op == "return") i0 = Return;
            else if (op == "ifjump") i0 = IfJump;
            else if (op == "jump") i0 = Jump;
            else if (op == "not") i0 = Not;
            else if (op == "and") i0 = And;
            else if (op == "or") i0 = Or;
            else if (op == "usubi") i0 = Usub;
            else if (op == "mul") i0 = Mul;
            else if (op == "div") i0 = Div;
            else if (op == "mod") i0 = Mod;
            else if (op == "add") i0 = Add;
            else if (op == "sub") i0 = Sub;
            else if (op == "lteq") i0 = Lteq;
            else if (op == "lt") i0 = Lt;
            else if (op == "gt") i0 = Gt;
            else if (op == "gteq") i0 = Gteq;
            else if (op == "eq") i0 = Eq;
            else if (op == "neq") i0 = Neq;

            int32_t i1 = Noop;
            if (ctx->intliteral()) i1 = visit(ctx->intliteral());
            else if (ctx->floatliteral()) i1 = visit(ctx->floatliteral());
            else if (ctx->name()) i1 = addresses[visit(ctx->name())];

            code.push_back(makeInstruction(i0, i1));

        } else if (ctx->stringarray()) {
            vector<int64_t> s = visit(ctx->stringarray());
            code.insert(code.end(), s.begin(), s.end());
        } else if (ctx->intl) {
            code.push_back(visit(ctx->intl).as<int64_t>());
        } else if (ctx->floatl) {
            code.push_back(visit(ctx->floatl).as<int64_t>());
        }
        return nullptr;
    }

    virtual antlrcpp::Any visitIntliteral(BytecodeParser::IntliteralContext *ctx) override {
        stringstream ss;
        ss << ctx->INT()->getText();
        int32_t val;
        ss >> val;
        return val;
    }

    virtual antlrcpp::Any visitFloatliteral(BytecodeParser::FloatliteralContext *ctx) override {
        stringstream ss;
        ss << ctx->FLOAT()->getText();
        float val;
        ss >> val;
        return *(int32_t*)&val;
    }

    virtual antlrcpp::Any visitName(BytecodeParser::NameContext *ctx) override {
        return ctx->ID()->getText();
    }

    virtual antlrcpp::Any visitOpcode(BytecodeParser::OpcodeContext *ctx) override {
        return ctx->o->getText();
    }

    virtual antlrcpp::Any visitStringarray(BytecodeParser::StringarrayContext *ctx) override {
        auto str = ctx->STRING()->getText();
        return stringArrayToCode(str);
    }

    addressmap addresses;
    vmcode code;

};

vmcode assemble(string assembly) {
    return Assembler().run(assembly);
}