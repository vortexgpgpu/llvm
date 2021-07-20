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

  DenseSet<const BasicBlock *> joins_;
  
  Function *split_func_;
  Function *join_func_;

  void initialize(Module &M, const RISCVSubtarget &ST);

public:

  static char ID; // Pass identification, replacement for typeid
  
  VortexBranchDivergence();

  StringRef getPassName() const override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  
  bool runOnFunction(Function &F) override;
};

}