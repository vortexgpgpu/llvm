#pragma once

#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"

// Divergence architecture selected by -vortex-divergence-arch and latched into
// gVortexDivergenceArch by RISCVTargetMachine when the vortex divergence
// pipeline is enabled (+xvortex and -vortex-branch-divergence != 0).
enum VortexDivergenceArch {
  VXDA_IPDOM  = 0, // baseline IPDOM split/join
  VXDA_SCS    = 1, // ThreadSplit (SCS): split/join + vx_yield on blocking loops
  VXDA_ITS    = 2, // per-thread-PC convergence barriers (vx_bar_add/vx_bar_wait)
};
extern int gVortexDivergenceArch;

// Fuse the divergence predicate/split into the following compare-branch, so the
// compiler emits vx_pbr/vx_sbr (a single B-type branch) instead of the
// setcc + vx_pred|vx_split + conditional-branch triple. Selected by
// -vortex-fused-divergence and latched by RISCVTargetMachine. This is a
// ThreadSplit (SCS) feature ONLY: emission is additionally gated on
// gVortexDivergenceArch == VXDA_SCS, so the baseline IPDOM arm keeps the
// legacy vx_split/vx_pred lowering and ITS is unaffected.
extern int gVortexFusedDivergence;

// Also fuse the divergent branch/select split into vx_sbr (implies the paired
// vx_join becomes tokenless). Selected by -vortex-fuse-split-branch and gated on
// gVortexFusedDivergence (SCS). Kept separate from the loop-predicate fusion
// (vx_pbr) because sbr's stack reconvergence needs its own validation; default
// off so vx_pbr can land independently.
extern int gVortexFuseSplitBranch;

namespace vortex {
using namespace llvm;

struct UniformAnnotationPass : PassInfoMixin<UniformAnnotationPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

struct dv_info_t {
public:
  bool isUniformRet() const {
    return uniform_ret_;
  }

  bool isUniformInArg(const Argument* Arg) const {
    return uniform_in_args_.count(Arg) != 0;
  }

  bool isUniformOutArg(const Argument* Arg) const {
    return uniform_out_args_.count(Arg) != 0;
  }

  bool isDivergentInstr(const Value* Instr) const {
    return divergent_instrs_.count(Instr) != 0;
  }

  bool setUniformRet(bool is_uniform) {
    if (uniform_ret_ != is_uniform) {
      uniform_ret_ = is_uniform;
      return true; // changed
    }
    return false; // not changed
  }

  bool setUniformInArg(const Argument* Arg, bool is_uniform) {
    if (is_uniform) {
      return uniform_in_args_.insert(Arg).second; // true if inserted
    } else {
      return uniform_in_args_.erase(Arg); // true if erased
    }
  }

  bool setUniformOutArg(const Argument* Arg, bool is_uniform) {
    if (is_uniform) {
      return uniform_out_args_.insert(Arg).second; // true if inserted
    } else {
      return uniform_out_args_.erase(Arg); // true if erased
    }
  }

  bool setDivergentInstr(const Value* Instr) {
    return divergent_instrs_.insert(Instr).second; // true if inserted
  }

private:
  DenseSet<const Argument*> uniform_in_args_;
  DenseSet<const Argument*> uniform_out_args_;
  DenseSet<const Value*>    divergent_instrs_;
  bool uniform_ret_ = false;
};

class DivergenceInfo {
public:
  static dv_info_t* get(const llvm::Function* F);

  static bool setUniformInArg(const llvm::Function* F, const llvm::Argument* Arg, bool is_uniform);

  static bool isUniformRet(const llvm::Function* F);

  static bool isUniformOutArg(const llvm::Function* F, const llvm::Argument* Arg);

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

  dv_info_t* dv_info_;
  const Function* function_;
  bool initialized_;
};

}