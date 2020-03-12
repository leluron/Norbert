#include <iostream>

#include <antlr4-runtime/antlr4-runtime.h>
#include "parser/NorbertParser.h"
#include "parser/NorbertLexer.h"
#include "VirtualMachine.h"
#include "Assembler.h"
#include "ASTGen.h"

using namespace std;
using namespace antlr4;

int main() {

    ifstream stream("test.nor");
    ANTLRInputStream input(stream);
    NorbertLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    NorbertParser parser(&tokens);    
    NorbertParser::FileContext* tree = parser.file();

    ASTGen gen;
    auto ast = gen.gen(tree);

    VirtualMachine m(cout);
    m.load(assemble(R"(

load_int 0
store_var 0
load_int 28472
store_var 1

loop:
    load_var 0
    load_int 100
    gt
    load_int 1
    load_var 1
    neq
    and
    not
    ifjump endp

    load_var 1
    load_ptr str
    call_ext printf

    load_int 2
    load_var 1
    mod
    load_int 1
    eq
    ifjump cond1

    load_int 2
    load_var 1
    div
    jump after

cond1:
    load_int 3
    load_var 1
    mul
    load_int 1
    add

after:
    store_var 1
    load_int 1
    load_var 0
    add
    store_var 0
    jump loop

endp:
    end

str:
    "%d\n"

)"));

    cout << "VM output : " << endl;
    m.run();

    return 0;
}