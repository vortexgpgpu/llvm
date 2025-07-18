#pragma once

#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"

namespace vortex {
using namespace llvm;

struct UniformAnnotationPass : PassInfoMixin<UniformAnnotationPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

struct dv_info_t {
  DenseSet<const Argument*> uniform_in_args;
  DenseSet<const Argument*> uniform_out_args;
  bool uniform_ret = false;
};

class DivergenceInfo {
public:
  static dv_info_t* get(const llvm::Function* F);

  static void setUniformArg(const llvm::Function* F, const llvm::Argument* Arg, bool is_uniform);

  static bool is_uniform_Call(const llvm::Function* F);

  static void clear(const llvm::Module* M);

private:
  static llvm::DenseMap<const llvm::Function*, dv_info_t> dv_infos_;
  static llvm::sys::Mutex mutex_;
};

class DivergenceTracker {
public:
  DivergenceTracker(const Function &F);

  bool isSourceOfDivergence(const Value *V);

  bool isAlwaysUniform(const Value *V);

private:
  void initialize();

  void buildAllocaTaints(const Function &F);

  bool loadIsDivergent(const Function &F, const LoadInst *LI);

  struct TaintAlloca {
    const Instruction* callInst;
    uint64_t offset;
    uint64_t size;
  };

  DenseMap<const AllocaInst*, SmallVector<TaintAlloca,4>> taints_;
  DenseSet<const Value*> dv_nodes_;
  DenseSet<const Value*> uv_nodes_;
  const Function* function_;
  bool initialized_;
  dv_info_t* dv_info_;
  bool external_linkage_;
};

}