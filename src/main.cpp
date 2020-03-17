#include <iostream>

#include <antlr4-runtime/antlr4-runtime.h>
#include "parser/NorbertParser.h"
#include "parser/NorbertLexer.h"
#include "VirtualMachine.h"
#include "Assembler.h"
#include "ASTGen.h"
#include "Codegen.h"

using namespace std;
using namespace antlr4;

int main(int argc, char **argv) {

    string filename = (argc==2)?argv[1]:"test.nor";
    ifstream stream(filename);
    ANTLRInputStream input(stream);
    NorbertLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    NorbertParser parser(&tokens);    
    NorbertParser::FileContext* tree = parser.file();

    ASTGen gen;
    auto ast = gen.gen(tree);

    auto code = CodeGen().gen(ast);

    VirtualMachine m(cout);
    m.load(code);

    cout << "VM output : " << endl;
    m.run("main");

    return 0;
}