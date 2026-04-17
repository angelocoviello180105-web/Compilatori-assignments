#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>



using namespace llvm;

//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {

  


      

// New PM implementation
struct MultLineOpt: PassInfoMixin<MultLineOpt> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  
  
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    
    // definizioni variabili ausiliarie

    std::vector<Instruction*> deadCode;
    // ------------
    
    for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
      BasicBlock &B = *Iter;
      
      
      
      
      //primo giro di popolamento
      for (Instruction &inst : B) {
        //debug
        outs() << inst << "\n";
        // ------------
        

        // CASO X = A / x = (a*x | x*a)/x, x numero intero
        if (inst.getOpcode() == Instruction::SDiv) {
          if (ConstantInt *n = dyn_cast<ConstantInt>(inst.getOperand(1))) {
            if (Instruction *A = dyn_cast<Instruction>(inst.getOperand(0))) {
              if (A->getOpcode() != Instruction::Mul){
                //debug
                outs() << "pattern rotto\t istruzione non opposta\n";
                // A = a * n

              } else if (n == A->getOperand(1)) {
                outs() << "il secondo operando di X corrisponde con il secondo operando di A (X=(a*n)/n)\n";
                
                inst.replaceAllUsesWith(A->getOperand(0));
                deadCode.push_back(&inst);
              
                // A = n * a 
              } else if (n == A->getOperand(0)) {
                outs() << "il secondo operando di X corrisponde con il primo operando di A (X=(n*a)/n)\n";
                
                inst.replaceAllUsesWith(A->getOperand(1));
                deadCode.push_back(&inst);
              }
            }
          }

        }



        else if(inst.getOpcode() == Instruction::Add) {
          // CASO X = A + n, con n numero intero, A istruzione (X = ( a-n )+n)
          if (ConstantInt *n = dyn_cast<ConstantInt>(inst.getOperand(1))) {
            
            if (Instruction *A = dyn_cast<Instruction>(inst.getOperand(0))) {
              // A = a - n
              if (A->getOpcode() != Instruction::Sub){
                //debug
                outs() << "pattern rotto\n";

              } else if (n == A->getOperand(1)) {
                //debug
                outs() << "il secondo operando di X corrisponde con il secondo operando di A (X=(a+n)-n)\n";
                
                inst.replaceAllUsesWith(A->getOperand(0));
                deadCode.push_back(&inst);
              }
            }
          }
          // CASO X = n + A, con n numero intero, A istruzione (X = n+( a-n ))
          if (ConstantInt *n = dyn_cast<ConstantInt>(inst.getOperand(0))) {
            
            if (Instruction *A = dyn_cast<Instruction>(inst.getOperand(1))) {
              // A = a - n
              if (A->getOpcode() != Instruction::Sub){
                //debug
                outs() << "pattern rotto\n";
                
              } else if (n == A->getOperand(1)) {
                //debug
                outs() << "il secondo operando di X corrisponde con il secondo operando di A (X=(a+n)-n)\n";
                
                inst.replaceAllUsesWith(A->getOperand(0));
                deadCode.push_back(&inst);
              }
            }
          }
          
        } else if (inst.getOpcode() == Instruction::Sub) {
          // CASO X = A - n = (a+n|n+a) - n 
          if (ConstantInt *n = dyn_cast<ConstantInt>(inst.getOperand(1))) {
            if (Instruction *A = dyn_cast<Instruction>(inst.getOperand(0))) {
              if (A->getOpcode() != Instruction::Add){
                //debug
                outs() << "pattern rotto\t istruzione non opposta\n";
                // A = a + n

              } else if (n == A->getOperand(1)) {
                outs() << "il secondo operando di X corrisponde con il secondo operando di A (X=(a+n)-n)\n";
                
                inst.replaceAllUsesWith(A->getOperand(0));
                deadCode.push_back(&inst);
              
                // A = n + a 
              } else if (n == A->getOperand(0)) {
                outs() << "il secondo operando di X corrisponde con il primo operando di A (X=(n+a)-n)\n";
                
                inst.replaceAllUsesWith(A->getOperand(1));
                deadCode.push_back(&inst);
              }
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
llvm::PassPluginLibraryInfo getMultLineOptPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "StrRedOpt", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback( [](StringRef Name, FunctionPassManager &FPM,ArrayRef<PassBuilder::PipelineElement>)  //Using these callbacks, callers can parse both a single pass name, as well as entire sub-pipelines, and populate the PassManager instance accordingly. 
              {
                if (Name == "mult-line-opt") {
                  FPM.addPass(MultLineOpt());
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
  return getMultLineOptPluginInfo();
}

  
