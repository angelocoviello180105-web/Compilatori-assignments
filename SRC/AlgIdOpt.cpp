#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/DCE.h"

using namespace llvm;

//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {


// New PM implementation
struct AlgIdOpt: PassInfoMixin<AlgIdOpt> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  
  
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

    // definizioni variabili ausiliarie
    std::vector<Instruction*> deadCode;
    // ------------

    for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
      BasicBlock &B = *Iter;

      for (Instruction &inst : B) {
        
        if(inst.isBinaryOp()) {
          
          // CASISTICA (X+0) | (X*1) 
          if(ConstantInt *C = dyn_cast<ConstantInt>(inst.getOperand(1))) {
           
            if( !(C->isZero() || C->isOne()) ) {
              // CASISTICA (0+X) | (1*X) 
              if (ConstantInt *C = dyn_cast<ConstantInt>(inst.getOperand(0))) {
                if( (inst.getOpcode() == Instruction::Add && C->isZero()) || (inst.getOpcode() == Instruction::Mul && C->isOne()) ) {
                  // outs() << "Istruzione : " << inst << "\n";
                  // outs() <<  "identità di moltiplicazione o addizione\n";
                  inst.replaceAllUsesWith(inst.getOperand(1));
                  //l'istruzione vecchia viene aggiunta al vettore di istruzioni da eliminare
                  deadCode.push_back(&inst);
                }
              }
            
            } else if( (inst.getOpcode() == Instruction::Add && C->isZero()) || (inst.getOpcode() == Instruction::Mul && C->isOne()) ) {
              // outs() << "Istruzione : " << inst << "\n";
              // outs() <<  "identità di moltiplicazione o addizione\n";
              inst.replaceAllUsesWith(inst.getOperand(0));
              //l'istruzione vecchia viene aggiunta al vettore di istruzioni da eliminare
              deadCode.push_back(&inst);
            }
          }
        }
      }
    }

    //rimozione delle istruzioni non utilizzate
    for (Instruction *inst : deadCode) {
      inst->eraseFromParent();
    }

    return PreservedAnalyses::all();
  }
  
  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getAlgIdOptPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "AlgIdOpt", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback( [](StringRef Name, FunctionPassManager &FPM,ArrayRef<PassBuilder::PipelineElement>)  //Using these callbacks, callers can parse both a single pass name, as well as entire sub-pipelines, and populate the PassManager instance accordingly. 
              {
                if (Name == "algId-opt") {
                  FPM.addPass(AlgIdOpt());
                  //FPM.addPass(DCEPass());
                  return true;
                }
                return false;
              });
          }
        };
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize TestPass when added to the pass pipeline on the
// command line, i.e. via '-passes=test-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getAlgIdOptPluginInfo();
}

  
