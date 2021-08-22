#include "llvm/Analysis/LegacyDivergenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "RISCVSubtarget.h"

namespace vortex {
using namespace llvm;

class isSourceOfDivergenceHandler {
private:
    DenseSet<const Value *> DV_;

public:
    bool eval(const Value *V);
};

class VortexBranchDivergence : public FunctionPass {
private:

  LegacyDivergenceAnalysis *DA_;
  DominatorTree *DT_;
  PostDominatorTree *PDT_;
  LoopInfo *LI_;

  std::vector<std::pair<BasicBlock *, BasicBlock*>> splits_;
  std::vector<Loop *> loops_;
  DenseSet<const BasicBlock *> stub_blocks_;

  Function *tmask_func_;
  Function *pred_func_;
  Function *tmc_func_;  
  Function *split_func_;
  Function *join_func_;

  void initialize(Module &M, const RISCVSubtarget &ST);

  void processSplitJoins(LLVMContext* context, Function* function);

  void processLoops(LLVMContext* context, Function* function);

  void recurseReplaceSuccessor(BasicBlock* start, BasicBlock* oldBB, BasicBlock* newBB);

  void recurseReplaceSuccessor(DenseSet<const BasicBlock *>& visited, 
                               BasicBlock* start, 
                               BasicBlock* oldBB, 
                               BasicBlock* newBB);

public:

  static char ID; // Pass identification, replacement for typeid
  
  VortexBranchDivergence();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  
  bool runOnFunction(Function &F) override;
};

}