#include "context.h"
#include "driver.h"
#include "parser.h"
#include "util.h"

#include <iostream>
#include <llvm/IR/PassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

llvm::cl::opt<bool> print_ast("print-ast",
                              llvm::cl::desc("Whether to print syntax tree"));
llvm::cl::opt<bool> print_llvm("print-llvm",
                               llvm::cl::desc("Whether to print llvm"));
llvm::cl::opt<bool> O3("O3", llvm::cl::desc("Optimization Level"));
llvm::cl::opt<std::string> input_filename(llvm::cl::Positional,
                                          llvm::cl::Required,
                                          llvm::cl::desc("<input filename>"));

int main(int argc, char *argv[]) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  Parser parser = parseFile(input_filename.c_str());
  ContextHolder holder = parser.getHolder();
  Sema sema;

  for (ASTBase *tree : parser.getSyntaxTree()) {
    if (print_ast)
      tree->debugDump();

    dyncast<Statement>(tree)->codegen(holder);
  }

  // Create the analysis managers.
  // These must be declared in this order so that they are destroyed in the
  // correct order due to inter-analysis-manager references.
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  // Create the new pass manager builder.
  // Take a look at the PassBuilder constructor parameters for more
  // customization, e.g. specifying a TargetMachine or various debugging
  // options. std::string error; //
  // TheModule->setDataLayout(TheTargetMachine->createDataLayout());
  llvm::PassBuilder PB;

  // Register all the basic analyses with the managers.
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  llvm::ModulePassManager MPM =
      PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

  // Optimize the IR!
  if (O3)
    MPM.run(holder->module, MAM);

  if (print_llvm)
    holder->module.dump();

  return 0;
}

