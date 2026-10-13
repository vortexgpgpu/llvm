#pragma once

#include <memory>
#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"

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

  // The argument may point into a caller's stack, which holds a different
  // value in every thread at the same address.
  bool isPrivateArg(const Argument* Arg) const {
    return private_args_.count(Arg) != 0;
  }

  bool setUniformRet(bool is_uniform) {
    if (uniform_ret_ != is_uniform) {
      uniform_ret_ = is_uniform;
      return true; // changed
    }
    return false; // not changed
  }

  // An argument is uniform only if every call site passes a uniform value: a
  // divergent call site earlier in the same fixed-point iteration wins over a
  // uniform one after it. beginIteration() forgets those per-iteration verdicts.
  bool setUniformInArg(const Argument* Arg, bool is_uniform) {
    if (is_uniform) {
      if (iter_divergent_in_args_.count(Arg))
        return false;
      return uniform_in_args_.insert(Arg).second; // true if inserted
    }
    iter_divergent_in_args_.insert(Arg);
    return uniform_in_args_.erase(Arg); // true if erased
  }

  void beginIteration() {
    iter_divergent_in_args_.clear();
  }

  // Every argument divergent: the verdict when the fixed point did not settle.
  bool clearUniformInArgs() {
    bool changed = !uniform_in_args_.empty();
    uniform_in_args_.clear();
    return changed;
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

  // Monotonic: a call site that passes a private pointer makes the argument
  // private for good, so the module fixed point converges.
  bool setPrivateArg(const Argument* Arg) {
    return private_args_.insert(Arg).second; // true if inserted
  }

private:
  DenseSet<const Argument*> uniform_in_args_;
  DenseSet<const Argument*> uniform_out_args_;
  DenseSet<const Value*>    divergent_instrs_;
  DenseSet<const Argument*> private_args_;
  DenseSet<const Argument*> iter_divergent_in_args_;
  bool uniform_ret_ = false;
};

class DivergenceInfo {
public:
  static dv_info_t* get(const llvm::Function* F);

  static bool setUniformInArg(const llvm::Function* F, const llvm::Argument* Arg, bool is_uniform);

  static bool isUniformRet(const llvm::Function* F);

  static bool isUniformOutArg(const llvm::Function* F, const llvm::Argument* Arg);

  static bool setPrivateArg(const llvm::Function* F, const llvm::Argument* Arg);

  // A private address was stored to memory that is not itself private, so a
  // pointer loaded from memory may point into a thread's stack. Monotonic.
  static bool setPrivateEscaped(const llvm::Module* M);

  static bool isPrivateEscaped(const llvm::Module* M);

  static void beginIteration(const llvm::Module* M);

  static void clearUniformInArgs(const llvm::Module* M);

  static void clear(const llvm::Module* M);

private:
  /* unique_ptr values: get() hands out raw dv_info_t* that callers hold
   * across map-growing calls; inline values would dangle on rehash. */
  static llvm::DenseMap<const llvm::Function*, std::unique_ptr<dv_info_t>> dv_infos_;
  static llvm::DenseSet<const llvm::Module*> private_escaped_;
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
  // Load addresses already classified: one uniformity computation asks once
  // per load, and the walk behind each answer can be long.
  DenseMap<const Value*, bool> private_ptr_cache_;
};

}