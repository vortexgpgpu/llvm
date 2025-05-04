#pragma once

#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"

namespace vortex {
using namespace llvm;

struct UniformAnnotationPass : PassInfoMixin<UniformAnnotationPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

class DivergenceTracker {
public:
  DivergenceTracker(const Function &function);

  bool isSourceOfDivergence(const Value *V);

  bool isAlwaysUniform(const Value *V);

private:
  void initialize();

  void buildAllocaTaints();

  bool loadIsDivergent(const LoadInst *LI);

  struct TaintAlloca {
    const Instruction *callInst;
    uint64_t     offset;
    uint64_t     size;
  };

  DenseMap<const AllocaInst*, SmallVector<TaintAlloca,4>> taints_;
  DenseSet<const Value *> dv_nodes_;
  DenseSet<const Value *> uv_nodes_;
  const Function* function_;
  bool initialized_;
};

}