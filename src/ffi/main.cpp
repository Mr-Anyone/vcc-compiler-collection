//===----------------------------------------------------------------------===//
// This utility parses an LLVM IR file and prints out the function declarations
// in a simplified "compilation-like" format, mapping LLVM types to 
// the programming language types (e.g., `i32` to `int`, `float` to `float`, etc).
//===----------------------------------------------------------------------===//
#include <cstdlib>
#include <iostream>

#include "type.h"
#include "llvm/IR/DerivedTypes.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>

llvm::cl::opt<std::string> input_filename(llvm::cl::Positional,
                                          llvm::cl::Required,
                                          llvm::cl::desc("<input filename>"));
struct Context {
  std::unique_ptr<llvm::Module> module;
};

void printIntegerType(llvm::IntegerType *type) {
  switch (type->getBitWidth()) {
  case 1:
    llvm::outs() << "bool";
    break;
  case 8:
    llvm::outs() << "char";
    break;
  case 16:
    llvm::outs() << "short";
    break;
  case 32:
    llvm::outs() << "int";
    break;
  case 64:
    llvm::outs() << "long";
    break;
  default:
    llvm::errs() << "cannot convert the following type \n";
    type->dump();
    std::exit(-1);
  }
}
void printCompType(llvm::Type *type) {
  switch (type->getTypeID()) {
  case llvm::Type::PointerTyID:
    llvm::outs() << "ptr void";
    break;
  case llvm::Type::IntegerTyID:
    printIntegerType(llvm::cast<llvm::IntegerType>(type));
    break;
  case llvm::Type::FloatTyID:
    llvm::outs() << "float";
    break;
  default:
    llvm::errs() << "cannot cast type into comp type.";
    type->dump();
    std::exit(-1);
  }
}

void printCompDecl(llvm::Function *function) {
  llvm::outs() << "external function " << function->getName() << " gives ";
  printCompType(function->getReturnType());
  llvm::outs() << " [";
  
  char start_of_alphabet =  'a';
  // printing the function argument list
  for (auto it = function->arg_begin(), ie = function->arg_end(); it != ie;
       ++it) {
      printCompType(it->getType());
      llvm::outs() <<" " << start_of_alphabet++ << ", ";
  }

  llvm::outs() << "]\n";
}

void printAsCompType(Context *context) {
  // cast the type
  for (auto it = context->module->begin(), ie = context->module->end();
       it != ie; ++it) {
    assert(llvm::isa<llvm::Function>(it) &&
           "we assume that they are all function for now");
    llvm::Function *function = llvm::cast<llvm::Function>(it);
    printCompDecl(function);
  }
}

int main(int argc, char *argv[]) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  llvm::SMDiagnostic diag;
  llvm::LLVMContext llvm_context;
  Context context{llvm::parseIRFile(input_filename, diag, llvm_context)};

  printAsCompType(&context);

  return 0;
}
