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

  /** La funzione ritorna 2 se il secondo operando(getOperand(1)) è un intero, 1 se è il primo è intero, -1 se non è binaria, 0 altrimenti 
   in ogni caso tenta di registrare il valore intero in \param opInteger*/
   
  int getIntegerOperand(Instruction &inst, ConstantInt *&opInteger) {
    if (!inst.isBinaryOp()) { //controlla se l'istruzione inst sia binaria 
      
      return -1;
    
    } else if (ConstantInt *C = dyn_cast<ConstantInt>(inst.getOperand(1))) { // secondo Op intero
      
      opInteger = C;
      return 2;
      
    } else if (ConstantInt *C = dyn_cast<ConstantInt>(inst.getOperand(0))) { // primo Op intero
    
      opInteger = C;
      return 1;
    
    }
    
    return 0;
  
  }

  Instruction* strRedDiv(ConstantInt *C, Value *registerOperand, IntegerType *integerCtx) {
    
    return BinaryOperator::Create(Instruction::AShr,
      registerOperand, 
      ConstantInt::get(
        integerCtx,
        C->getValue().logBase2()));
        
  }

  /*
  Controlla se esiste una potenza di due nell'intorno ]integerOperand-maxDist ; integerOperand+maxDist[
  */
  int strReductionProbe(ConstantInt* integerOperand, int maxDist) {
    
    int dist=0;
    
    
    while(dist<maxDist) {
      if ((integerOperand->getValue()+dist).isPowerOf2()) {
        return dist;
      } else if ((integerOperand->getValue()-dist).isPowerOf2()) {
        return -dist;
      }
      dist++;
    }
    
    return dist;
    
  }
      

// New PM implementation
struct StrRedOpt: PassInfoMixin<StrRedOpt> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  
  
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    
    // definizioni variabili ausiliarie
    auto Integer32bitCtx = IntegerType::getInt32Ty(F.getContext());
    const int MAX_DISTANCE = 3; //Intorno massimo entro il quale cercare la potenza di due (3 = distanza massima di 2)
    std::vector<Instruction*> deadCode;
    // ------------
    
    for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
      BasicBlock &B = *Iter;
      
      
      // definizioni variabili ausiliarie
      ConstantInt *C;
      // ------------
      
      for (Instruction &inst : B) {

        
        // definizioni variabili ausiliarie
        int hasIntegerOp = getIntegerOperand(inst, C);
        // ------------
        
        
        if (hasIntegerOp == 2){ // istruzione del tipo %reg, int : con questo pattern dove posso eseguire sia la strRedDiv che strRedMul
          
          
          if(// 1. DIVISIONE DIVENTA SHR
            (inst.getOpcode() == Instruction::SDiv 
            || inst.getOpcode() == Instruction::FDiv 
            || inst.getOpcode() == Instruction::UDiv) && C->getValue().isPowerOf2()) {
              // outs() << "riconosciuta una divisione\n";
              
              Instruction *StrengthRed = BinaryOperator::Create(Instruction::AShr,
                                          inst.getOperand(0), 
                                          ConstantInt::get(
                                            Integer32bitCtx,
                                            C->getValue().logBase2()));
              StrengthRed->insertAfter(&inst);
              inst.replaceAllUsesWith(StrengthRed);
              //se l'istruzione non viene piu' usata viene aggiunta al vettore di istruzioni da eliminare
              deadCode.push_back(&inst);
              

          } else if ( // 2. MOLTIPLICAZIONE CON IL SECONOD OPERANDO IN UN INTORNO DA 2^n
            (inst.getOpcode() == Instruction::Mul)
          ) {
            
            // outs() << "riconosciuta moltiplicazione\n";
            int distFromLog = strReductionProbe(C, MAX_DISTANCE);
            
            if (C->getValue().isPowerOf2()) { //strenght reduction classica
              Instruction *strMul = BinaryOperator::Create(Instruction::Shl, inst.getOperand(0), ConstantInt::get(Integer32bitCtx, C->getValue().logBase2()));
              strMul->insertAfter(&inst);
              inst.replaceAllUsesWith(strMul);
              //se l'istruzione non viene piu' usata viene aggiunta al vettore di istruzioni da eliminare
              deadCode.push_back(&inst);
            } else if(abs(distFromLog) < MAX_DISTANCE) {

              std::vector<Instruction*> daInserire; //array che ospiterà la strenght reduction e le add/sub successive
              // outs() << "l'operando dista da una potenza:" << strReductionProbe(C, MAX_DISTANCE) << "\n";
              
              /*18 ha distanza -2 : log+add, 15 ha distanza 1: ceilLog+sub
              if-in-linea per eseguire la giusta strenght reduction*/
              Instruction *strMul = (distFromLog > 0)  
              ? BinaryOperator::Create(Instruction::Shl, inst.getOperand(0), ConstantInt::get(Integer32bitCtx, C->getValue().ceilLogBase2())) /*ritorna il logaritmo in base 2 arrotondato per eccesso*/
              : BinaryOperator::Create(Instruction::Shl, inst.getOperand(0), ConstantInt::get(Integer32bitCtx, C->getValue().logBase2()));
              
              daInserire.push_back(strMul);
              daInserire[0]->insertAfter(&inst);
              
              /*crea le addizioni/sottrazioni per eguagliare l'istruzione originale*/
              for(int i=0; i<abs(distFromLog); i++) {
                Instruction *auxInstr = (distFromLog > 0) /*18 ha distanza -2 : log+add, 15 ha distanza 1: ceilLog+sub*/ 
                ? BinaryOperator::Create(Instruction::Sub, daInserire.back(), inst.getOperand(0)) /**/
                : BinaryOperator::Create(Instruction::Add, daInserire.back(), inst.getOperand(0));
                daInserire.push_back(auxInstr);
              }
              
              /*le istruzioni vengono concatenate le une con le altre, la strReduction è stata concatenata fuori dal loop*/
              for(int i=1; i<daInserire.size(); i++) {
                daInserire[i]->insertAfter(daInserire[i-1]);
              }
              inst.replaceAllUsesWith(daInserire.back());
              //se l'istruzione non viene piu' usata viene aggiunta al vettore di istruzioni da eliminare
              deadCode.push_back(&inst);
              
              
            }
            
                 
          }

        } else if (hasIntegerOp == 1) {
          if ( // 2. MOLTIPLICAZIONE CON IL PRIMO OPERANDO IN UN INTORNO DA 2^n
            (inst.getOpcode() == Instruction::Mul)
          ) {
            
            // outs() << "riconosciuta moltiplicazione\n";
            int distFromLog = strReductionProbe(C, MAX_DISTANCE);
            
            if (C->getValue().isPowerOf2()) {
              Instruction *strMul = BinaryOperator::Create(Instruction::Shl, inst.getOperand(1), ConstantInt::get(Integer32bitCtx, C->getValue().logBase2()));
              strMul->insertAfter(&inst);
              inst.replaceAllUsesWith(strMul);
            } else if(abs(distFromLog) < MAX_DISTANCE) {

              std::vector<Instruction*> daInserire;
              // outs() << "l'operando dista da una potenza:" << strReductionProbe(C, MAX_DISTANCE) << "\n";
              
              Instruction *strMul = (distFromLog > 0) /*18 ha distanza -2 : log+add, 15 ha distanza 1: ceilLog+sub*/ 
              ? BinaryOperator::Create(Instruction::Shl, inst.getOperand(1), ConstantInt::get(Integer32bitCtx, C->getValue().ceilLogBase2()))
              : BinaryOperator::Create(Instruction::Shl, inst.getOperand(1), ConstantInt::get(Integer32bitCtx, C->getValue().logBase2()));
              daInserire.push_back(strMul);
              daInserire[0]->insertAfter(&inst);
              
              for(int i=0; i<abs(distFromLog); i++) {
                Instruction *auxInstr = (distFromLog > 0) /*18 ha distanza -2 : log+add, 15 ha distanza 1: ceilLog+sub*/ 
                ? BinaryOperator::Create(Instruction::Sub, daInserire.back(), inst.getOperand(1))
                : BinaryOperator::Create(Instruction::Add, daInserire.back(), inst.getOperand(1));
                daInserire.push_back(auxInstr);
              }
              
              for(int i=1; i<daInserire.size(); i++) {
                daInserire[i]->insertAfter(daInserire[i-1]);
              }
              inst.replaceAllUsesWith(daInserire.back());
            } 

            //se l'istruzione non viene piu' usata viene aggiunta al vettore di istruzioni da eliminare
            deadCode.push_back(&inst);
            
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
llvm::PassPluginLibraryInfo getStrRedOptPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "StrRedOpt", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback( [](StringRef Name, FunctionPassManager &FPM,ArrayRef<PassBuilder::PipelineElement>)  //Using these callbacks, callers can parse both a single pass name, as well as entire sub-pipelines, and populate the PassManager instance accordingly. 
              {
                if (Name == "str-red-opt") {
                  FPM.addPass(StrRedOpt());
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
  return getStrRedOptPluginInfo();
}

  
